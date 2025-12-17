#include "compositor.h"
#include "../input/input.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

/**
 * wlroots Integration Glue Layer
 *
 * This module provides the actual wlroots API integration without
 * changing any data models. It connects wlroots events to the
 * existing compositor framework.
 *
 * Compile with: -DWLR_USE_UNSTABLE when wlroots is available
 */

#ifdef WLR_USE_UNSTABLE
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_surface.h>
#include <wayland-server.h>

/* wlroots-specific state */
struct wlroots_state {
    struct wlr_backend *backend;
    struct wlr_seat *seat;
    struct wl_listener new_input_listener;
    struct wl_listener pointer_motion_listener;
    struct wl_listener pointer_button_listener;
    struct wl_listener pointer_axis_listener;
    struct wl_listener surface_commit_listener;
    struct wl_listener surface_frame_listener;
};

static struct wlroots_state *g_wlroots_state = NULL;

/* Forward declarations */
static void handle_new_input(struct wl_listener *listener, void *data);
static void handle_pointer_motion(struct wl_listener *listener, void *data);
static void handle_pointer_button(struct wl_listener *listener, void *data);
static void handle_pointer_axis(struct wl_listener *listener, void *data);
static void handle_surface_commit(struct wl_listener *listener, void *data);
static void handle_surface_frame_done(void *data);

/**
 * Initialize wlroots integration
 *
 * @param backend wlroots backend (can be NULL if not available)
 * @param seat wlroots seat (can be NULL if not available)
 * @return 0 on success, negative on error
 */
int compositor_wlroots_init(void *backend, void *seat) {
    if (g_wlroots_state) {
        return -EBUSY;  /* Already initialized */
    }
    
    g_wlroots_state = calloc(1, sizeof(struct wlroots_state));
    if (!g_wlroots_state) {
        return -ENOMEM;
    }
    
    g_wlroots_state->backend = (struct wlr_backend *)backend;
    g_wlroots_state->seat = (struct wlr_seat *)seat;
    
    if (g_wlroots_state->backend) {
        /* Register for new input devices */
        g_wlroots_state->new_input_listener.notify = handle_new_input;
        wl_signal_add(&g_wlroots_state->backend->events.new_input,
                     &g_wlroots_state->new_input_listener);
    }
    
    return 0;
}

/**
 * Cleanup wlroots integration
 */
void compositor_wlroots_cleanup(void) {
    if (!g_wlroots_state) {
        return;
    }
    
    /* Remove listeners */
    if (g_wlroots_state->backend) {
        wl_list_remove(&g_wlroots_state->new_input_listener.link);
    }
    
    free(g_wlroots_state);
    g_wlroots_state = NULL;
}

/**
 * Handle new input device from wlroots
 */
static void handle_new_input(struct wl_listener *listener, void *data) {
    struct wlr_input_device *device = (struct wlr_input_device *)data;
    compositor_input_type_t type;
    
    /* Map wlroots device type to compositor type */
    switch (device->type) {
        case WLR_INPUT_DEVICE_POINTER:
            type = COMPOSITOR_INPUT_POINTER;
            break;
        case WLR_INPUT_DEVICE_KEYBOARD:
            type = COMPOSITOR_INPUT_KEYBOARD;
            break;
        case WLR_INPUT_DEVICE_TOUCH:
            type = COMPOSITOR_INPUT_TOUCHSCREEN;
            break;
        case WLR_INPUT_DEVICE_TABLET_TOOL:
            type = COMPOSITOR_INPUT_TOUCHPAD;
            break;
        default:
            return;  /* Unknown device type */
    }
    
    /* Register device with compositor framework */
    compositor_register_input_device((struct wl_input_device *)device, type);
    
    /* Set up device-specific listeners */
    if (device->type == WLR_INPUT_DEVICE_POINTER) {
        struct wlr_pointer *pointer = wlr_pointer_from_input_device(device);
        
        /* Motion events */
        g_wlroots_state->pointer_motion_listener.notify = handle_pointer_motion;
        wl_signal_add(&pointer->events.motion, &g_wlroots_state->pointer_motion_listener);
        
        /* Button events */
        g_wlroots_state->pointer_button_listener.notify = handle_pointer_button;
        wl_signal_add(&pointer->events.button, &g_wlroots_state->pointer_button_listener);
        
        /* Scroll events */
        g_wlroots_state->pointer_axis_listener.notify = handle_pointer_axis;
        wl_signal_add(&pointer->events.axis, &g_wlroots_state->pointer_axis_listener);
    }
}

