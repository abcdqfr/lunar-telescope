#ifndef TELESCOPE_H
#define TELESCOPE_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

/* Forward declare logging if available */
#ifndef LOGGING_H
typedef enum {
    LOG_LEVEL_ERROR = 0,
    LOG_LEVEL_WARN = 1,
    LOG_LEVEL_INFO = 2,
    LOG_LEVEL_DEBUG = 3,
    LOG_LEVEL_TRACE = 4
} log_level_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Lunar Telescope Core API
 *
 * This header defines the core orchestration interface for remote
 * Wayland application publishing with predictive input and performance
 * optimization.
 */

/* Forward declarations */
struct telescope_config;
struct telescope_session;
struct telescope_metrics;

/**
 * Performance profile types
 */
typedef enum {
    TELESCOPE_PROFILE_LOW_LATENCY,
    TELESCOPE_PROFILE_BALANCED,
    TELESCOPE_PROFILE_HIGH_QUALITY,
    TELESCOPE_PROFILE_BANDWIDTH_CONSTRAINED
} telescope_profile_t;

/**
 * Transport lens types
 */
typedef enum {
    TELESCOPE_LENS_WAYPIPE,
    TELESCOPE_LENS_SUNSHINE,
    TELESCOPE_LENS_MOONLIGHT,
    TELESCOPE_LENS_AUTO
} telescope_lens_t;

/**
 * Connection configuration
 */
typedef struct {
    char *remote_host;
    uint16_t remote_port;
    char *ssh_user;
    char *ssh_key_path;
    char *compression;      /* "none", "lz4", "zstd" */
    char *video_codec;      /* "h264", "h265", "vp8", "vp9", "av1" */
    uint32_t bandwidth_limit_mbps;  /* 0 = unlimited */
} telescope_connection_t;

/**
 * Application configuration
 */
typedef struct {
    char *executable;
    char **args;
    size_t args_count;
    char **env;
    size_t env_count;
    char *working_directory;
} telescope_application_t;

/**
 * Performance configuration
 */
typedef struct {
    telescope_profile_t profile;
    uint32_t target_latency_ms;
    uint32_t frame_rate;    /* 0 = adaptive */
    bool enable_prediction;
    uint32_t prediction_window_ms;
    bool enable_scroll_smoothing;
} telescope_performance_t;

/**
 * Observability configuration
 */
typedef struct {
    bool enable_metrics;
    uint32_t metrics_interval_ms;
    char *metrics_file;
    int log_level;  /* 0=error, 1=warn, 2=info, 3=debug, 4=trace */
} telescope_observability_t;

/**
 * Lens configuration
 */
typedef struct {
    telescope_lens_t type;
    telescope_lens_t *fallback;
    size_t fallback_count;
} telescope_lens_config_t;

/**
 * Complete telescope configuration
 */
struct telescope_config {
    telescope_connection_t connection;
    telescope_application_t application;
    telescope_performance_t performance;
    telescope_observability_t observability;
    telescope_lens_config_t lens;
};

/**
 * Session metrics
 */
struct telescope_metrics {
    /* Latency metrics (milliseconds) */
    uint32_t end_to_end_latency_ms;
    uint32_t input_lag_ms;
    uint32_t frame_delay_ms;
    
    /* Frame metrics */
    uint32_t frames_per_second;
    uint32_t frames_dropped;
    uint32_t frames_total;
    
    /* Bandwidth metrics (bytes per second) */
    uint64_t bandwidth_rx_bps;
    uint64_t bandwidth_tx_bps;
    
    /* Input metrics */
    uint32_t input_events_predicted;
    uint32_t input_events_reconciled;
    uint32_t input_events_total;
    
    /* Timestamp of last update */
    uint64_t timestamp_us;
};

/**
 * Initialize telescope configuration from JSON file
 *
 * @param config_path Path to JSON configuration file
 * @param config_out Output configuration structure (must be freed with telescope_config_free)
 * @return 0 on success, negative error code on failure
 */
int telescope_config_load(const char *config_path, struct telescope_config **config_out);

/**
 * Free telescope configuration
 */
void telescope_config_free(struct telescope_config *config);

/**
 * Create a new telescope session
 *
 * @param config Configuration to use
 * @param session_out Output session handle
 * @return 0 on success, negative error code on failure
 */
int telescope_session_create(const struct telescope_config *config,
                            struct telescope_session **session_out);

/**
 * Start the telescope session (launch remote application)
 *
 * @param session Session handle
 * @return 0 on success, negative error code on failure
 */
int telescope_session_start(struct telescope_session *session);

/**
 * Stop the telescope session
 *
 * @param session Session handle
 * @return 0 on success, negative error code on failure
 */
int telescope_session_stop(struct telescope_session *session);

/**
 * Destroy telescope session
 */
void telescope_session_destroy(struct telescope_session *session);

/**
 * Get current session metrics
 *
 * @param session Session handle
 * @param metrics_out Output metrics structure
 * @return 0 on success, negative error code on failure
 */
int telescope_session_get_metrics(const struct telescope_session *session,
                                  struct telescope_metrics *metrics_out);

/**
 * Apply performance profile to configuration
 *
 * @param config Configuration to modify
 * @param profile Profile to apply
 * @return 0 on success, negative error code on failure
 */
int telescope_config_apply_profile(struct telescope_config *config,
                                   telescope_profile_t profile);

/**
 * Select optimal transport lens based on application characteristics
 *
 * @param config Configuration containing application info
 * @return Selected lens type
 */
telescope_lens_t telescope_select_lens(const struct telescope_config *config);

#ifdef __cplusplus
}
#endif

#endif /* TELESCOPE_H */

