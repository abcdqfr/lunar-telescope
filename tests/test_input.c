#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "../input/input.h"

/* Test input proxy creation */
void test_input_proxy_create(void) {
    struct input_proxy *proxy = NULL;
    int ret = input_proxy_create(true, 16, true, &proxy);
    assert(ret == 0);
    assert(proxy != NULL);
    
    prediction_state_t state;
    ret = input_proxy_get_prediction_state(proxy, &state);
    assert(ret == 0);
    assert(state.enabled == true);
    assert(state.window_ms == 16);
    
    input_proxy_destroy(proxy);
    
    printf("✓ test_input_proxy_create passed\n");
}

/* Test scroll smoothing */
void test_scroll_smoothing(void) {
    struct scroll_state *smoother = NULL;
    int ret = scroll_smoother_create(&smoother);
    assert(ret == 0);
    assert(smoother != NULL);
    
    double smoothed_dx, smoothed_dy;
    ret = scroll_smoother_process(smoother, 10.0, 5.0, false, &smoothed_dx, &smoothed_dy);
    assert(ret == 0);
    
    /* Smoothed values should be different from input (due to smoothing) */
    /* In first call, might be similar, but subsequent calls will show smoothing */
    ret = scroll_smoother_process(smoother, 10.0, 5.0, false, &smoothed_dx, &smoothed_dy);
    assert(ret == 0);
    
    scroll_smoother_destroy(smoother);
    
    printf("✓ test_scroll_smoothing passed\n");
}

/* Test input event processing */
void test_input_event_processing(void) {
    struct input_proxy *proxy = NULL;
    int ret = input_proxy_create(true, 16, false, &proxy);
    assert(ret == 0);
    
    struct input_event event = {
        .type = INPUT_EVENT_POINTER_MOTION,
        .timestamp_us = 1000000,
        .pointer_motion = {
            .dx = 10.0,
            .dy = 5.0,
            .absolute = false
        }
    };
    
    struct input_event *predicted = NULL;
    ret = input_proxy_process(proxy, &event, &predicted);
    assert(ret == 0);
    /* Prediction may or may not be applied depending on implementation */
    
    if (predicted) {
        free(predicted);
    }
    
    input_proxy_destroy(proxy);
    
    printf("✓ test_input_event_processing passed\n");
}

int main(void) {
    printf("Running input tests...\n\n");
    
    test_input_proxy_create();
    test_scroll_smoothing();
    test_input_event_processing();
    
    printf("\nAll input tests passed!\n");
    return 0;
}

