#include "compositor.h"
#include "../input/input.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

/**
 * Wayland surface tracking implementation
 *
 * This module tracks Wayland surfaces and frame presentation events
 * for latency measurement and input reconciliation.
 */

/* Surface tracking entry */
struct surface_entry {
    struct wl_surface *surface;
    uint64_t frame_id_counter;
    uint64_t *frame_timestamps;  /* Map frame_id to creation timestamp */
    size_t frame_capacity;
    size_t frame_count;
    struct surface_entry *next;
};

static bool g_surfaces_initialized = false;
static struct surface_entry *g_surfaces = NULL;

int compositor_register_surface(struct wl_surface *surface) {
    if (!surface) {
        return -1;
    }
    
    if (!g_surfaces_initialized) {
        g_surfaces_initialized = true;
    }
    
    /* Check if surface already registered */
    struct surface_entry *entry = g_surfaces;
    while (entry) {
        if (entry->surface == surface) {
            return 0;  /* Already registered */
        }
        entry = entry->next;
    }
    
    /* Create new surface entry */
    entry = calloc(1, sizeof(struct surface_entry));
    if (!entry) {
        return -1;
    }
    
    entry->surface = surface;
    entry->frame_id_counter = 0;
    entry->frame_capacity = 64;  /* Initial capacity */
    entry->frame_count = 0;
    entry->frame_timestamps = calloc(entry->frame_capacity, sizeof(uint64_t));
    if (!entry->frame_timestamps) {
        free(entry);
        return -1;
    }
    
    entry->next = g_surfaces;
    g_surfaces = entry;
    
    /* wlroots frame callbacks are set up by compositor_wlroots_register_surface() */
    
    return 0;
}

void compositor_unregister_surface(struct wl_surface *surface) {
    if (!surface) {
        return;
    }
    
    /* Find and remove surface entry */
    struct surface_entry **entry_ptr = &g_surfaces;
    while (*entry_ptr) {
        if ((*entry_ptr)->surface == surface) {
            struct surface_entry *to_free = *entry_ptr;
            *entry_ptr = (*entry_ptr)->next;
            
            free(to_free->frame_timestamps);
            free(to_free);
            return;
        }
        entry_ptr = &(*entry_ptr)->next;
    }
    
    /* wlroots frame callbacks are cleaned up by compositor_wlroots_cleanup() */
}

/* Find surface entry */
static struct surface_entry *find_surface_entry(struct wl_surface *surface) {
    struct surface_entry *entry = g_surfaces;
    while (entry) {
        if (entry->surface == surface) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

int compositor_notify_frame_presented(struct wl_surface *surface,
                                     uint64_t frame_id,
                                     uint64_t timestamp_us) {
    if (!surface) {
        return -1;
    }
    
    struct surface_entry *entry = find_surface_entry(surface);
    if (!entry) {
        return -1;  /* Surface not registered */
    }
    
    /* Calculate latency */
    uint32_t latency_ms = 0;
    bool dropped = false;
    
    /* Look up frame creation timestamp */
    if (frame_id < entry->frame_capacity && entry->frame_timestamps[frame_id] > 0) {
        uint64_t frame_created_us = entry->frame_timestamps[frame_id];
        uint64_t latency_us = timestamp_us - frame_created_us;
        latency_ms = latency_us / 1000;
        
        /* Clear timestamp (frame processed) */
        entry->frame_timestamps[frame_id] = 0;
    } else {
        /* Frame not found in tracking - might be dropped or old */
        dropped = true;
    }
    
    /* Forward to metrics collector */
    extern void metrics_record_frame(uint32_t latency_ms, bool dropped);
    metrics_record_frame(latency_ms, dropped);
    
    /* Trigger input reconciliation for this frame */
    struct input_proxy *proxy = compositor_get_global_input_proxy();
    if (proxy) {
        input_proxy_reconcile(proxy, frame_id, NULL);
    }
    
    return 0;
}

/* Generate and track new frame ID */
uint64_t compositor_generate_frame_id(struct wl_surface *surface) {
    struct surface_entry *entry = find_surface_entry(surface);
    if (!entry) {
        return 0;
    }
    
    uint64_t frame_id = ++entry->frame_id_counter;
    
    /* Get current timestamp */
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t timestamp_us = ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;
    
    /* Expand frame_timestamps array if needed */
    if (frame_id >= entry->frame_capacity) {
        size_t new_capacity = entry->frame_capacity * 2;
        uint64_t *new_timestamps = realloc(entry->frame_timestamps,
                                          new_capacity * sizeof(uint64_t));
        if (new_timestamps) {
            /* Zero out new entries */
            memset(new_timestamps + entry->frame_capacity, 0,
                   (new_capacity - entry->frame_capacity) * sizeof(uint64_t));
            entry->frame_timestamps = new_timestamps;
            entry->frame_capacity = new_capacity;
        }
    }
    
    /* Store frame creation timestamp */
    if (frame_id < entry->frame_capacity) {
        entry->frame_timestamps[frame_id] = timestamp_us;
        entry->frame_count++;
    }
    
    return frame_id;
}

