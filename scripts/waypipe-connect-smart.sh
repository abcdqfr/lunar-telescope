#!/usr/bin/env bash
# waypipe-connect-smart.sh
#
# Smart connection orchestration for Waypipe-based remote applications.
# This script handles discovery, validation, profile selection, and
# connection establishment with automatic fallback and performance tuning.
#
# Usage:
#   waypipe-connect-smart.sh <config.json> [--profile <name>] [--dry-run]

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Default values
CONFIG_FILE=""
PROFILE_OVERRIDE=""
DRY_RUN=false
VERBOSE=false

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --profile)
            PROFILE_OVERRIDE="$2"
            shift 2
            ;;
        --dry-run)
            DRY_RUN=true
            shift
            ;;
        --verbose|-v)
            VERBOSE=true
            shift
            ;;
        --help|-h)
            echo "Usage: $0 <config.json> [--profile <name>] [--dry-run] [--verbose]"
            echo ""
            echo "Options:"
            echo "  --profile <name>    Override performance profile"
            echo "  --dry-run          Validate and show commands without executing"
            echo "  --verbose          Enable verbose logging"
            exit 0
            ;;
        *)
            if [[ -z "$CONFIG_FILE" ]]; then
                CONFIG_FILE="$1"
            else
                echo "Error: Unknown argument: $1" >&2
                exit 1
            fi
            shift
            ;;
    esac
done

if [[ -z "$CONFIG_FILE" ]]; then
    echo "Error: Configuration file required" >&2
    exit 1
fi

if [[ ! -f "$CONFIG_FILE" ]]; then
    echo "Error: Configuration file not found: $CONFIG_FILE" >&2
    exit 1
fi

# Logging function
log() {
    if [[ "$VERBOSE" == "true" ]] || [[ "$1" != "DEBUG" ]]; then
        echo "[$(date +%H:%M:%S)] [$1] $2" >&2
    fi
}

