#include "rust_predictor.h"
#include <stdlib.h>

/**
 * Rust predictor stub implementation
 *
 * This provides fallback stubs when the Rust library is not linked.
 * In production, these would be replaced by actual Rust function calls
 * via dynamic linking or static linking of the Rust library.
 */

rust_input_predictor_t *rust_input_predictor_create(uint32_t window_ms,
                                                    double smoothing_factor,
                                                    double velocity_decay) {
    /* Stub: return NULL to indicate Rust predictor not available */
    /* In production, this would call the actual Rust function */
    (void)window_ms;
    (void)smoothing_factor;
    (void)velocity_decay;
    return NULL;
}

void rust_input_predictor_destroy(rust_input_predictor_t *predictor) {
    (void)predictor;
    /* Stub: no-op */
}

int rust_input_predictor_predict_pointer(rust_input_predictor_t *predictor,
                                        double timestamp,
                                        double dx, double dy,
                                        double *predicted_dx,
                                        double *predicted_dy) {
    (void)predictor;
    (void)timestamp;
    
    if (!predicted_dx || !predicted_dy) {
        return -1;
    }
    
    /* Stub: return input unchanged */
    *predicted_dx = dx;
    *predicted_dy = dy;
    return -1;  /* Indicate stub/failure */
}

int rust_input_predictor_predict_scroll(rust_input_predictor_t *predictor,
                                       double timestamp,
                                       double dx, double dy,
                                       double *predicted_dx,
                                       double *predicted_dy) {
    (void)predictor;
    (void)timestamp;
    
    if (!predicted_dx || !predicted_dy) {
        return -1;
    }
    
    /* Stub: return input unchanged */
    *predicted_dx = dx;
    *predicted_dy = dy;
    return -1;  /* Indicate stub/failure */
}

int rust_input_predictor_reset(rust_input_predictor_t *predictor) {
    (void)predictor;
    return -1;  /* Indicate stub/failure */
}

