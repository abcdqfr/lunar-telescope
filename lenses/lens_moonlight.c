#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 500
#include "lens.h"
#include "../core/telescope.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

/**
 * Moonlight lens implementation
 *
 * Moonlight provides low-latency decode optimized for client-side performance.
 * Uses CLI-driven approach: fork/exec moonlight-qt or moonlight client process.
 */

struct moonlight_session {
    struct telescope_config *config;
    pid_t moonlight_pid;
    bool running;
    uint64_t start_time_us;
};

static int moonlight_create(const struct telescope_config *config,
                            struct lens_session **session_out) {
    if (!config || !session_out) {
        return -EINVAL;
    }
    
    struct moonlight_session *ms = calloc(1, sizeof(struct moonlight_session));
    if (!ms) {
        return -ENOMEM;
    }
    
    ms->config = (struct telescope_config *)config;
    ms->moonlight_pid = -1;
    ms->running = false;
    
    struct lens_session *session = calloc(1, sizeof(struct lens_session));
    if (!session) {
        free(ms);
        return -ENOMEM;
    }
    
    session->type = TELESCOPE_LENS_MOONLIGHT;
    session->ops = lens_get_ops(TELESCOPE_LENS_MOONLIGHT);
    session->private_data = ms;
    session->process_pid = -1;
    session->running = false;
    
    *session_out = session;
    return 0;
}

static int build_moonlight_argv(const struct telescope_config *config,
                                char ***argv_out, size_t *argc_out) {
    const telescope_connection_t *conn = &config->connection;
    const telescope_application_t *app = &config->application;
    
    size_t max_args = 32;
    char **argv = calloc(max_args, sizeof(char*));
    if (!argv) {
        return -ENOMEM;
    }
    
    size_t argc = 0;
    
    /* Moonlight client command (assuming 'moonlight' or 'moonlight-qt' CLI) */
    argv[argc++] = strdup("moonlight");
    
    /* Connection parameters */
    if (conn->remote_host) {
        argv[argc++] = strdup("stream");
        argv[argc++] = strdup(conn->remote_host);
    }
    
    if (conn->remote_port != 0 && conn->remote_port != 47984) {
        /* Default Moonlight port is 47984, only specify if different */
        char port_str[16];
        snprintf(port_str, sizeof(port_str), "%u", conn->remote_port);
        argv[argc++] = strdup("--port");
        argv[argc++] = strdup(port_str);
    }
    
    /* Performance options */
    if (config->performance.frame_rate > 0) {
        char fps_str[16];
        snprintf(fps_str, sizeof(fps_str), "%u", config->performance.frame_rate);
        argv[argc++] = strdup("--fps");
        argv[argc++] = strdup(fps_str);
    }
    
    if (conn->video_codec) {
        argv[argc++] = strdup("--codec");
        argv[argc++] = strdup(conn->video_codec);
    }
    
    /* Application to launch */
    if (app->executable) {
        argv[argc++] = strdup(app->executable);
        
        /* Application arguments */
        if (app->args_count > 0) {
            for (size_t i = 0; i < app->args_count; i++) {
                if (argc >= max_args - 1) {
                    max_args *= 2;
                    char **new_argv = realloc(argv, max_args * sizeof(char*));
                    if (!new_argv) {
                        goto error;
                    }
                    argv = new_argv;
                }
                argv[argc++] = strdup(app->args[i]);
            }
        }
    }
    
    argv[argc] = NULL;
    
    *argv_out = argv;
    *argc_out = argc;
    return 0;
    
error:
    for (size_t i = 0; i < argc; i++) {
        free(argv[i]);
    }
    free(argv);
    return -ENOMEM;
}

static void free_moonlight_argv(char **argv, size_t argc) {
    if (!argv) {
        return;
    }
    for (size_t i = 0; i < argc; i++) {
        free(argv[i]);
    }
    free(argv);
}

static int moonlight_start(struct lens_session *session) {
    if (!session || !session->private_data) {
        return -EINVAL;
    }
    
    struct moonlight_session *ms = (struct moonlight_session *)session->private_data;
    
    if (ms->running) {
        return -EBUSY;
    }
    
    char **moonlight_argv = NULL;
    size_t moonlight_argc = 0;
    int ret = build_moonlight_argv(ms->config, &moonlight_argv, &moonlight_argc);
    if (ret < 0) {
        return ret;
    }
    
    pid_t pid = fork();
    if (pid < 0) {
        free_moonlight_argv(moonlight_argv, moonlight_argc);
        return -errno;
    }
    
    if (pid == 0) {
        /* Child: exec moonlight client */
        if (ms->config->application.env_count > 0) {
            for (size_t i = 0; i < ms->config->application.env_count; i++) {
                putenv(ms->config->application.env[i]);
            }
        }
        
        if (ms->config->application.working_directory) {
            if (chdir(ms->config->application.working_directory) != 0) {
                /* Ignore chdir errors in child - will be handled by exec */
            }
        }
        
        execvp("moonlight", moonlight_argv);
        _exit(127);
    }
    
    ms->moonlight_pid = pid;
    session->process_pid = pid;
    
    free_moonlight_argv(moonlight_argv, moonlight_argc);
    
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    ms->start_time_us = ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;
    ms->running = true;
    session->running = true;
    
    return 0;
}

static int moonlight_stop(struct lens_session *session) {
    if (!session || !session->private_data) {
        return -EINVAL;
    }
    
    struct moonlight_session *ms = (struct moonlight_session *)session->private_data;
    
    if (!ms->running) {
        return 0;
    }
    
    if (ms->moonlight_pid > 0) {
        kill(ms->moonlight_pid, SIGTERM);
        waitpid(ms->moonlight_pid, NULL, 0);
        ms->moonlight_pid = -1;
        session->process_pid = -1;
    }
    
    ms->running = false;
    session->running = false;
    
    return 0;
}

static void moonlight_destroy(struct lens_session *session) {
    if (!session) {
        return;
    }
    
    moonlight_stop(session);
    
    if (session->private_data) {
        free(session->private_data);
    }
    
    free(session);
}

static int moonlight_get_metrics(const struct lens_session *session,
                                 struct telescope_metrics *metrics_out) {
    if (!session || !metrics_out) {
        return -EINVAL;
    }
    
    /* Moonlight-specific metrics would be collected here */
    /* For now, return placeholder metrics */
    memset(metrics_out, 0, sizeof(struct telescope_metrics));
    
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    metrics_out->timestamp_us = ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;
    
    /* TODO: Query Moonlight client for actual metrics if API available */
    /* Could parse stdout/stderr or query status endpoint */
    
    return 0;
}

const lens_ops_t moonlight_ops = {
    .create = moonlight_create,
    .start = moonlight_start,
    .stop = moonlight_stop,
    .destroy = moonlight_destroy,
    .get_metrics = moonlight_get_metrics
};

