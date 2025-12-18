#include "telescope.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

/**
 * Metrics collection and observability
 */

/* Bandwidth sample for time-based averaging */
struct bandwidth_sample {
    uint64_t timestamp_us;
    uint64_t rx_bytes;
    uint64_t tx_bytes;
    struct bandwidth_sample *next;
};

struct metrics_collector {
    struct telescope_metrics metrics;
    bool enabled;
    uint32_t interval_ms;
    char *metrics_file;
    FILE *metrics_fp;
    uint64_t last_collection_us;
    uint64_t frame_counter;
    uint64_t input_event_counter;
    
    /* Time-based bandwidth averaging */
    struct bandwidth_sample *bandwidth_samples;
    size_t bandwidth_sample_count;
    uint64_t bandwidth_window_us;  /* Averaging window (default 1 second) */
    uint64_t total_rx_bytes;
    uint64_t total_tx_bytes;
    uint64_t bandwidth_last_update_us;
};

static struct metrics_collector *g_collector = NULL;

int metrics_collector_init(const telescope_observability_t *obs_config) {
    if (!obs_config || !obs_config->enable_metrics) {
        return 0;  /* Metrics disabled */
    }
    
    if (g_collector) {
        return -EBUSY;  /* Already initialized */
    }
    
    g_collector = calloc(1, sizeof(struct metrics_collector));
    if (!g_collector) {
        return -ENOMEM;
    }
    
    g_collector->enabled = true;
    g_collector->interval_ms = obs_config->metrics_interval_ms;
    g_collector->last_collection_us = 0;
    g_collector->frame_counter = 0;
    g_collector->input_event_counter = 0;
    
    g_collector->bandwidth_samples = NULL;
    g_collector->bandwidth_sample_count = 0;
    g_collector->bandwidth_window_us = 1000000ULL;  /* 1 second window */
    g_collector->total_rx_bytes = 0;
    g_collector->total_tx_bytes = 0;
    g_collector->bandwidth_last_update_us = 0;
    
    memset(&g_collector->metrics, 0, sizeof(g_collector->metrics));
    
    if (obs_config->metrics_file) {
        g_collector->metrics_file = strdup(obs_config->metrics_file);
        g_collector->metrics_fp = fopen(obs_config->metrics_file, "a");
        if (!g_collector->metrics_fp) {
            /* Non-fatal: metrics collection continues without file output */
        }
    } else {
        g_collector->metrics_file = NULL;
        g_collector->metrics_fp = NULL;
    }
    
    return 0;
}

void metrics_collector_cleanup(void) {
    if (!g_collector) {
        return;
    }
    
    if (g_collector->metrics_fp) {
        fclose(g_collector->metrics_fp);
    }
    
    /* Free bandwidth samples */
    struct bandwidth_sample *sample = g_collector->bandwidth_samples;
    while (sample) {
        struct bandwidth_sample *next = sample->next;
        free(sample);
        sample = next;
    }
    
    free(g_collector->metrics_file);
    free(g_collector);
    g_collector = NULL;
}

void metrics_record_frame(uint32_t latency_ms, bool dropped) {
    if (!g_collector || !g_collector->enabled) {
        return;
    }
    
    g_collector->frame_counter++;
    g_collector->metrics.frames_total++;
    
    if (dropped) {
        g_collector->metrics.frames_dropped++;
    }
    
    /* Update latency metrics */
    g_collector->metrics.frame_delay_ms = latency_ms;
    
    /* Calculate FPS (simplified) */
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t now_us = ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;
    
    if (g_collector->last_collection_us > 0) {
        uint64_t dt_us = now_us - g_collector->last_collection_us;
        if (dt_us > 0) {
            g_collector->metrics.frames_per_second = 1000000ULL / dt_us;
        }
    }
    
    g_collector->last_collection_us = now_us;
    g_collector->metrics.timestamp_us = now_us;
}

void metrics_record_input_event(bool predicted, bool reconciled) {
    if (!g_collector || !g_collector->enabled) {
        return;
    }
    
    g_collector->input_event_counter++;
    g_collector->metrics.input_events_total++;
    
    if (predicted) {
        g_collector->metrics.input_events_predicted++;
    }
    
    if (reconciled) {
        g_collector->metrics.input_events_reconciled++;
    }
}

