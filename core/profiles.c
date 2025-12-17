#include "telescope.h"
#include <string.h>
#include <stdlib.h>

/**
 * Performance profile management
 */

int telescope_config_apply_profile(struct telescope_config *config,
                                   telescope_profile_t profile) {
    if (!config) {
        return -1;
    }
    
    config->performance.profile = profile;
    
    switch (profile) {
        case TELESCOPE_PROFILE_LOW_LATENCY:
            config->performance.target_latency_ms = 16;
            config->performance.frame_rate = 120;
            config->performance.enable_prediction = true;
            config->performance.prediction_window_ms = 16;
            config->performance.enable_scroll_smoothing = true;
            
            if (config->connection.compression) {
                free(config->connection.compression);
            }
            config->connection.compression = strdup("lz4");
            
            if (config->connection.video_codec) {
                free(config->connection.video_codec);
            }
            config->connection.video_codec = strdup("h264");
            
            config->connection.bandwidth_limit_mbps = 0;
            break;
            
        case TELESCOPE_PROFILE_BALANCED:
            config->performance.target_latency_ms = 50;
            config->performance.frame_rate = 60;
            config->performance.enable_prediction = true;
            config->performance.prediction_window_ms = 16;
            config->performance.enable_scroll_smoothing = true;
            
            if (config->connection.compression) {
                free(config->connection.compression);
            }
            config->connection.compression = strdup("lz4");
            
            if (config->connection.video_codec) {
                free(config->connection.video_codec);
            }
            config->connection.video_codec = strdup("h264");
            
            config->connection.bandwidth_limit_mbps = 0;
            break;
            
        case TELESCOPE_PROFILE_HIGH_QUALITY:
            config->performance.target_latency_ms = 100;
            config->performance.frame_rate = 60;
            config->performance.enable_prediction = false;
            config->performance.prediction_window_ms = 0;
            config->performance.enable_scroll_smoothing = false;
            
            if (config->connection.compression) {
                free(config->connection.compression);
            }
            config->connection.compression = strdup("zstd");
            
            if (config->connection.video_codec) {
                free(config->connection.video_codec);
            }
            config->connection.video_codec = strdup("h265");
            
            config->connection.bandwidth_limit_mbps = 0;
            break;
            
        case TELESCOPE_PROFILE_BANDWIDTH_CONSTRAINED:
            config->performance.target_latency_ms = 100;
            config->performance.frame_rate = 30;
            config->performance.enable_prediction = true;
            config->performance.prediction_window_ms = 33;
            config->performance.enable_scroll_smoothing = true;
            
            if (config->connection.compression) {
                free(config->connection.compression);
            }
            config->connection.compression = strdup("zstd");
            
            if (config->connection.video_codec) {
                free(config->connection.video_codec);
            }
            config->connection.video_codec = strdup("h265");
            
            config->connection.bandwidth_limit_mbps = 10;
            break;
    }
    
    return 0;
}

telescope_lens_t telescope_select_lens(const struct telescope_config *config) {
    if (!config) {
        return TELESCOPE_LENS_WAYPIPE;
    }
    
    /* If lens type is explicitly set and not auto, use it */
    if (config->lens.type != TELESCOPE_LENS_AUTO) {
        return config->lens.type;
    }
    
    /* Auto-select based on application characteristics */
    const char *executable = config->application.executable;
    if (!executable) {
        return TELESCOPE_LENS_WAYPIPE;
    }
    
    /* Heuristic: video/gaming applications benefit from Sunshine/Moonlight */
    if (strstr(executable, "mpv") || strstr(executable, "vlc") ||
        strstr(executable, "ffmpeg") || strstr(executable, "game") ||
        strstr(executable, "steam")) {
        return TELESCOPE_LENS_SUNSHINE;
    }
    
    /* Default to waypipe for general applications */
    return TELESCOPE_LENS_WAYPIPE;
}

