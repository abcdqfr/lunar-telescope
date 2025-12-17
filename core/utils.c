#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

uint64_t utils_timestamp_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;
}

double utils_timestamp_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1000000000.0;
}

uint32_t utils_time_diff_ms(uint64_t start_us, uint64_t end_us) {
    if (end_us < start_us) {
        return 0;  /* Invalid or wrapped */
    }
    return (end_us - start_us) / 1000;
}

bool utils_file_exists(const char *path) {
    if (!path) {
        return false;
    }
    return access(path, R_OK) == 0;
}

char *utils_strdup_safe(const char *str) {
    if (!str) {
        return NULL;
    }
    return strdup(str);
}

