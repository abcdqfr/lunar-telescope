#include "input.h"
#include "rust_predictor.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

/**
 * Input proxy implementation with predictive input
 */

/* Pending predicted event with frame ID */
struct pending_prediction {
    uint64_t frame_id;
    struct input_event *predicted_event;
    uint64_t timestamp_us;
    struct pending_prediction *next;
};

struct input_proxy {
    bool enable_prediction;
    uint32_t prediction_window_ms;
    bool enable_scroll_smoothing;
    struct scroll_state *scroll_smoother;
    prediction_state_t prediction_state;
    uint64_t frame_counter;
    
    /* Reconciliation tracking */
    struct pending_prediction *pending_predictions;
    size_t pending_count;
    uint64_t next_frame_id;
    
    /* Rust predictor (if available) */
    rust_input_predictor_t *rust_predictor;
    bool use_rust_predictor;
};

int input_proxy_create(bool enable_prediction,
                      uint32_t prediction_window_ms,
                      bool enable_scroll_smoothing,
                      struct input_proxy **proxy_out) {
    if (!proxy_out) {
        return -1;
    }
    
    struct input_proxy *proxy = calloc(1, sizeof(struct input_proxy));
    if (!proxy) {
        return -1;
    }
    
    proxy->enable_prediction = enable_prediction;
    proxy->prediction_window_ms = prediction_window_ms;
    proxy->enable_scroll_smoothing = enable_scroll_smoothing;
    proxy->frame_counter = 0;
    
    proxy->prediction_state.enabled = enable_prediction;
    proxy->prediction_state.window_ms = prediction_window_ms;
    proxy->prediction_state.last_prediction_us = 0;
    proxy->prediction_state.events_predicted = 0;
    proxy->prediction_state.events_reconciled = 0;
    
    proxy->pending_predictions = NULL;
    proxy->pending_count = 0;
    proxy->next_frame_id = 1;
    
    /* Try to initialize Rust predictor */
    proxy->rust_predictor = NULL;
    proxy->use_rust_predictor = false;
    
    if (enable_prediction) {
        /* Try to create Rust predictor with default parameters */
        proxy->rust_predictor = rust_input_predictor_create(
            prediction_window_ms,
            0.7,  /* smoothing_factor */
            0.9   /* velocity_decay */
        );
        
        if (proxy->rust_predictor) {
            proxy->use_rust_predictor = true;
        }
    }
    
    if (enable_scroll_smoothing) {
        if (scroll_smoother_create(&proxy->scroll_smoother) < 0) {
            free(proxy);
            return -1;
        }
    } else {
        proxy->scroll_smoother = NULL;
    }
    
    *proxy_out = proxy;
    return 0;
}

void input_proxy_destroy(struct input_proxy *proxy) {
    if (!proxy) {
        return;
    }
    
    if (proxy->scroll_smoother) {
        scroll_smoother_destroy(proxy->scroll_smoother);
    }
    
    /* Free pending predictions */
    struct pending_prediction *pred = proxy->pending_predictions;
    while (pred) {
        struct pending_prediction *next = pred->next;
        if (pred->predicted_event) {
            free(pred->predicted_event);
        }
        free(pred);
        pred = next;
    }
    
    /* Destroy Rust predictor */
    if (proxy->rust_predictor) {
        rust_input_predictor_destroy(proxy->rust_predictor);
    }
    
    free(proxy);
}

int input_proxy_process(struct input_proxy *proxy,
                       const struct input_event *event,
                       struct input_event **predicted_out) {
    if (!proxy || !event) {
        return -1;
    }
    
    if (predicted_out) {
        *predicted_out = NULL;
    }
    
    /* Get current timestamp */
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t now_us = ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;
    
    /* Handle scroll smoothing */
    if (event->type == INPUT_EVENT_SCROLL && proxy->enable_scroll_smoothing && proxy->scroll_smoother) {
        double smoothed_dx, smoothed_dy;
        if (scroll_smoother_process(proxy->scroll_smoother,
                                    event->scroll.dx, event->scroll.dy,
                                    event->scroll.discrete,
                                    &smoothed_dx, &smoothed_dy) == 0) {
            /* Only allocate an output event if the caller requested one. */
            if (predicted_out) {
                struct input_event *smoothed = malloc(sizeof(struct input_event));
                if (smoothed) {
                    *smoothed = *event;
                    smoothed->scroll.dx = smoothed_dx;
                    smoothed->scroll.dy = smoothed_dy;
                    *predicted_out = smoothed;
                }
            }
        }
    }
    
    /* Handle prediction for pointer motion */
    if (event->type == INPUT_EVENT_POINTER_MOTION && proxy->enable_prediction) {
        uint64_t prediction_window_us = proxy->prediction_window_ms * 1000ULL;
        
        /* Create predicted event */
        struct input_event *predicted = malloc(sizeof(struct input_event));
        if (predicted) {
            *predicted = *event;
            predicted->timestamp_us = now_us + prediction_window_us;
            
            double predicted_dx = event->pointer_motion.dx;
            double predicted_dy = event->pointer_motion.dy;
            
            /* Use Rust predictor if available, otherwise fallback to simple prediction */
            if (proxy->use_rust_predictor && proxy->rust_predictor) {
                /* Convert timestamp to seconds for Rust */
                double timestamp_sec = now_us / 1000000.0;
                
                int ret = rust_input_predictor_predict_pointer(
                    proxy->rust_predictor,
                    timestamp_sec,
                    event->pointer_motion.dx,
                    event->pointer_motion.dy,
                    &predicted_dx,
                    &predicted_dy
                );
                
                if (ret < 0) {
                    /* Fallback to simple prediction if Rust predictor fails */
                    predicted_dx = event->pointer_motion.dx * 1.1;
                    predicted_dy = event->pointer_motion.dy * 1.1;
                }
            } else {
                /* Simple extrapolation: assume constant velocity */
                predicted_dx = event->pointer_motion.dx * 1.1;
                predicted_dy = event->pointer_motion.dy * 1.1;
            }
            
            predicted->pointer_motion.dx = predicted_dx;
            predicted->pointer_motion.dy = predicted_dy;
            
            /* Assign frame ID for tracking */
            uint64_t frame_id = proxy->next_frame_id++;
            
            /* Track prediction for reconciliation */
            struct pending_prediction *pending = malloc(sizeof(struct pending_prediction));
            if (pending) {
                pending->frame_id = frame_id;
                /*
                 * Ownership rule:
                 * - If caller requested predicted_out, the caller owns that returned event and may free it.
                 *   We therefore store an internal copy for reconciliation.
                 * - If caller did not request predicted_out, the proxy owns the predicted event and stores it directly.
                 */
                if (predicted_out) {
                    pending->predicted_event = malloc(sizeof(struct input_event));
                    if (pending->predicted_event) {
                        *pending->predicted_event = *predicted;
                    }
                } else {
                    pending->predicted_event = predicted;
                    predicted = NULL; /* transferred to pending list */
                }
                pending->timestamp_us = now_us;
                pending->next = proxy->pending_predictions;
                proxy->pending_predictions = pending;
                proxy->pending_count++;
            } else {
                /* If nobody will receive this predicted event, avoid leaking it. */
                if (!predicted_out) {
                    free(predicted);
                    predicted = NULL;
                }
            }
            
            if (predicted_out) {
                *predicted_out = predicted;
            }
            
            proxy->prediction_state.events_predicted++;
            proxy->prediction_state.last_prediction_us = now_us;
        }
    }
    
    return 0;
}

