#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Utility functions for Lunar Telescope
 *
 * Common helper functions used across the codebase.
 */

/**
 * Get current timestamp in microseconds (monotonic clock)
 *
 * @return Timestamp in microseconds
 */
uint64_t utils_timestamp_us(void);

/**
 * Get current timestamp in seconds (monotonic clock, as double)
 *
 * @return Timestamp in seconds
 */
double utils_timestamp_sec(void);

/**
 * Convert microseconds to seconds
 */
static inline double utils_us_to_sec(uint64_t us) {
    return us / 1000000.0;
}

/**
 * Convert seconds to microseconds
 */
static inline uint64_t utils_sec_to_us(double sec) {
    return (uint64_t)(sec * 1000000.0);
}

/**
 * Calculate time difference in milliseconds
 *
 * @param start_us Start timestamp (microseconds)
 * @param end_us End timestamp (microseconds)
 * @return Difference in milliseconds
 */
uint32_t utils_time_diff_ms(uint64_t start_us, uint64_t end_us);

/**
 * Check if a file exists and is readable
 *
 * @param path File path
 * @return true if file exists and is readable
 */
bool utils_file_exists(const char *path);

/**
 * Safe string duplication (with null check)
 *
 * @param str String to duplicate
 * @return Duplicated string, or NULL on error
 */
char *utils_strdup_safe(const char *str);

#ifdef __cplusplus
}
#endif

#endif /* UTILS_H */

