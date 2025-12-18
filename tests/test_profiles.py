#!/usr/bin/env python3
"""
Unit tests for performance profiles
"""

import sys
import os
import json
import tempfile

# Add profiles directory to path
profiles_path = os.path.join(os.path.dirname(__file__), '..', 'profiles')
sys.path.insert(0, profiles_path)

# Import the module (it's a script, so we need to import it differently)
import importlib.util
spec = importlib.util.spec_from_file_location(
    "waypipe_performance_profiles",
    os.path.join(profiles_path, "waypipe-performance-profiles.py")
)
waypipe_profiles = importlib.util.module_from_spec(spec)
spec.loader.exec_module(waypipe_profiles)

# Import functions
get_profile = waypipe_profiles.get_profile
list_profiles = waypipe_profiles.list_profiles
apply_profile = waypipe_profiles.apply_profile
validate_profile_compatibility = waypipe_profiles.validate_profile_compatibility
generate_config_template = waypipe_profiles.generate_config_template


def test_list_profiles():
    """Test listing all profiles"""
    profiles = list_profiles()
    assert len(profiles) > 0
    assert "low-latency" in profiles
    assert "balanced" in profiles
    assert "high-quality" in profiles
    assert "bandwidth-constrained" in profiles
    print("✓ test_list_profiles passed")


def test_get_profile():
    """Test getting a specific profile"""
    profile = get_profile("low-latency")
    assert profile is not None
    assert profile.name == "low-latency"
    assert profile.target_latency_ms == 16
    assert profile.frame_rate == 120
    assert profile.enable_prediction is True
    
    profile = get_profile("nonexistent")
    assert profile is None
    print("✓ test_get_profile passed")


def test_apply_profile():
    """Test applying a profile to configuration"""
    config = {
        "connection": {},
        "application": {
            "executable": "/usr/bin/test",
            "args": []
        },
        "performance": {}
    }
    
    config = apply_profile(config, "low-latency")
    assert config["performance"]["profile"] == "low-latency"
    assert config["performance"]["target_latency_ms"] == 16
    assert config["performance"]["frame_rate"] == 120
    assert config["connection"]["compression"] == "lz4"
    print("✓ test_apply_profile passed")


def test_validate_profile_compatibility():
    """Test profile compatibility validation"""
    config = {
        "connection": {
            "bandwidth_limit": 10,
            "compression": "lz4"
        },
        "performance": {
            "profile": "low-latency",
            "enable_prediction": True
        }
    }
    
    valid, error = validate_profile_compatibility(config)
    assert valid is True
    
    # Test invalid: bandwidth limited but compression disabled
    config["connection"]["compression"] = "none"
    valid, error = validate_profile_compatibility(config)
    assert valid is False
    assert "compression" in error.lower()
    print("✓ test_validate_profile_compatibility passed")


def test_generate_config_template():
    """Test generating configuration template"""
    config = generate_config_template("balanced")
    assert "connection" in config
    assert "application" in config
    assert "performance" in config
    assert config["performance"]["profile"] == "balanced"
    assert config["performance"]["target_latency_ms"] == 50
    print("✓ test_generate_config_template passed")


def main():
    print("Running profile tests...\n")
    
    test_list_profiles()
    test_get_profile()
    test_apply_profile()
    test_validate_profile_compatibility()
    test_generate_config_template()
    
    print("\nAll profile tests passed!")
    return 0


if __name__ == "__main__":
    sys.exit(main())

