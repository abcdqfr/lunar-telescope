#include "telescope.h"
#include "lens.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>

/* Forward declaration for metrics */
extern int metrics_collector_init(const telescope_observability_t *obs_config);
extern void metrics_collector_cleanup(void);
extern const struct telescope_metrics *metrics_collector_get(void);

/**
 * Telescope session management
 */

struct telescope_session {
    struct telescope_config *config;
    telescope_lens_t lens_type;
    struct lens_session *lens_session;
    bool running;
    struct telescope_metrics metrics;
    uint64_t start_time_us;
};

int telescope_session_create(const struct telescope_config *config,
                            struct telescope_session **session_out) {
    if (!config || !session_out) {
        return -EINVAL;
    }
    
    struct telescope_session *session = calloc(1, sizeof(struct telescope_session));
    if (!session) {
        return -ENOMEM;
    }
    
    /* Copy configuration (simplified - in production would deep copy) */
    session->config = (struct telescope_config *)config;
    session->lens_type = TELESCOPE_LENS_WAYPIPE;
    session->lens_session = NULL;
    session->running = false;
    
    memset(&session->metrics, 0, sizeof(session->metrics));
    
    *session_out = session;
    return 0;
}

/* Transport process launching is handled by the lens layer (`lenses/`). */

int telescope_session_start(struct telescope_session *session) {
    if (!session || !session->config) {
        return -EINVAL;
    }
    
    if (session->running) {
        return -EBUSY;
    }
    
    /* Try primary lens first, then configured fallbacks, then waypipe as last resort. */
    telescope_lens_t candidates[8];
    size_t candidate_count = 0;

    telescope_lens_t primary = telescope_select_lens(session->config);
    candidates[candidate_count++] = primary;

    if (session->config->lens.fallback && session->config->lens.fallback_count > 0) {
        for (size_t i = 0; i < session->config->lens.fallback_count && candidate_count < (sizeof(candidates) / sizeof(candidates[0])); i++) {
            telescope_lens_t t = session->config->lens.fallback[i];
            bool seen = false;
            for (size_t j = 0; j < candidate_count; j++) {
                if (candidates[j] == t) {
                    seen = true;
                    break;
                }
            }
            if (!seen) {
                candidates[candidate_count++] = t;
            }
        }
    }

    /* Ensure waypipe is always attempted last (widest availability). */
    bool has_waypipe = false;
    for (size_t j = 0; j < candidate_count; j++) {
        if (candidates[j] == TELESCOPE_LENS_WAYPIPE) {
            has_waypipe = true;
            break;
        }
    }
    if (!has_waypipe && candidate_count < (sizeof(candidates) / sizeof(candidates[0]))) {
        candidates[candidate_count++] = TELESCOPE_LENS_WAYPIPE;
    }

    int ret = -ENOTSUP;
    for (size_t i = 0; i < candidate_count; i++) {
        telescope_lens_t lens = candidates[i];
        struct lens_session *ls = NULL;

        ret = lens_session_create(lens, session->config, &ls);
        if (ret < 0) {
            continue;
        }

        ret = lens_session_start(ls);
        if (ret < 0) {
            lens_session_destroy(ls);
            continue;
        }

        session->lens_type = lens;
        session->lens_session = ls;
        ret = 0;
        break;
    }

    if (ret < 0) {
        session->lens_session = NULL;
        return ret;
    }
    
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    session->start_time_us = ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;
    session->running = true;
    
    /* Initialize metrics collection */
    metrics_collector_init(&session->config->observability);
    
    /* Initialize logging if available */
    #ifdef LOGGING_H
    extern int logging_init(log_level_t, FILE *);
    logging_init(session->config->observability.log_level, NULL);
    #endif
    
    return 0;
}

int telescope_session_stop(struct telescope_session *session) {
    if (!session) {
        return -EINVAL;
    }
    
    if (!session->running) {
        return 0;
    }

    if (session->lens_session) {
        (void)lens_session_stop(session->lens_session);
        lens_session_destroy(session->lens_session);
        session->lens_session = NULL;
    }
    
    session->running = false;
    
    /* Cleanup metrics */
    metrics_collector_cleanup();
    
    return 0;
}

void telescope_session_destroy(struct telescope_session *session) {
    if (!session) {
        return;
    }
    
    telescope_session_stop(session);
    free(session);
}

int telescope_session_get_metrics(const struct telescope_session *session,
                                  struct telescope_metrics *metrics_out) {
    if (!session || !metrics_out) {
        return -EINVAL;
    }
    
    /* Try to get metrics from collector first */
    const struct telescope_metrics *collected = metrics_collector_get();
    if (collected) {
        *metrics_out = *collected;
    } else {
        /* Fallback to session metrics */
        *metrics_out = session->metrics;
        
        /* Update timestamp */
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        metrics_out->timestamp_us = ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;
    }
    
    return 0;
}

