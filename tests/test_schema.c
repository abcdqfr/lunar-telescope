#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "../core/telescope.h"

/* Test configuration loading */
void test_config_load_valid(void) {
    const char *test_config = "/tmp/test_config.json";
    FILE *fp = fopen(test_config, "w");
    assert(fp != NULL);
    
    fprintf(fp, "{\n"
            "  \"connection\": {\n"
            "    \"remote_host\": \"example.com\",\n"
            "    \"remote_port\": 22,\n"
            "    \"ssh_user\": \"user\"\n"
            "  },\n"
            "  \"application\": {\n"
            "    \"executable\": \"/usr/bin/test\",\n"
            "    \"args\": [\"--test\"]\n"
            "  }\n"
            "}\n");
    fclose(fp);
    
    struct telescope_config *config = NULL;
    int ret = telescope_config_load(test_config, &config);
    assert(ret == 0);
    assert(config != NULL);
    assert(strcmp(config->connection.remote_host, "example.com") == 0);
    assert(config->connection.remote_port == 22);
    assert(strcmp(config->application.executable, "/usr/bin/test") == 0);
    
    telescope_config_free(config);
    unlink(test_config);
    
    printf("✓ test_config_load_valid passed\n");
}

/* Test profile application */
void test_profile_application(void) {
    struct telescope_config *config = calloc(1, sizeof(struct telescope_config));
    assert(config != NULL);
    
    config->connection.compression = strdup("none");
    config->connection.video_codec = strdup("h264");
    config->connection.bandwidth_limit_mbps = 0;
    
    int ret = telescope_config_apply_profile(config, TELESCOPE_PROFILE_LOW_LATENCY);
    assert(ret == 0);
    assert(config->performance.profile == TELESCOPE_PROFILE_LOW_LATENCY);
    assert(config->performance.target_latency_ms == 16);
    assert(config->performance.frame_rate == 120);
    assert(strcmp(config->connection.compression, "lz4") == 0);
    
    telescope_config_free(config);
    
    printf("✓ test_profile_application passed\n");
}

/* Test lens selection */
void test_lens_selection(void) {
    struct telescope_config *config = calloc(1, sizeof(struct telescope_config));
    assert(config != NULL);
    
    config->application.executable = strdup("/usr/bin/mpv");
    config->lens.type = TELESCOPE_LENS_AUTO;
    
    telescope_lens_t lens = telescope_select_lens(config);
    assert(lens == TELESCOPE_LENS_SUNSHINE || lens == TELESCOPE_LENS_WAYPIPE);
    
    free(config->application.executable);
    config->application.executable = strdup("/usr/bin/editor");
    lens = telescope_select_lens(config);
    assert(lens == TELESCOPE_LENS_WAYPIPE);
    
    telescope_config_free(config);
    
    printf("✓ test_lens_selection passed\n");
}

int main(void) {
    printf("Running schema tests...\n\n");
    
    test_config_load_valid();
    test_profile_application();
    test_lens_selection();
    
    printf("\nAll schema tests passed!\n");
    return 0;
}

