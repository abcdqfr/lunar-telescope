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
#include <fcntl.h>

/**
 * Sunshine lens implementation
 *
 * Sunshine provides high-motion video streaming optimized for gaming.
 * Uses CLI-driven approach: fork/exec sunshine client process.
 */

struct sunshine_session {
    struct telescope_config *config;
    pid_t sunshine_pid;
    bool running;
    uint64_t start_time_us;
};

static int sunshine_create(const struct telescope_config *config,
                           struct lens_session **session_out) {
    if (!config || !session_out) {
        return -EINVAL;
    }
    
    struct sunshine_session *ss = calloc(1, sizeof(struct sunshine_session));
    if (!ss) {
        return -ENOMEM;
    }
    
    ss->config = (struct telescope_config *)config;
    ss->sunshine_pid = -1;
    ss->running = false;
    
    struct lens_session *session = calloc(1, sizeof(struct lens_session));
    if (!session) {
        free(ss);
        return -ENOMEM;
    }
    
    session->type = TELESCOPE_LENS_SUNSHINE;
    session->ops = lens_get_ops(TELESCOPE_LENS_SUNSHINE);
    session->private_data = ss;
    session->process_pid = -1;
    session->running = false;
    
    *session_out = session;
    return 0;
}

static int build_sunshine_argv(const struct telescope_config *config,
                               char ***argv_out, size_t *argc_out) {
    const telescope_connection_t *conn = &config->connection;
    const telescope_application_t *app = &config->application;
    
    size_t max_args = 32;
    char **argv = calloc(max_args, sizeof(char*));
    if (!argv) {
        return -ENOMEM;
    }
    
    size_t argc = 0;
    
    /* Sunshine client command (assuming 'sunshine' or 'sunshine-client' CLI) */
    argv[argc++] = strdup("sunshine");
    
    /* Connection parameters */
    if (conn->remote_host) {
        argv[argc++] = strdup("--host");
        argv[argc++] = strdup(conn->remote_host);
    }
    
    if (conn->remote_port != 0 && conn->remote_port != 47989) {
        /* Default Sunshine port is 47989, only specify if different */
        char port_str[16];
        snprintf(port_str, sizeof(port_str), "%u", conn->remote_port);
        argv[argc++] = strdup("--port");
        argv[argc++] = strdup(port_str);
    }
    
    /* Performance options from config */
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
        argv[argc++] = strdup("--app");
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

static void free_sunshine_argv(char **argv, size_t argc) {
    if (!argv) {
        return;
    }
    for (size_t i = 0; i < argc; i++) {
        free(argv[i]);
    }
    free(argv);
}

static int sunshine_start(struct lens_session *session) {
    if (!session || !session->private_data) {
        return -EINVAL;
    }
    
    struct sunshine_session *ss = (struct sunshine_session *)session->private_data;
    
    if (ss->running) {
        return -EBUSY;
    }
    
    char **sunshine_argv = NULL;
    size_t sunshine_argc = 0;
    int ret = build_sunshine_argv(ss->config, &sunshine_argv, &sunshine_argc);
    if (ret < 0) {
        return ret;
    }

    /* Exec handshake: detect execvp() failure reliably (missing binary, etc.) */
    int exec_pipe[2];
    if (pipe(exec_pipe) != 0) {
        free_sunshine_argv(sunshine_argv, sunshine_argc);
        return -errno;
    }
    if (fcntl(exec_pipe[1], F_SETFD, FD_CLOEXEC) != 0) {
        close(exec_pipe[0]);
        close(exec_pipe[1]);
        free_sunshine_argv(sunshine_argv, sunshine_argc);
        return -errno;
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(exec_pipe[0]);
        close(exec_pipe[1]);
        free_sunshine_argv(sunshine_argv, sunshine_argc);
        return -errno;
    }
    
    if (pid == 0) {
        /* Child: exec sunshine client */
        close(exec_pipe[0]); /* child writes */
        if (ss->config->application.env_count > 0) {
            for (size_t i = 0; i < ss->config->application.env_count; i++) {
                putenv(ss->config->application.env[i]);
            }
        }
        
        if (ss->config->application.working_directory) {
            if (chdir(ss->config->application.working_directory) != 0) {
                /* Ignore chdir errors in child - will be handled by exec */
            }
        }
        
        execvp("sunshine", sunshine_argv);
        int err = errno;
        ssize_t _ignored = write(exec_pipe[1], &err, sizeof(err));
        (void)_ignored;
        _exit(127);
    }

    close(exec_pipe[1]); /* parent reads */
    free_sunshine_argv(sunshine_argv, sunshine_argc);

    int child_errno = 0;
    ssize_t n;
    do {
        n = read(exec_pipe[0], &child_errno, sizeof(child_errno));
    } while (n < 0 && errno == EINTR);
    close(exec_pipe[0]);

    if (n > 0) {
        (void)waitpid(pid, NULL, 0);
        return -child_errno;
    }

    ss->sunshine_pid = pid;
    session->process_pid = pid;
    
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    ss->start_time_us = ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;
    ss->running = true;
    session->running = true;
    
    return 0;
}

static int sunshine_stop(struct lens_session *session) {
    if (!session || !session->private_data) {
        return -EINVAL;
    }
    
    struct sunshine_session *ss = (struct sunshine_session *)session->private_data;
    
    if (!ss->running) {
        return 0;
    }
    
    if (ss->sunshine_pid > 0) {
        kill(ss->sunshine_pid, SIGTERM);
        waitpid(ss->sunshine_pid, NULL, 0);
        ss->sunshine_pid = -1;
        session->process_pid = -1;
    }
    
    ss->running = false;
    session->running = false;
    
    return 0;
}

static void sunshine_destroy(struct lens_session *session) {
    if (!session) {
        return;
    }
    
    sunshine_stop(session);
    
    if (session->private_data) {
        free(session->private_data);
    }
    
    free(session);
}

static int sunshine_get_metrics(const struct lens_session *session,
                                struct telescope_metrics *metrics_out) {
    if (!session || !metrics_out) {
        return -EINVAL;
    }
    
    /* Sunshine-specific metrics would be collected here */
    /* For now, return placeholder metrics */
    memset(metrics_out, 0, sizeof(struct telescope_metrics));
    
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    metrics_out->timestamp_us = ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;
    
    /* TODO: Query Sunshine client for actual metrics if API available */
    /* Could parse stdout/stderr or query status endpoint */
    
    return 0;
}

const lens_ops_t sunshine_ops = {
    .create = sunshine_create,
    .start = sunshine_start,
    .stop = sunshine_stop,
    .destroy = sunshine_destroy,
    .get_metrics = sunshine_get_metrics
};