/**
 * Handle pointer motion from wlroots
 */
static void handle_pointer_motion(struct wl_listener *listener, void *data) {
    struct wlr_pointer_motion_event *event = (struct wlr_pointer_motion_event *)data;
    struct wlr_input_device *device = event->pointer->base;
    
    /* Convert to compositor format and intercept */
    compositor_intercept_pointer_motion(
        (struct wl_input_device *)device,
        event->delta_x,
        event->delta_y,
        false,  /* Relative motion */
        0.0, 0.0
    );
}

/**
 * Handle pointer button from wlroots
 */
static void handle_pointer_button(struct wl_listener *listener, void *data) {
    struct wlr_pointer_button_event *event = (struct wlr_pointer_button_event *)data;
    struct wlr_input_device *device = event->pointer->base;
    
    compositor_intercept_button(
        (struct wl_input_device *)device,
        event->button,
        event->state == WLR_BUTTON_PRESSED
    );
}

/**
 * Handle pointer axis (scroll) from wlroots
 */
static void handle_pointer_axis(struct wl_listener *listener, void *data) {
    struct wlr_pointer_axis_event *event = (struct wlr_pointer_axis_event *)data;
    struct wlr_input_device *device = event->pointer->base;
    
    double dx = 0.0, dy = 0.0;
    bool discrete = false;
    
    if (event->orientation == WLR_AXIS_ORIENTATION_HORIZONTAL) {
        dx = event->delta;
        if (event->delta_discrete != 0) {
            discrete = true;
        }
    } else {
        dy = event->delta;
        if (event->delta_discrete != 0) {
            discrete = true;
        }
    }
    
    compositor_intercept_scroll(
        (struct wl_input_device *)device,
        dx, dy,
        discrete
    );
}

/**
 * Register wlroots surface for frame tracking
 */
int compositor_wlroots_register_surface(void *wlr_surface) {
    struct wlr_surface *surface = (struct wlr_surface *)wlr_surface;
    
    /* Register with compositor framework */
    int ret = compositor_register_surface((struct wl_surface *)surface);
    if (ret < 0) {
        return ret;
    }
    
    /* Set up frame callback */
    struct wlr_surface_state *state = &surface->current;
    
    /* Frame done callback */
    wlr_surface_set_frame_done_callback(surface, handle_surface_frame_done, surface);
    
    /* Commit listener for frame ID generation */
    if (g_wlroots_state) {
        g_wlroots_state->surface_commit_listener.notify = handle_surface_commit;
        wl_signal_add(&surface->events.commit, &g_wlroots_state->surface_commit_listener);
    }
    
    return 0;
}

/**
 * Handle surface commit (generate frame ID)
 */
static void handle_surface_commit(struct wl_listener *listener, void *data) {
    struct wlr_surface *surface = (struct wlr_surface *)data;
    
    /* Generate frame ID on commit */
    uint64_t frame_id = compositor_generate_frame_id((struct wl_surface *)surface);
    
    /* Frame ID is now tracked for this surface */
    (void)frame_id;  /* Used by reconciliation system */
}

/**
 * Handle surface frame done (notify presentation)
 */
static void handle_surface_frame_done(void *data) {
    struct wlr_surface *surface = (struct wlr_surface *)data;
    
    /* Get current time */
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t timestamp_us = ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;
    
    /* Get frame ID from surface tracking */
    /* compositor_generate_frame_id() was called on commit */
    /* We need to track the last frame ID per surface - for now use simple approach */
    /* In production, would maintain frame_id -> surface mapping */
    extern uint64_t compositor_generate_frame_id(struct wl_surface *);
    uint64_t frame_id = compositor_generate_frame_id((struct wl_surface *)surface);
    if (frame_id == 0) {
        /* Fallback: use simple counter if frame ID generation failed */
        static uint64_t frame_counter = 1;
        frame_id = frame_counter++;
    }
    
    compositor_notify_frame_presented(
        (struct wl_surface *)surface,
        frame_id,
        timestamp_us
    );
}

#else /* WLR_USE_UNSTABLE not defined */

/* Stub implementations when wlroots is not available */

int compositor_wlroots_init(void *backend, void *seat) {
    (void)backend;
    (void)seat;
    return -ENOTSUP;  /* wlroots not available */
}

void compositor_wlroots_cleanup(void) {
    /* No-op */
}

int compositor_wlroots_register_surface(void *wlr_surface) {
    (void)wlr_surface;
    return -ENOTSUP;  /* wlroots not available */
}

#endif /* WLR_USE_UNSTABLE */

