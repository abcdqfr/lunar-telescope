#ifndef LOGGING_H
#define LOGGING_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Simple logging system for Lunar Telescope
 *
 * Provides structured logging with levels, no external dependencies.
 */

typedef enum {
    LOG_LEVEL_ERROR = 0,
    LOG_LEVEL_WARN = 1,
    LOG_LEVEL_INFO = 2,
    LOG_LEVEL_DEBUG = 3,
    LOG_LEVEL_TRACE = 4
} log_level_t;

/**
 * Initialize logging system
 *
 * @param level Minimum log level to output
 * @param file Output file (NULL = stderr)
 * @return 0 on success, negative on error
 */
int logging_init(log_level_t level, FILE *file);

/**
 * Set log level
 */
void logging_set_level(log_level_t level);

/**
 * Log a message
 */
void logging_log(log_level_t level, const char *file, int line,
                const char *func, const char *format, ...);

/* Convenience macros */
#define LOG_ERROR(...) logging_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_WARN(...)  logging_log(LOG_LEVEL_WARN, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_INFO(...)  logging_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_DEBUG(...) logging_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_TRACE(...) logging_log(LOG_LEVEL_TRACE, __FILE__, __LINE__, __func__, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* LOGGING_H */

