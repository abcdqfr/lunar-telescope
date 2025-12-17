#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include "../core/telescope.h"
#include "../input/input.h"
#include "../compositor/compositor.h"

/**
 * Integration tests for Lunar Telescope
 *
 * Tests end-to-end functionality including:
 * - Configuration loading and session creation
 * - Input proxy with prediction
 * - Metrics collection
 * - Compositor hooks
 */

/* Test configuration template */
static const char *test_config_json = "{\n"
    "  \"connection\": {\n"
    "    \"remote_host\": \"localhost\",\n"
    "    \"remote_port\": 22,\n"
    "    \"ssh_user\": \"test\"\n"
    "  },\n"
    "  \"application\": {\n"
    "    \"executable\": \"/usr/bin/echo\",\n"
    "    \"args\": [\"test\"]\n"
    "  },\n"
    "  \"performance\": {\n"
    "    \"profile\": \"balanced\"\n"
    "  }\n"
"}";

void test_config_and_session(void) {
    printf("Testing configuration and session...\n");
    
    /* Create temporary config file */
    const char *config_path = "/tmp/lunar_test_config.json";
    FILE *fp = fopen(config_path, "w");
    assert(fp != NULL);
    fputs(test_config_json, fp);
    fclose(fp);
    
    /* Load configuration */
    struct telescope_config *config = NULL;
    int ret = telescope_config_load(config_path, &config);
    assert(ret == 0);
    assert(config != NULL);
    assert(strcmp(config->connection.remote_host, "localhost") == 0);
    
    /* Create session */
    struct telescope_session *session = NULL;
    ret = telescope_session_create(config, &session);
    assert(ret == 0);
    assert(session != NULL);
    
    /* Cleanup */
    telescope_session_destroy(session);
    telescope_config_free(config);
    unlink(config_path);
    
    printf("  ✓ Configuration and session test passed\n");
}

void test_input_proxy_integration(void) {
    printf("Testing input proxy integration...\n");
    
    /* Create input proxy */
    struct input_proxy *proxy = NULL;
    int ret = input_proxy_create(true, 16, true, &proxy);
    assert(ret == 0);
    assert(proxy != NULL);
    
    /* Create pointer motion event */
    struct input_event event = {
        .type = INPUT_EVENT_POINTER_MOTION,
        .timestamp_us = 1000000,
        .pointer_motion = {
            .dx = 10.0,
            .dy = 5.0,
            .absolute = false
        }
    };
    
    /* Process event */
    struct input_event *predicted = NULL;
    ret = input_proxy_process(proxy, &event, &predicted);
    assert(ret == 0);
    
    /* Verify prediction was applied (may or may not have Rust predictor) */
    if (predicted) {
        assert(predicted->type == INPUT_EVENT_POINTER_MOTION);
        free(predicted);
    }
    
    /* Test reconciliation */
    ret = input_proxy_reconcile(proxy, 1, NULL);
    assert(ret == 0);
    
    input_proxy_destroy(proxy);
    
    printf("  ✓ Input proxy integration test passed\n");
}

void test_compositor_hooks(void) {
    printf("Testing compositor hooks...\n");
    
    /* Initialize compositor hooks */
    int ret = compositor_hooks_init();
    assert(ret == 0);
    
    /* Test input interception (stub - would need actual device) */
    /* For now, just verify hooks are initialized */
    
    compositor_hooks_cleanup();
    
    printf("  ✓ Compositor hooks test passed\n");
}

void test_metrics_collection(void) {
    printf("Testing metrics collection...\n");
    
    telescope_observability_t obs_config = {
        .enable_metrics = true,
        .metrics_interval_ms = 1000,
        .metrics_file = NULL,
        .log_level = 2
    };
    
    extern int metrics_collector_init(const telescope_observability_t *);
    extern void metrics_record_frame(uint32_t, bool);
    extern void metrics_record_input_event(bool, bool);
    extern void metrics_record_bandwidth(uint64_t, uint64_t);
    extern void metrics_collector_cleanup(void);
    
    int ret = metrics_collector_init(&obs_config);
    assert(ret == 0);
    
    /* Record some metrics */
    metrics_record_frame(16, false);
    metrics_record_input_event(true, false);
    metrics_record_bandwidth(1000, 500);
    
    /* Get metrics */
    extern const struct telescope_metrics *metrics_collector_get(void);
    const struct telescope_metrics *metrics = metrics_collector_get();
    assert(metrics != NULL);
    assert(metrics->frames_total > 0);
    assert(metrics->input_events_total > 0);
    
    metrics_collector_cleanup();
    
    printf("  ✓ Metrics collection test passed\n");
}

void test_profile_application(void) {
    printf("Testing profile application...\n");
    
    struct telescope_config *config = calloc(1, sizeof(struct telescope_config));
    assert(config != NULL);
    
    config->connection.compression = strdup("none");
    config->connection.video_codec = strdup("h264");
    config->connection.bandwidth_limit_mbps = 0;
    
    int ret = telescope_config_apply_profile(config, TELESCOPE_PROFILE_LOW_LATENCY);
    assert(ret == 0);
    assert(config->performance.profile == TELESCOPE_PROFILE_LOW_LATENCY);
    assert(config->performance.target_latency_ms == 16);
    assert(strcmp(config->connection.compression, "lz4") == 0);
    
    telescope_config_free(config);
    
    printf("  ✓ Profile application test passed\n");
}

int main(void) {
    printf("\n=== Lunar Telescope Integration Tests ===\n\n");
    
    test_config_and_session();
    test_input_proxy_integration();
    test_compositor_hooks();
    test_metrics_collection();
    test_profile_application();
    
    printf("\n=== All Integration Tests Passed! ===\n\n");
    return 0;
}

