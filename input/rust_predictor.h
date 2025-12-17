#ifndef RUST_PREDICTOR_H
#define RUST_PREDICTOR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Rust Input Predictor C API
 *
 * High-performance input prediction implemented in Rust,
 * exposed via C ABI for integration with C input proxy.
 */

/* Forward declaration */
typedef void rust_input_predictor_t;

/**
 * Create input predictor instance
 *
 * @param window_ms Prediction window in milliseconds
 * @param smoothing_factor Smoothing factor (0.0-1.0)
 * @param velocity_decay Velocity decay factor (0.0-1.0)
 * @return Predictor handle, or NULL on error
 */
rust_input_predictor_t *rust_input_predictor_create(uint32_t window_ms,
                                                    double smoothing_factor,
                                                    double velocity_decay);

/**
 * Destroy input predictor instance
 *
 * @param predictor Predictor handle
 */
void rust_input_predictor_destroy(rust_input_predictor_t *predictor);

/**
 * Predict pointer motion
 *
 * @param predictor Predictor handle
 * @param timestamp Timestamp in seconds (monotonic)
 * @param dx Delta X
 * @param dy Delta Y
 * @param predicted_dx Output predicted delta X (can be same as input)
 * @param predicted_dy Output predicted delta Y (can be same as input)
 * @return 0 on success, negative on error
 */
int rust_input_predictor_predict_pointer(rust_input_predictor_t *predictor,
                                        double timestamp,
                                        double dx, double dy,
                                        double *predicted_dx,
                                        double *predicted_dy);

/**
 * Predict scroll motion
 *
 * @param predictor Predictor handle
 * @param timestamp Timestamp in seconds (monotonic)
 * @param dx Delta X
 * @param dy Delta Y
 * @param predicted_dx Output predicted delta X
 * @param predicted_dy Output predicted delta Y
 * @return 0 on success, negative on error
 */
int rust_input_predictor_predict_scroll(rust_input_predictor_t *predictor,
                                       double timestamp,
                                       double dx, double dy,
                                       double *predicted_dx,
                                       double *predicted_dy);

/**
 * Reset predictor state
 *
 * @param predictor Predictor handle
 * @return 0 on success, negative on error
 */
int rust_input_predictor_reset(rust_input_predictor_t *predictor);

#ifdef __cplusplus
}
#endif

#endif /* RUST_PREDICTOR_H */

