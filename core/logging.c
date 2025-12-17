#include "logging.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

static log_level_t g_log_level = LOG_LEVEL_INFO;
static FILE *g_log_file = NULL;

int logging_init(log_level_t level, FILE *file) {
    g_log_level = level;
    g_log_file = file ? file : stderr;
    return 0;
}

void logging_set_level(log_level_t level) {
    g_log_level = level;
}

void logging_log(log_level_t level, const char *file, int line,
                const char *func, const char *format, ...) {
    if (level > g_log_level) {
        return;  /* Below threshold */
    }
    
    const char *level_str;
    switch (level) {
        case LOG_LEVEL_ERROR: level_str = "ERROR"; break;
        case LOG_LEVEL_WARN:  level_str = "WARN";  break;
        case LOG_LEVEL_INFO:  level_str = "INFO";  break;
        case LOG_LEVEL_DEBUG: level_str = "DEBUG"; break;
        case LOG_LEVEL_TRACE: level_str = "TRACE"; break;
        default: level_str = "UNKNOWN"; break;
    }
    
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm *tm_info = localtime(&ts.tv_sec);
    
    /* Format: [HH:MM:SS.mmm] [LEVEL] file:line:func: message */
    fprintf(g_log_file, "[%02d:%02d:%02d.%03ld] [%s] %s:%d:%s: ",
            tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
            ts.tv_nsec / 1000000,
            level_str,
            strrchr(file, '/') ? strrchr(file, '/') + 1 : file,
            line, func ? func : "");
    
    va_list args;
    va_start(args, format);
    vfprintf(g_log_file, format, args);
    va_end(args);
    
    fprintf(g_log_file, "\n");
    fflush(g_log_file);
}

