#ifndef COMPOSITOR_H
#define COMPOSITOR_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Compositor Integration API
 *
 * Provides hooks for Wayland compositor integration (wlroots-based).
 * These are stubs that will be implemented when compositor integration
 * is available.
 */

/* Forward declarations */
struct wl_input_device;
struct wl_surface;
struct wl_seat;

/**
 * Input device types
 */
typedef enum {
    COMPOSITOR_INPUT_POINTER,
    COMPOSITOR_INPUT_KEYBOARD,
    COMPOSITOR_INPUT_TOUCHPAD,
    COMPOSITOR_INPUT_TOUCHSCREEN
} compositor_input_type_t;

/**
 * Initialize compositor hooks
 *
 * @return 0 on success, negative error code on failure
 */
int compositor_hooks_init(void);

/**
 * Cleanup compositor hooks
 */
void compositor_hooks_cleanup(void);

/**
 * Register input device for interception
 *
 * @param device Input device handle
 * @param type Device type
 * @return 0 on success, negative error code on failure
 */
int compositor_register_input_device(struct wl_input_device *device,
                                     compositor_input_type_t type);

/**
 * Unregister input device
 *
 * @param device Input device handle
 */
void compositor_unregister_input_device(struct wl_input_device *device);

/**
 * Intercept pointer motion event
 *
 * @param device Input device
 * @param dx Delta X
 * @param dy Delta Y
 * @param absolute Whether this is absolute positioning
 * @param x Absolute X (if absolute)
 * @param y Absolute Y (if absolute)
 * @return 0 if event should be processed, negative to drop
 */
int compositor_intercept_pointer_motion(struct wl_input_device *device,
                                       double dx, double dy,
                                       bool absolute,
                                       double x, double y);

/**
 * Intercept scroll event
 *
 * @param device Input device
 * @param dx Scroll delta X
 * @param dy Scroll delta Y
 * @param discrete Whether this is a discrete scroll event
 * @return 0 if event should be processed, negative to drop
 */
int compositor_intercept_scroll(struct wl_input_device *device,
                               double dx, double dy,
                               bool discrete);

/**
 * Intercept button event
 *
 * @param device Input device
 * @param button Button number
 * @param pressed Whether button is pressed
 * @return 0 if event should be processed, negative to drop
 */
int compositor_intercept_button(struct wl_input_device *device,
                               uint32_t button,
                               bool pressed);

/**
 * Register surface for frame tracking
 *
 * @param surface Wayland surface
 * @return 0 on success, negative error code on failure
 */
int compositor_register_surface(struct wl_surface *surface);

/**
 * Unregister surface
 *
 * @param surface Wayland surface
 */
void compositor_unregister_surface(struct wl_surface *surface);

/**
 * Notify frame presentation
 *
 * @param surface Wayland surface
 * @param frame_id Frame identifier
 * @param timestamp_us Presentation timestamp in microseconds
 * @return 0 on success, negative error code on failure
 */
int compositor_notify_frame_presented(struct wl_surface *surface,
                                     uint64_t frame_id,
                                     uint64_t timestamp_us);

/**
 * Generate and track new frame ID for surface
 *
 * @param surface Wayland surface
 * @return Frame ID (0 on error)
 */
uint64_t compositor_generate_frame_id(struct wl_surface *surface);

/**
 * wlroots Integration Functions
 *
 * These functions provide the glue layer to wlroots without
 * changing the data model. Compile with -DWLR_USE_UNSTABLE
 * when wlroots is available.
 */

/**
 * Initialize wlroots integration
 *
 * @param backend wlroots backend (can be NULL)
 * @param seat wlroots seat (can be NULL)
 * @return 0 on success, negative on error
 */
int compositor_wlroots_init(void *backend, void *seat);

/**
 * Cleanup wlroots integration
 */
void compositor_wlroots_cleanup(void);

/**
 * Register wlroots surface for frame tracking
 *
 * @param wlr_surface wlroots surface pointer
 * @return 0 on success, negative on error
 */
int compositor_wlroots_register_surface(void *wlr_surface);

#ifdef __cplusplus
}
#endif

#endif /* COMPOSITOR_H */

