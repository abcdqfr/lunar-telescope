#define _POSIX_C_SOURCE 200809L
#include "input.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/**
 * Scroll smoothing implementation
 *
 * Applies smoothing to scroll events to reduce jitter and improve
 * perceived smoothness, especially for touchpad scrolling.
 */

struct scroll_state {
    /* Smoothing parameters */
    double smoothing_factor;
    double velocity_decay;
    
    /* State */
    double velocity_x;
    double velocity_y;
    double position_x;
    double position_y;
    uint64_t last_update_us;
    
    /* Discrete scroll accumulator */
    int32_t discrete_accum_x;
    int32_t discrete_accum_y;
};

int scroll_smoother_create(struct scroll_state **smoother_out) {
    if (!smoother_out) {
        return -1;
    }
    
    struct scroll_state *smoother = calloc(1, sizeof(struct scroll_state));
    if (!smoother) {
        return -1;
    }
    
    /* Default smoothing parameters */
    smoother->smoothing_factor = 0.7;  /* 0.0 = no smoothing, 1.0 = maximum smoothing */
    smoother->velocity_decay = 0.9;    /* Velocity decay factor */
    
    smoother->velocity_x = 0.0;
    smoother->velocity_y = 0.0;
    smoother->position_x = 0.0;
    smoother->position_y = 0.0;
    
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    smoother->last_update_us = ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;
    
    smoother->discrete_accum_x = 0;
    smoother->discrete_accum_y = 0;
    
    *smoother_out = smoother;
    return 0;
}

void scroll_smoother_destroy(struct scroll_state *smoother) {
    if (smoother) {
        free(smoother);
    }
}

int scroll_smoother_process(struct scroll_state *smoother,
                           double dx, double dy,
                           bool discrete,
                           double *smoothed_dx, double *smoothed_dy) {
    if (!smoother || !smoothed_dx || !smoothed_dy) {
        return -1;
    }
    
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t now_us = ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;
    
    double dt = (now_us - smoother->last_update_us) / 1000000.0;  /* Convert to seconds */
    if (dt <= 0.0) {
        dt = 0.001;  /* Minimum 1ms */
    }
    
    if (discrete) {
        /* For discrete scroll, accumulate and convert to smooth */
        smoother->discrete_accum_x += (int32_t)dx;
        smoother->discrete_accum_y += (int32_t)dy;
        
        /* Convert discrete to smooth with interpolation */
        double smooth_dx = (double)smoother->discrete_accum_x * 0.1;
        double smooth_dy = (double)smoother->discrete_accum_y * 0.1;
        
        smoother->discrete_accum_x = 0;
        smoother->discrete_accum_y = 0;
        
        dx = smooth_dx;
        dy = smooth_dy;
    }
    
    /* Update velocity */
    double new_velocity_x = dx / dt;
    double new_velocity_y = dy / dt;
    
    /* Apply exponential smoothing to velocity */
    smoother->velocity_x = smoother->smoothing_factor * smoother->velocity_x +
                          (1.0 - smoother->smoothing_factor) * new_velocity_x;
    smoother->velocity_y = smoother->smoothing_factor * smoother->velocity_y +
                          (1.0 - smoother->smoothing_factor) * new_velocity_y;
    
    /* Apply velocity decay */
    smoother->velocity_x *= smoother->velocity_decay;
    smoother->velocity_y *= smoother->velocity_decay;
    
    /* Calculate smoothed output */
    *smoothed_dx = smoother->velocity_x * dt;
    *smoothed_dy = smoother->velocity_y * dt;
    
    /* Update position */
    smoother->position_x += *smoothed_dx;
    smoother->position_y += *smoothed_dy;
    
    smoother->last_update_us = now_us;
    
    return 0;
}