void metrics_record_bandwidth(uint64_t rx_bytes, uint64_t tx_bytes) {
    if (!g_collector || !g_collector->enabled) {
        return;
    }
    
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t now_us = ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;
    
    /* Create new bandwidth sample */
    struct bandwidth_sample *sample = malloc(sizeof(struct bandwidth_sample));
    if (sample) {
        sample->timestamp_us = now_us;
        sample->rx_bytes = rx_bytes;
        sample->tx_bytes = tx_bytes;
        sample->next = g_collector->bandwidth_samples;
        g_collector->bandwidth_samples = sample;
        g_collector->bandwidth_sample_count++;
        
        g_collector->total_rx_bytes += rx_bytes;
        g_collector->total_tx_bytes += tx_bytes;
    }
    
    /* Remove samples outside the averaging window */
    uint64_t window_start_us = now_us - g_collector->bandwidth_window_us;
    struct bandwidth_sample **sample_ptr = &g_collector->bandwidth_samples;
    
    while (*sample_ptr) {
        if ((*sample_ptr)->timestamp_us < window_start_us) {
            struct bandwidth_sample *old = *sample_ptr;
            *sample_ptr = (*sample_ptr)->next;
            
            g_collector->total_rx_bytes -= old->rx_bytes;
            g_collector->total_tx_bytes -= old->tx_bytes;
            g_collector->bandwidth_sample_count--;
            free(old);
        } else {
            sample_ptr = &(*sample_ptr)->next;
        }
    }
    
    /* Calculate average bandwidth over the window */
    if (g_collector->bandwidth_sample_count > 0) {
        uint64_t window_duration_us = g_collector->bandwidth_window_us;
        if (window_duration_us > 0) {
            /* Convert bytes to bits per second */
            g_collector->metrics.bandwidth_rx_bps = 
                (g_collector->total_rx_bytes * 8 * 1000000ULL) / window_duration_us;
            g_collector->metrics.bandwidth_tx_bps = 
                (g_collector->total_tx_bytes * 8 * 1000000ULL) / window_duration_us;
        }
    }
    
    g_collector->bandwidth_last_update_us = now_us;
}

void metrics_record_latency(uint32_t end_to_end_ms, uint32_t input_lag_ms) {
    if (!g_collector || !g_collector->enabled) {
        return;
    }
    
    g_collector->metrics.end_to_end_latency_ms = end_to_end_ms;
    g_collector->metrics.input_lag_ms = input_lag_ms;
}

int metrics_collector_flush(void) {
    if (!g_collector || !g_collector->enabled || !g_collector->metrics_fp) {
        return 0;
    }
    
    /* Write metrics to file as JSON */
    fprintf(g_collector->metrics_fp,
            "{\"timestamp\":%llu,"
            "\"end_to_end_latency_ms\":%u,"
            "\"input_lag_ms\":%u,"
            "\"frame_delay_ms\":%u,"
            "\"frames_per_second\":%u,"
            "\"frames_dropped\":%u,"
            "\"frames_total\":%u,"
            "\"bandwidth_rx_bps\":%llu,"
            "\"bandwidth_tx_bps\":%llu,"
            "\"input_events_predicted\":%u,"
            "\"input_events_reconciled\":%u,"
            "\"input_events_total\":%u}\n",
            (unsigned long long)g_collector->metrics.timestamp_us,
            g_collector->metrics.end_to_end_latency_ms,
            g_collector->metrics.input_lag_ms,
            g_collector->metrics.frame_delay_ms,
            g_collector->metrics.frames_per_second,
            g_collector->metrics.frames_dropped,
            g_collector->metrics.frames_total,
            (unsigned long long)g_collector->metrics.bandwidth_rx_bps,
            (unsigned long long)g_collector->metrics.bandwidth_tx_bps,
            g_collector->metrics.input_events_predicted,
            g_collector->metrics.input_events_reconciled,
            g_collector->metrics.input_events_total);
    
    fflush(g_collector->metrics_fp);
    return 0;
}

const struct telescope_metrics *metrics_collector_get(void) {
    if (!g_collector || !g_collector->enabled) {
        return NULL;
    }
    
    return &g_collector->metrics;
}

