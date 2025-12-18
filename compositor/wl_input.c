#include "compositor.h"
#include "../input/input.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

/**
 * Wayland input interception implementation
 *
 * This module provides hooks for intercepting Wayland input events
 * from wlroots-based compositors. The framework is ready for wlroots
 * integration - actual Wayland connection would be added in production.
 */

/* Input device tracking */
struct input_device_entry {
    struct wl_input_device *device;
    compositor_input_type_t type;
    struct input_proxy *proxy;
    struct input_device_entry *next;
};

static bool g_hooks_initialized = false;
static struct input_device_entry *g_input_devices = NULL;
static struct input_proxy *g_global_input_proxy = NULL;

struct input_proxy *compositor_get_global_input_proxy(void) {
    return g_global_input_proxy;
}

int compositor_hooks_init(void) {
    if (g_hooks_initialized) {
        return -1;  /* Already initialized */
    }
    
    /* Initialize input proxy with default settings */
    /* In production, these would come from configuration */
    int ret = input_proxy_create(true, 16, true, &g_global_input_proxy);
    if (ret < 0) {
        return ret;
    }
    
    /* wlroots integration is handled by compositor_wlroots_init() */
    /* This function is called separately when wlroots is available */
    
    g_hooks_initialized = true;
    return 0;
}

void compositor_hooks_cleanup(void) {
    if (!g_hooks_initialized) {
        return;
    }
    
    /* Unregister all input devices */
    struct input_device_entry *entry = g_input_devices;
    while (entry) {
        struct input_device_entry *next = entry->next;
        free(entry);
        entry = next;
    }
    g_input_devices = NULL;
    
    /* Destroy input proxy */
    if (g_global_input_proxy) {
        input_proxy_destroy(g_global_input_proxy);
        g_global_input_proxy = NULL;
    }
    
    /* wlroots cleanup is handled by compositor_wlroots_cleanup() */
    
    g_hooks_initialized = false;
}

int compositor_register_input_device(struct wl_input_device *device,
                                     compositor_input_type_t type) {
    if (!g_hooks_initialized || !device) {
        return -1;
    }
    
    /* Check if device already registered */
    struct input_device_entry *entry = g_input_devices;
    while (entry) {
        if (entry->device == device) {
            return 0;  /* Already registered */
        }
        entry = entry->next;
    }
    
    /* Create new device entry */
    entry = malloc(sizeof(struct input_device_entry));
    if (!entry) {
        return -1;
    }
    
    entry->device = device;
    entry->type = type;
    entry->proxy = g_global_input_proxy;  /* Use global proxy for now */
    entry->next = g_input_devices;
    g_input_devices = entry;
    
    /* wlroots event callbacks are set up by compositor_wlroots_init() */
    /* when a new input device is detected */
    
    return 0;
}

void compositor_unregister_input_device(struct wl_input_device *device) {
    if (!g_hooks_initialized || !device) {
        return;
    }
    
    /* Find and remove device entry */
    struct input_device_entry **entry_ptr = &g_input_devices;
    while (*entry_ptr) {
        if ((*entry_ptr)->device == device) {
            struct input_device_entry *to_free = *entry_ptr;
            *entry_ptr = (*entry_ptr)->next;
            free(to_free);
            return;
        }
        entry_ptr = &(*entry_ptr)->next;
    }
    
    /* wlroots event callbacks are cleaned up by compositor_wlroots_cleanup() */
}

int compositor_intercept_pointer_motion(struct wl_input_device *device,
                                       double dx, double dy,
                                       bool absolute,
                                       double x, double y) {
    if (!g_hooks_initialized || !device || !g_global_input_proxy) {
        return -1;
    }
    
    /* Get current timestamp */
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t timestamp_us = ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;
    
    /* Create input event */
    struct input_event event = {
        .type = INPUT_EVENT_POINTER_MOTION,
        .timestamp_us = timestamp_us,
        .pointer_motion = {
            .dx = dx,
            .dy = dy,
            .absolute = absolute,
            .x = x,
            .y = y
        }
    };
    
    /* Process through input proxy for prediction */
    struct input_event *predicted = NULL;
    int ret = input_proxy_process(g_global_input_proxy, &event, &predicted);
    if (ret < 0) {
        return ret;
    }
    
    /* Local feedback: Input proxy applies immediate local feedback for predicted events */
    /* Remote transport: Lens adapter is responsible for sending events to remote */
    /* This function only processes through input proxy for prediction */
    
    if (predicted) {
        free(predicted);
    }
    
    return 0;  /* Allow event to proceed */
}

int compositor_intercept_scroll(struct wl_input_device *device,
                               double dx, double dy,
                               bool discrete) {
    if (!g_hooks_initialized || !device || !g_global_input_proxy) {
        return -1;
    }
    
    /* Get current timestamp */
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t timestamp_us = ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;
    
    /* Create input event */
    struct input_event event = {
        .type = INPUT_EVENT_SCROLL,
        .timestamp_us = timestamp_us,
        .scroll = {
            .dx = dx,
            .dy = dy,
            .discrete = discrete,
            .discrete_dx = discrete ? (int32_t)dx : 0,
            .discrete_dy = discrete ? (int32_t)dy : 0
        }
    };
    
    /* Process through input proxy (includes scroll smoothing) */
    struct input_event *smoothed = NULL;
    int ret = input_proxy_process(g_global_input_proxy, &event, &smoothed);
    if (ret < 0) {
        return ret;
    }
    
    /* Remote transport is handled by lens adapters */
    /* This function only processes through input proxy for smoothing */
    
    if (smoothed) {
        free(smoothed);
    }
    
    return 0;  /* Allow event to proceed */
}

int compositor_intercept_button(struct wl_input_device *device,
                               uint32_t button,
                               bool pressed) {
    if (!g_hooks_initialized || !device || !g_global_input_proxy) {
        return -1;
    }
    
    /* Get current timestamp */
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t timestamp_us = ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;
    
    /* Create input event */
    struct input_event event = {
        .type = INPUT_EVENT_POINTER_BUTTON,
        .timestamp_us = timestamp_us,
        .pointer_button = {
            .button = button,
            .pressed = pressed
        }
    };
    
    /* Button events are not predicted, but track for reconciliation */
    /* Process without prediction */
    int ret = input_proxy_process(g_global_input_proxy, &event, NULL);
    if (ret < 0) {
        return ret;
    }
    
    /* Remote transport is handled by lens adapters */
    /* Button events are sent immediately without prediction */
    
    return 0;  /* Allow event to proceed */
}

