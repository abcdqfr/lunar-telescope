#!/usr/bin/env python3
"""
Waypipe Performance Profiles

Defines performance profile presets for different use cases.
These profiles are used by the connection orchestration to optimize
latency, bandwidth, and quality trade-offs.

This module is used for configuration generation and validation,
not in the runtime hot path (which is C).
"""

from dataclasses import dataclass
from typing import Dict, Any, Optional
import json


@dataclass
class PerformanceProfile:
    """Performance profile configuration"""
    name: str
    target_latency_ms: int
    frame_rate: int
    compression: str
    video_codec: str
    bandwidth_limit: int
    enable_prediction: bool
    prediction_window_ms: int
    enable_scroll_smoothing: bool
    description: str


# Performance profile presets
PROFILES: Dict[str, PerformanceProfile] = {
    "low-latency": PerformanceProfile(
        name="low-latency",
        target_latency_ms=16,
        frame_rate=120,
        compression="lz4",
        video_codec="h264",
        bandwidth_limit=0,  # Unlimited
        enable_prediction=True,
        prediction_window_ms=16,
        enable_scroll_smoothing=True,
        description="Optimized for competitive gaming and real-time interaction"
    ),
    "balanced": PerformanceProfile(
        name="balanced",
        target_latency_ms=50,
        frame_rate=60,
        compression="lz4",
        video_codec="h264",
        bandwidth_limit=0,
        enable_prediction=True,
        prediction_window_ms=16,
        enable_scroll_smoothing=True,
        description="Default profile balancing latency and quality"
    ),
    "high-quality": PerformanceProfile(
        name="high-quality",
        target_latency_ms=100,
        frame_rate=60,
        compression="zstd",
        video_codec="h265",
        bandwidth_limit=0,
        enable_prediction=False,
        prediction_window_ms=0,
        enable_scroll_smoothing=False,
        description="Prioritizes visual quality over latency"
    ),
    "bandwidth-constrained": PerformanceProfile(
        name="bandwidth-constrained",
        target_latency_ms=100,
        frame_rate=30,
        compression="zstd",
        video_codec="h265",
        bandwidth_limit=10,  # 10 Mbps
        enable_prediction=True,
        prediction_window_ms=33,
        enable_scroll_smoothing=True,
        description="Optimized for limited bandwidth connections"
    )
}


def get_profile(name: str) -> Optional[PerformanceProfile]:
    """Get a performance profile by name"""
    return PROFILES.get(name)


def list_profiles() -> list[str]:
    """List all available profile names"""
    return list(PROFILES.keys())


def apply_profile(config: Dict[str, Any], profile_name: str) -> Dict[str, Any]:
    """
    Apply a performance profile to a configuration dictionary.
    
    Args:
        config: Configuration dictionary (may be partial)
        profile_name: Name of the profile to apply
        
    Returns:
        Updated configuration dictionary with profile settings applied
    """
    profile = get_profile(profile_name)
    if not profile:
        raise ValueError(f"Unknown profile: {profile_name}")
    
    # Ensure nested dictionaries exist
    if "performance" not in config:
        config["performance"] = {}
    if "connection" not in config:
        config["connection"] = {}
    
    # Apply profile settings
    config["performance"]["profile"] = profile.name
    config["performance"]["target_latency_ms"] = profile.target_latency_ms
    config["performance"]["frame_rate"] = profile.frame_rate
    config["performance"]["enable_prediction"] = profile.enable_prediction
    config["performance"]["prediction_window_ms"] = profile.prediction_window_ms
    config["performance"]["enable_scroll_smoothing"] = profile.enable_scroll_smoothing
    
    config["connection"]["compression"] = profile.compression
    config["connection"]["video_codec"] = profile.video_codec
    config["connection"]["bandwidth_limit"] = profile.bandwidth_limit
    
    return config


def validate_profile_compatibility(config: Dict[str, Any]) -> tuple[bool, Optional[str]]:
    """
    Validate that configuration is compatible with selected profile.
    
    Returns:
        (is_valid, error_message)
    """
    perf = config.get("performance", {})
    profile_name = perf.get("profile", "balanced")
    
    profile = get_profile(profile_name)
    if not profile:
        return False, f"Unknown profile: {profile_name}"
    
    # Check for conflicting settings
    conn = config.get("connection", {})
    
    # If bandwidth is limited, ensure compression is enabled
    bandwidth_limit = conn.get("bandwidth_limit", 0)
    if bandwidth_limit > 0 and conn.get("compression") == "none":
        return False, "Compression must be enabled when bandwidth is limited"
    
    # If low latency, ensure prediction is enabled
    if profile.target_latency_ms < 30 and not perf.get("enable_prediction", True):
        return False, "Prediction should be enabled for low-latency profiles"
    
    return True, None


def generate_config_template(profile_name: str = "balanced") -> Dict[str, Any]:
    """
    Generate a configuration template with a specific profile applied.
    
    Args:
        profile_name: Profile to use
        
    Returns:
        Complete configuration dictionary
    """
    template = {
        "connection": {
            "remote_host": "example.com",
            "remote_port": 22,
            "ssh_user": "user",
            "compression": "lz4",
            "video_codec": "h264",
            "bandwidth_limit": 0
        },
        "application": {
            "executable": "/usr/bin/application",
            "args": [],
            "working_directory": "/home/user"
        },
        "performance": {},
        "observability": {
            "enable_metrics": True,
            "metrics_interval_ms": 1000,
            "log_level": "info"
        },
        "lens": {
            "type": "auto",
            "fallback": ["waypipe", "sunshine", "moonlight"]
        }
    }
    
    return apply_profile(template, profile_name)


if __name__ == "__main__":
    # CLI interface for profile management
    import sys
    
    if len(sys.argv) < 2:
        print("Usage:")
        print("  python waypipe-performance-profiles.py list")
        print("  python waypipe-performance-profiles.py show <profile>")
        print("  python waypipe-performance-profiles.py generate <profile>")
        sys.exit(1)
    
    command = sys.argv[1]
    
    if command == "list":
        print("Available profiles:")
        for name in list_profiles():
            profile = get_profile(name)
            print(f"  {name}: {profile.description}")
    
    elif command == "show":
        if len(sys.argv) < 3:
            print("Error: profile name required")
            sys.exit(1)
        profile = get_profile(sys.argv[2])
        if profile:
            print(json.dumps({
                "name": profile.name,
                "target_latency_ms": profile.target_latency_ms,
                "frame_rate": profile.frame_rate,
                "compression": profile.compression,
                "video_codec": profile.video_codec,
                "bandwidth_limit": profile.bandwidth_limit,
                "enable_prediction": profile.enable_prediction,
                "prediction_window_ms": profile.prediction_window_ms,
                "enable_scroll_smoothing": profile.enable_scroll_smoothing,
                "description": profile.description
            }, indent=2))
        else:
            print(f"Error: Unknown profile: {sys.argv[2]}")
            sys.exit(1)
    
    elif command == "generate":
        if len(sys.argv) < 3:
            print("Error: profile name required")
            sys.exit(1)
        config = generate_config_template(sys.argv[2])
        print(json.dumps(config, indent=2))
    
    else:
        print(f"Error: Unknown command: {command}")
        sys.exit(1)

