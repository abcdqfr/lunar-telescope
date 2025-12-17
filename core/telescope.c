#define _POSIX_C_SOURCE 200809L
#include "telescope.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>

/* Forward declaration for metrics */
extern int metrics_collector_init(const telescope_observability_t *obs_config);
extern void metrics_collector_cleanup(void);
extern const struct telescope_metrics *metrics_collector_get(void);

/**
 * Telescope session management
 */

struct telescope_session {
    struct telescope_config *config;
    pid_t waypipe_pid;
    bool running;
    struct telescope_metrics metrics;
    uint64_t start_time_us;
};

int telescope_session_create(const struct telescope_config *config,
                            struct telescope_session **session_out) {
    if (!config || !session_out) {
        return -EINVAL;
    }
    
    struct telescope_session *session = calloc(1, sizeof(struct telescope_session));
    if (!session) {
        return -ENOMEM;
    }
    
    /* Copy configuration (simplified - in production would deep copy) */
    session->config = (struct telescope_config *)config;
    session->waypipe_pid = -1;
    session->running = false;
    
    memset(&session->metrics, 0, sizeof(session->metrics));
    
    *session_out = session;
    return 0;
}

/* Build waypipe command arguments from configuration */
static int build_waypipe_argv(const struct telescope_config *config,
                              char ***argv_out, size_t *argc_out) {
    const telescope_connection_t *conn = &config->connection;
    const telescope_application_t *app = &config->application;
    
    /* Estimate maximum argument count */
    size_t max_args = 32;  /* waypipe + client + options + ssh + app + args */
    char **argv = calloc(max_args, sizeof(char*));
    if (!argv) {
        return -ENOMEM;
    }
    
    size_t argc = 0;
    
    /* waypipe client */
    argv[argc++] = strdup("waypipe");
    argv[argc++] = strdup("client");
    
    /* Compression option */
    if (conn->compression && strcmp(conn->compression, "none") != 0) {
        char *compress_opt = malloc(strlen(conn->compression) + 12);
        if (!compress_opt) {
            goto error;
        }
        snprintf(compress_opt, strlen(conn->compression) + 12, "--compress=%s", conn->compression);
        argv[argc++] = compress_opt;
    }
    
    /* Video codec option (if waypipe supports it) */
    if (conn->video_codec) {
        char *codec_opt = malloc(strlen(conn->video_codec) + 14);
        if (!codec_opt) {
            goto error;
        }
        snprintf(codec_opt, strlen(conn->video_codec) + 14, "--video-codec=%s", conn->video_codec);
        argv[argc++] = codec_opt;
    }
    
    /* SSH connection */
    argv[argc++] = strdup("--ssh");
    
    /* Build SSH user@host string */
    char *ssh_target = malloc(strlen(conn->ssh_user) + strlen(conn->remote_host) + 2);
    if (!ssh_target) {
        goto error;
    }
    snprintf(ssh_target, strlen(conn->ssh_user) + strlen(conn->remote_host) + 2,
             "%s@%s", conn->ssh_user, conn->remote_host);
    argv[argc++] = ssh_target;
    
    /* Separator */
    argv[argc++] = strdup("--");
    
    /* Application executable */
    argv[argc++] = strdup(app->executable);
    
    /* Application arguments */
    for (size_t i = 0; i < app->args_count; i++) {
        if (argc >= max_args - 1) {
            /* Reallocate if needed */
            max_args *= 2;
            char **new_argv = realloc(argv, max_args * sizeof(char*));
            if (!new_argv) {
                goto error;
            }
            argv = new_argv;
        }
        argv[argc++] = strdup(app->args[i]);
    }
    
    /* NULL terminator */
    argv[argc] = NULL;
    
    *argv_out = argv;
    *argc_out = argc;
    return 0;
    
error:
    /* Free allocated strings */
    for (size_t i = 0; i < argc; i++) {
        free(argv[i]);
    }
    free(argv);
    return -ENOMEM;
}

/* Free waypipe argv */
static void free_waypipe_argv(char **argv, size_t argc) {
    if (!argv) {
        return;
    }
    for (size_t i = 0; i < argc; i++) {
        free(argv[i]);
    }
    free(argv);
}

int telescope_session_start(struct telescope_session *session) {
    if (!session || !session->config) {
        return -EINVAL;
    }
    
    if (session->running) {
        return -EBUSY;
    }
    
    /* Only support waypipe lens for now */
    telescope_lens_t lens = telescope_select_lens(session->config);
    if (lens != TELESCOPE_LENS_WAYPIPE && lens != TELESCOPE_LENS_AUTO) {
        /* Other lenses not yet implemented */
        return -ENOTSUP;
    }
    
    /* Build waypipe command arguments */
    char **waypipe_argv = NULL;
    size_t waypipe_argc = 0;
    int ret = build_waypipe_argv(session->config, &waypipe_argv, &waypipe_argc);
    if (ret < 0) {
        return ret;
    }
    
    /* Fork to launch waypipe */
    pid_t pid = fork();
    if (pid < 0) {
        free_waypipe_argv(waypipe_argv, waypipe_argc);
        return -errno;
    }
    
    if (pid == 0) {
        /* Child process: exec waypipe */
        
        /* Set up environment variables if specified */
        if (session->config->application.env_count > 0) {
            for (size_t i = 0; i < session->config->application.env_count; i++) {
                putenv(session->config->application.env[i]);
            }
        }
        
        /* Change working directory if specified */
        if (session->config->application.working_directory) {
            chdir(session->config->application.working_directory);
        }
        
        /* Redirect stdout/stderr for monitoring (optional) */
        /* In production, might want to capture for metrics */
        
        /* Execute waypipe */
        execvp("waypipe", waypipe_argv);
        
        /* If we get here, exec failed */
        _exit(127);
    }
    
    /* Parent process: store PID and continue */
    session->waypipe_pid = pid;
    
    /* Free argv (child has exec'd, parent doesn't need it) */
    free_waypipe_argv(waypipe_argv, waypipe_argc);
    
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    session->start_time_us = ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;
    session->running = true;
    
    /* Initialize metrics collection */
    metrics_collector_init(&session->config->observability);
    
    /* Initialize logging if available */
    #ifdef LOGGING_H
    extern int logging_init(log_level_t, FILE *);
    logging_init(session->config->observability.log_level, NULL);
    #endif
    
    return 0;
}

int telescope_session_stop(struct telescope_session *session) {
    if (!session) {
        return -EINVAL;
    }
    
    if (!session->running) {
        return 0;
    }
    
    if (session->waypipe_pid > 0) {
        kill(session->waypipe_pid, SIGTERM);
        waitpid(session->waypipe_pid, NULL, 0);
        session->waypipe_pid = -1;
    }
    
    session->running = false;
    
    /* Cleanup metrics */
    metrics_collector_cleanup();
    
    return 0;
}

void telescope_session_destroy(struct telescope_session *session) {
    if (!session) {
        return;
    }
    
    telescope_session_stop(session);
    free(session);
}

int telescope_session_get_metrics(const struct telescope_session *session,
                                  struct telescope_metrics *metrics_out) {
    if (!session || !metrics_out) {
        return -EINVAL;
    }
    
    /* Try to get metrics from collector first */
    const struct telescope_metrics *collected = metrics_collector_get();
    if (collected) {
        *metrics_out = *collected;
    } else {
        /* Fallback to session metrics */
        *metrics_out = session->metrics;
        
        /* Update timestamp */
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        metrics_out->timestamp_us = ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;
    }
    
    return 0;
}

