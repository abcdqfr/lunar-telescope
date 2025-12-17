#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Predictive Input API
 *
 * Provides predictive input processing for pointer motion and scroll events
 * to reduce perceived latency. Prediction is local, reversible, and
 * reconciled on frame acknowledgment.
 */

/* Forward declarations */
struct input_proxy;
struct input_event;
struct scroll_state;

/**
 * Input event types
 */
typedef enum {
    INPUT_EVENT_POINTER_MOTION,
    INPUT_EVENT_POINTER_BUTTON,
    INPUT_EVENT_SCROLL,
    INPUT_EVENT_KEY,
    INPUT_EVENT_TOUCH
} input_event_type_t;

/**
 * Input event structure
 */
struct input_event {
    input_event_type_t type;
    uint64_t timestamp_us;
    
    union {
        struct {
            double dx, dy;
            bool absolute;
            double x, y;  /* For absolute positioning */
        } pointer_motion;
        
        struct {
            uint32_t button;
            bool pressed;
        } pointer_button;
        
        struct {
            double dx, dy;
            bool discrete;
            int32_t discrete_dx, discrete_dy;
        } scroll;
        
        struct {
            uint32_t key;
            bool pressed;
        } key;
        
        struct {
            uint32_t touch_id;
            double x, y;
            bool pressed;
        } touch;
    };
};

/**
 * Prediction state
 */
typedef struct {
    bool enabled;
    uint32_t window_ms;
    uint64_t last_prediction_us;
    uint32_t events_predicted;
    uint32_t events_reconciled;
} prediction_state_t;

/**
 * Create input proxy
 *
 * @param enable_prediction Enable predictive input
 * @param prediction_window_ms Prediction window in milliseconds
 * @param enable_scroll_smoothing Enable scroll smoothing
 * @param proxy_out Output proxy handle
 * @return 0 on success, negative error code on failure
 */
int input_proxy_create(bool enable_prediction,
                      uint32_t prediction_window_ms,
                      bool enable_scroll_smoothing,
                      struct input_proxy **proxy_out);

/**
 * Destroy input proxy
 */
void input_proxy_destroy(struct input_proxy *proxy);

/**
 * Process input event (with prediction if enabled)
 *
 * @param proxy Input proxy handle
 * @param event Input event
 * @param predicted_out Output predicted event (if prediction applied)
 * @return 0 on success, negative error code on failure
 */
int input_proxy_process(struct input_proxy *proxy,
                       const struct input_event *event,
                       struct input_event **predicted_out);

/**
 * Reconcile predicted input with server acknowledgment
 *
 * @param proxy Input proxy handle
 * @param frame_id Frame ID that was acknowledged
 * @param actual_event Actual event that was processed (NULL if prediction was correct)
 * @return 0 on success, negative error code on failure
 */
int input_proxy_reconcile(struct input_proxy *proxy,
                         uint64_t frame_id,
                         const struct input_event *actual_event);

/**
 * Get prediction statistics
 *
 * @param proxy Input proxy handle
 * @param state_out Output prediction state
 * @return 0 on success, negative error code on failure
 */
int input_proxy_get_prediction_state(const struct input_proxy *proxy,
                                    prediction_state_t *state_out);

/**
 * Scroll smoothing functions
 */

/**
 * Create scroll smoother
 *
 * @param smoother_out Output smoother handle
 * @return 0 on success, negative error code on failure
 */
int scroll_smoother_create(struct scroll_state **smoother_out);

/**
 * Destroy scroll smoother
 */
void scroll_smoother_destroy(struct scroll_state *smoother);

/**
 * Smooth scroll event
 *
 * @param smoother Scroll smoother handle
 * @param dx Raw scroll delta X
 * @param dy Raw scroll delta Y
 * @param discrete Whether this is a discrete scroll event
 * @param smoothed_dx Output smoothed delta X
 * @param smoothed_dy Output smoothed delta Y
 * @return 0 on success, negative error code on failure
 */
int scroll_smoother_process(struct scroll_state *smoother,
                           double dx, double dy,
                           bool discrete,
                           double *smoothed_dx, double *smoothed_dy);

#ifdef __cplusplus
}
#endif

#endif /* INPUT_H */