# Check for required tools
check_dependencies() {
    local missing=()
    
    if ! command -v waypipe &> /dev/null; then
        missing+=("waypipe")
    fi
    
    if ! command -v python3 &> /dev/null; then
        missing+=("python3")
    fi
    
    if ! command -v ssh &> /dev/null; then
        missing+=("ssh")
    fi
    
    if [[ ${#missing[@]} -gt 0 ]]; then
        echo "Error: Missing required dependencies: ${missing[*]}" >&2
        exit 1
    fi
}

# Validate JSON schema
validate_config() {
    log "INFO" "Validating configuration schema..."
    
    # Check if jsonschema is available (optional, graceful degradation)
    if command -v python3 &> /dev/null; then
        python3 -c "
import json
import sys

try:
    import jsonschema
    schema_path = '$PROJECT_ROOT/schemas/waypipe-schema.json'
    config_path = '$CONFIG_FILE'
    
    with open(schema_path) as f:
        schema = json.load(f)
    with open(config_path) as f:
        config = json.load(f)
    
    jsonschema.validate(instance=config, schema=schema)
    print('Configuration valid')
except ImportError:
    # Fallback: basic JSON validation
    with open('$CONFIG_FILE') as f:
        json.load(f)
    print('Configuration JSON valid (schema validation skipped, install jsonschema for full validation)')
except jsonschema.ValidationError as e:
    print(f'Validation error: {e.message}', file=sys.stderr)
    sys.exit(1)
except Exception as e:
    print(f'Error: {e}', file=sys.stderr)
    sys.exit(1)
" || exit 1
    else
        log "WARN" "Python3 not available, skipping schema validation"
    fi
}

# Apply performance profile
apply_profile() {
    if [[ -n "$PROFILE_OVERRIDE" ]]; then
        log "INFO" "Applying profile override: $PROFILE_OVERRIDE"
        python3 "$PROJECT_ROOT/profiles/waypipe-performance-profiles.py" generate "$PROFILE_OVERRIDE" > /tmp/waypipe-profile-$$.json
        
        # Merge profile with config (simple merge, profile takes precedence)
        python3 -c "
import json
import sys

with open('$CONFIG_FILE') as f:
    config = json.load(f)
with open('/tmp/waypipe-profile-$$.json') as f:
    profile = json.load(f)

# Deep merge
def merge_dict(base, override):
    for key, value in override.items():
        if key in base and isinstance(base[key], dict) and isinstance(value, dict):
            merge_dict(base[key], value)
        else:
            base[key] = value

merge_dict(config, profile)
print(json.dumps(config, indent=2))
" > /tmp/waypipe-config-$$.json
        
        CONFIG_FILE="/tmp/waypipe-config-$$.json"
    fi
}

# Test remote connectivity
test_connectivity() {
    local host
    local port
    
    host=$(python3 -c "import json; print(json.load(open('$CONFIG_FILE'))['connection']['remote_host'])")
    port=$(python3 -c "import json; print(json.load(open('$CONFIG_FILE'))['connection'].get('remote_port', 22))")
    
    log "INFO" "Testing connectivity to $host:$port..."
    
    if ! timeout 5 bash -c "echo > /dev/tcp/$host/$port" 2>/dev/null; then
        log "WARN" "Cannot reach $host:$port, connection may fail"
        return 1
    fi
    
    log "INFO" "Connectivity test passed"
    return 0
}

# Select transport lens
select_lens() {
    local lens_type
    
    lens_type=$(python3 -c "
import json
config = json.load(open('$CONFIG_FILE'))
print(config.get('lens', {}).get('type', 'auto'))
")
    
    if [[ "$lens_type" == "auto" ]]; then
        # Auto-select based on application characteristics
        local app_exec
        app_exec=$(python3 -c "import json; print(json.load(open('$CONFIG_FILE'))['application']['executable'])")
        
        # Heuristic: video/gaming apps -> sunshine/moonlight, others -> waypipe
        if echo "$app_exec" | grep -qiE "(mpv|vlc|ffmpeg|game|steam)"; then
            lens_type="sunshine"
            log "INFO" "Auto-selected lens: sunshine (video/gaming application detected)"
        else
            lens_type="waypipe"
            log "INFO" "Auto-selected lens: waypipe (default for general applications)"
        fi
    fi
    
    echo "$lens_type"
}

# Build waypipe command
build_waypipe_command() {
    local lens_type="$1"
    local cmd_args=()
    
    # Extract configuration
    local remote_host remote_port ssh_user compression video_codec
    local app_exec app_args app_env app_wd
    
    remote_host=$(python3 -c "import json; print(json.load(open('$CONFIG_FILE'))['connection']['remote_host'])")
    remote_port=$(python3 -c "import json; print(json.load(open('$CONFIG_FILE'))['connection'].get('remote_port', 22))")
    ssh_user=$(python3 -c "import json; print(json.load(open('$CONFIG_FILE'))['connection'].get('ssh_user', 'root'))")
    compression=$(python3 -c "import json; print(json.load(open('$CONFIG_FILE'))['connection'].get('compression', 'lz4'))")
    video_codec=$(python3 -c "import json; print(json.load(open('$CONFIG_FILE'))['connection'].get('video_codec', 'h264'))")
    
    app_exec=$(python3 -c "import json; print(json.load(open('$CONFIG_FILE'))['application']['executable'])")
    app_args=$(python3 -c "import json; import sys; args = json.load(open('$CONFIG_FILE'))['application'].get('args', []); print(' '.join(sys.argv[1:]) if sys.argv[1:] else '')" -- "${app_args[@]}")
    
    # Build waypipe command
    cmd_args+=("waypipe")
    cmd_args+=("client")
    
    # Compression
    if [[ "$compression" != "none" ]]; then
        cmd_args+=("--compress=$compression")
    fi
    
    # Video codec (if waypipe supports it)
    if [[ -n "$video_codec" ]]; then
        cmd_args+=("--video-codec=$video_codec")
    fi
    
    # SSH connection
    cmd_args+=("--ssh")
    cmd_args+=("$ssh_user@$remote_host")
    cmd_args+=("--")
    
    # Application command
    cmd_args+=("$app_exec")
    
    # Application arguments
    if [[ -n "$app_args" ]]; then
        read -ra args_array <<< "$app_args"
        cmd_args+=("${args_array[@]}")
    fi
    
    echo "${cmd_args[@]}"
}

# Main execution
main() {
    log "INFO" "Starting smart connection orchestration"
    
    check_dependencies
    validate_config
    apply_profile
    
    if ! test_connectivity; then
        log "WARN" "Connectivity test failed, proceeding anyway..."
    fi
    
    local lens_type
    lens_type=$(select_lens)
    log "INFO" "Selected transport lens: $lens_type"
    
    # For now, we only support waypipe directly
    # Sunshine/Moonlight integration would be handled by lens adapters
    if [[ "$lens_type" != "waypipe" ]]; then
        log "WARN" "Lens type '$lens_type' not yet implemented, falling back to waypipe"
        lens_type="waypipe"
    fi
    
    local waypipe_cmd
    waypipe_cmd=$(build_waypipe_command "$lens_type")
    
    log "INFO" "Waypipe command: $waypipe_cmd"
    
    if [[ "$DRY_RUN" == "true" ]]; then
        log "INFO" "Dry-run mode: would execute:"
        echo "$waypipe_cmd"
        exit 0
    fi
    
    # Execute waypipe
    log "INFO" "Launching waypipe connection..."
    exec $waypipe_cmd
}

# Cleanup on exit
cleanup() {
    rm -f /tmp/waypipe-config-$$.json /tmp/waypipe-profile-$$.json
}
trap cleanup EXIT

main "$@"

