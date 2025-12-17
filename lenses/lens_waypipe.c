#include "lens.h"
#include "../core/telescope.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>

/**
 * Waypipe lens implementation
 *
 * Provides waypipe transport with protocol correctness and low overhead.
 */

struct waypipe_session {
    struct telescope_config *config;
    pid_t waypipe_pid;
    bool running;
    uint64_t start_time_us;
};

static int waypipe_create(const struct telescope_config *config,
                         struct lens_session **session_out) {
    if (!config || !session_out) {
        return -EINVAL;
    }
    
    struct waypipe_session *ws = calloc(1, sizeof(struct waypipe_session));
    if (!ws) {
        return -ENOMEM;
    }
    
    ws->config = (struct telescope_config *)config;
    ws->waypipe_pid = -1;
    ws->running = false;
    
    struct lens_session *session = calloc(1, sizeof(struct lens_session));
    if (!session) {
        free(ws);
        return -ENOMEM;
    }
    
    session->type = TELESCOPE_LENS_WAYPIPE;
    session->ops = lens_get_ops(TELESCOPE_LENS_WAYPIPE);
    session->private_data = ws;
    session->process_pid = -1;
    session->running = false;
    
    *session_out = session;
    return 0;
}

static int build_waypipe_argv(const struct telescope_config *config,
                              char ***argv_out, size_t *argc_out) {
    const telescope_connection_t *conn = &config->connection;
    const telescope_application_t *app = &config->application;
    
    size_t max_args = 32;
    char **argv = calloc(max_args, sizeof(char*));
    if (!argv) {
        return -ENOMEM;
    }
    
    size_t argc = 0;
    
    argv[argc++] = strdup("waypipe");
    argv[argc++] = strdup("client");
    
    if (conn->compression && strcmp(conn->compression, "none") != 0) {
        char *compress_opt = malloc(strlen(conn->compression) + 12);
        if (!compress_opt) {
            goto error;
        }
        snprintf(compress_opt, strlen(conn->compression) + 12, "--compress=%s", conn->compression);
        argv[argc++] = compress_opt;
    }
    
    if (conn->video_codec) {
        char *codec_opt = malloc(strlen(conn->video_codec) + 14);
        if (!codec_opt) {
            goto error;
        }
        snprintf(codec_opt, strlen(conn->video_codec) + 14, "--video-codec=%s", conn->video_codec);
        argv[argc++] = codec_opt;
    }
    
    argv[argc++] = strdup("--ssh");
    
    char *ssh_target = malloc(strlen(conn->ssh_user) + strlen(conn->remote_host) + 2);
    if (!ssh_target) {
        goto error;
    }
    snprintf(ssh_target, strlen(conn->ssh_user) + strlen(conn->remote_host) + 2,
             "%s@%s", conn->ssh_user, conn->remote_host);
    argv[argc++] = ssh_target;
    
    argv[argc++] = strdup("--");
    argv[argc++] = strdup(app->executable);
    
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

static void free_waypipe_argv(char **argv, size_t argc) {
    if (!argv) {
        return;
    }
    for (size_t i = 0; i < argc; i++) {
        free(argv[i]);
    }
    free(argv);
}

static int waypipe_start(struct lens_session *session) {
    if (!session || !session->private_data) {
        return -EINVAL;
    }
    
    struct waypipe_session *ws = (struct waypipe_session *)session->private_data;
    
    if (ws->running) {
        return -EBUSY;
    }
    
    char **waypipe_argv = NULL;
    size_t waypipe_argc = 0;
    int ret = build_waypipe_argv(ws->config, &waypipe_argv, &waypipe_argc);
    if (ret < 0) {
        return ret;
    }
    
    pid_t pid = fork();
    if (pid < 0) {
        free_waypipe_argv(waypipe_argv, waypipe_argc);
        return -errno;
    }
    
    if (pid == 0) {
        if (ws->config->application.env_count > 0) {
            for (size_t i = 0; i < ws->config->application.env_count; i++) {
                putenv(ws->config->application.env[i]);
            }
        }
        
        if (ws->config->application.working_directory) {
            chdir(ws->config->application.working_directory);
        }
        
        execvp("waypipe", waypipe_argv);
        _exit(127);
    }
    
    ws->waypipe_pid = pid;
    session->process_pid = pid;
    
    free_waypipe_argv(waypipe_argv, waypipe_argc);
    
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    ws->start_time_us = ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;
    ws->running = true;
    session->running = true;
    
    return 0;
}

static int waypipe_stop(struct lens_session *session) {
    if (!session || !session->private_data) {
        return -EINVAL;
    }
    
    struct waypipe_session *ws = (struct waypipe_session *)session->private_data;
    
    if (!ws->running) {
        return 0;
    }
    
    if (ws->waypipe_pid > 0) {
        kill(ws->waypipe_pid, SIGTERM);
        waitpid(ws->waypipe_pid, NULL, 0);
        ws->waypipe_pid = -1;
        session->process_pid = -1;
    }
    
    ws->running = false;
    session->running = false;
    
    return 0;
}

static void waypipe_destroy(struct lens_session *session) {
    if (!session) {
        return;
    }
    
    waypipe_stop(session);
    
    if (session->private_data) {
        free(session->private_data);
    }
    
    free(session);
}

static int waypipe_get_metrics(const struct lens_session *session,
                               struct telescope_metrics *metrics_out) {
    if (!session || !metrics_out) {
        return -EINVAL;
    }
    
    /* Waypipe-specific metrics would be collected here */
    /* For now, return zeroed metrics */
    memset(metrics_out, 0, sizeof(struct telescope_metrics));
    
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    metrics_out->timestamp_us = ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;
    
    return 0;
}

static const lens_ops_t waypipe_ops = {
    .create = waypipe_create,
    .start = waypipe_start,
    .stop = waypipe_stop,
    .destroy = waypipe_destroy,
    .get_metrics = waypipe_get_metrics
};

/* Forward declarations for Sunshine and Moonlight ops */
extern const lens_ops_t sunshine_ops;
extern const lens_ops_t moonlight_ops;

const lens_ops_t *lens_get_ops(telescope_lens_t type) {
    switch (type) {
        case TELESCOPE_LENS_WAYPIPE:
            return &waypipe_ops;
        case TELESCOPE_LENS_SUNSHINE:
            return &sunshine_ops;
        case TELESCOPE_LENS_MOONLIGHT:
            return &moonlight_ops;
        case TELESCOPE_LENS_AUTO:
            /* Auto-select defaults to waypipe */
            return &waypipe_ops;
        default:
            return NULL;
    }
}

int lens_session_create(telescope_lens_t type,
                       const struct telescope_config *config,
                       struct lens_session **session_out) {
    const lens_ops_t *ops = lens_get_ops(type);
    if (!ops || !ops->create) {
        return -ENOTSUP;
    }
    
    return ops->create(config, session_out);
}

int lens_session_start(struct lens_session *session) {
    if (!session || !session->ops || !session->ops->start) {
        return -EINVAL;
    }
    
    return session->ops->start(session);
}

int lens_session_stop(struct lens_session *session) {
    if (!session || !session->ops || !session->ops->stop) {
        return -EINVAL;
    }
    
    return session->ops->stop(session);
}

void lens_session_destroy(struct lens_session *session) {
    if (!session || !session->ops || !session->ops->destroy) {
        return;
    }
    
    session->ops->destroy(session);
}

int lens_session_get_metrics(const struct lens_session *session,
                            struct telescope_metrics *metrics_out) {
    if (!session || !session->ops || !session->ops->get_metrics) {
        return -EINVAL;
    }
    
    return session->ops->get_metrics(session, metrics_out);
}