int input_proxy_reconcile(struct input_proxy *proxy,
                         uint64_t frame_id,
                         const struct input_event *actual_event) {
    if (!proxy) {
        return -1;
    }
    
    /* Find pending prediction for this frame */
    struct pending_prediction **pred_ptr = &proxy->pending_predictions;
    struct pending_prediction *found = NULL;
    
    while (*pred_ptr) {
        if ((*pred_ptr)->frame_id == frame_id) {
            found = *pred_ptr;
            *pred_ptr = (*pred_ptr)->next;  /* Remove from list */
            break;
        }
        pred_ptr = &(*pred_ptr)->next;
    }
    
    if (found) {
        /* Compare predicted vs actual */
        bool prediction_correct = false;
        
        if (actual_event == NULL) {
            /* No actual event provided - assume prediction was correct */
            prediction_correct = true;
        } else if (found->predicted_event) {
            /* Compare predicted and actual events */
            if (found->predicted_event->type == actual_event->type) {
                if (found->predicted_event->type == INPUT_EVENT_POINTER_MOTION) {
                    /* Check if deltas are close (within tolerance) */
                    double dx_diff = found->predicted_event->pointer_motion.dx - 
                                   actual_event->pointer_motion.dx;
                    double dy_diff = found->predicted_event->pointer_motion.dy - 
                                   actual_event->pointer_motion.dy;
                    double tolerance = 0.1;  /* 10% tolerance */
                    
                    if (fabs(dx_diff) < tolerance && fabs(dy_diff) < tolerance) {
                        prediction_correct = true;
                    }
                } else {
                    /* For other event types, exact match */
                    prediction_correct = true;
                }
            }
        }
        
        if (!prediction_correct && actual_event && found->predicted_event) {
            /* Apply correction: calculate delta and update metrics */
            if (found->predicted_event->type == INPUT_EVENT_POINTER_MOTION &&
                actual_event->type == INPUT_EVENT_POINTER_MOTION) {
                double dx_error = found->predicted_event->pointer_motion.dx -
                                actual_event->pointer_motion.dx;
                double dy_error = found->predicted_event->pointer_motion.dy -
                                actual_event->pointer_motion.dy;
                
                /* Correction delta would be applied to local cursor position */
                /* In production, this would trigger a correction event to the compositor */
                /* For now, we just track the error for prediction model updates */
                
                /* Update prediction accuracy metrics */
                /* Larger errors indicate prediction model needs adjustment */
                (void)dx_error;  /* Used for future model updates */
                (void)dy_error;  /* Used for future model updates */
            }
        }
        
        /* Free prediction entry */
        if (found->predicted_event) {
            free(found->predicted_event);
        }
        free(found);
        proxy->pending_count--;
    }
    
    proxy->prediction_state.events_reconciled++;
    
    /* Clean up stale predictions (older than 1 second) */
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t now_us = ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;
    uint64_t stale_threshold_us = 1000000ULL;  /* 1 second */
    
    pred_ptr = &proxy->pending_predictions;
    while (*pred_ptr) {
        if (now_us - (*pred_ptr)->timestamp_us > stale_threshold_us) {
            struct pending_prediction *stale = *pred_ptr;
            *pred_ptr = (*pred_ptr)->next;
            if (stale->predicted_event) {
                free(stale->predicted_event);
            }
            free(stale);
            proxy->pending_count--;
        } else {
            pred_ptr = &(*pred_ptr)->next;
        }
    }
    
    return 0;
}

int input_proxy_get_prediction_state(const struct input_proxy *proxy,
                                    prediction_state_t *state_out) {
    if (!proxy || !state_out) {
        return -1;
    }
    
    *state_out = proxy->prediction_state;
    return 0;
}

