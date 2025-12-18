#include "telescope.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#if defined(LT_HAVE_JSONC) && (LT_HAVE_JSONC)
#include <json-c/json.h>
#endif

/**
 * Schema validation and JSON parsing for telescope configuration
 */

#if defined(LT_HAVE_JSONC) && (LT_HAVE_JSONC)
static int parse_connection(json_object *obj, telescope_connection_t *conn) {
    json_object *tmp;
    
    if (!json_object_object_get_ex(obj, "remote_host", &tmp)) {
        return -1;
    }
    conn->remote_host = strdup(json_object_get_string(tmp));
    
    if (json_object_object_get_ex(obj, "remote_port", &tmp)) {
        conn->remote_port = json_object_get_int(tmp);
    } else {
        conn->remote_port = 22;
    }
    
    if (json_object_object_get_ex(obj, "ssh_user", &tmp)) {
        conn->ssh_user = strdup(json_object_get_string(tmp));
    } else {
        conn->ssh_user = strdup("root");
    }
    
    if (json_object_object_get_ex(obj, "ssh_key_path", &tmp)) {
        conn->ssh_key_path = strdup(json_object_get_string(tmp));
    } else {
        conn->ssh_key_path = NULL;
    }
    
    if (json_object_object_get_ex(obj, "compression", &tmp)) {
        conn->compression = strdup(json_object_get_string(tmp));
    } else {
        conn->compression = strdup("lz4");
    }
    
    if (json_object_object_get_ex(obj, "video_codec", &tmp)) {
        conn->video_codec = strdup(json_object_get_string(tmp));
    } else {
        conn->video_codec = strdup("h264");
    }
    
    if (json_object_object_get_ex(obj, "bandwidth_limit", &tmp)) {
        conn->bandwidth_limit_mbps = json_object_get_int(tmp);
    } else {
        conn->bandwidth_limit_mbps = 0;
    }
    
    return 0;
}

static int parse_application(json_object *obj, telescope_application_t *app) {
    json_object *tmp;
    json_object *args_array;
    json_object *env_obj;
    
    if (!json_object_object_get_ex(obj, "executable", &tmp)) {
        return -1;
    }
    app->executable = strdup(json_object_get_string(tmp));
    
    if (json_object_object_get_ex(obj, "args", &args_array)) {
        size_t len = json_object_array_length(args_array);
        app->args = calloc(len + 1, sizeof(char*));
        app->args_count = len;
        
        for (size_t i = 0; i < len; i++) {
            json_object *arg = json_object_array_get_idx(args_array, i);
            app->args[i] = strdup(json_object_get_string(arg));
        }
        app->args[len] = NULL;
    } else {
        app->args = NULL;
        app->args_count = 0;
    }
    
    if (json_object_object_get_ex(obj, "env", &env_obj)) {
        json_object_object_foreach(env_obj, key, val) {
            (void)key;
            (void)val;
            app->env_count++;
        }
        
        if (app->env_count > 0) {
            app->env = calloc(app->env_count + 1, sizeof(char*));
            size_t idx = 0;
            json_object_object_foreach(env_obj, key, val) {
                size_t key_len = strlen(key);
                size_t val_len = strlen(json_object_get_string(val));
                app->env[idx] = malloc(key_len + val_len + 2);
                snprintf(app->env[idx], key_len + val_len + 2, "%s=%s", key, json_object_get_string(val));
                idx++;
            }
            app->env[app->env_count] = NULL;
        }
    } else {
        app->env = NULL;
        app->env_count = 0;
    }
    
    if (json_object_object_get_ex(obj, "working_directory", &tmp)) {
        app->working_directory = strdup(json_object_get_string(tmp));
    } else {
        app->working_directory = NULL;
    }
    
    return 0;
}

static int parse_performance(json_object *obj, telescope_performance_t *perf) {
    json_object *tmp;
    
    if (json_object_object_get_ex(obj, "profile", &tmp)) {
        const char *profile_str = json_object_get_string(tmp);
        if (strcmp(profile_str, "low-latency") == 0) {
            perf->profile = TELESCOPE_PROFILE_LOW_LATENCY;
        } else if (strcmp(profile_str, "balanced") == 0) {
            perf->profile = TELESCOPE_PROFILE_BALANCED;
        } else if (strcmp(profile_str, "high-quality") == 0) {
            perf->profile = TELESCOPE_PROFILE_HIGH_QUALITY;
        } else if (strcmp(profile_str, "bandwidth-constrained") == 0) {
            perf->profile = TELESCOPE_PROFILE_BANDWIDTH_CONSTRAINED;
        } else {
            perf->profile = TELESCOPE_PROFILE_BALANCED;
        }
    } else {
        perf->profile = TELESCOPE_PROFILE_BALANCED;
    }
    
    if (json_object_object_get_ex(obj, "target_latency_ms", &tmp)) {
        perf->target_latency_ms = json_object_get_int(tmp);
    } else {
        perf->target_latency_ms = 50;
    }
    
    if (json_object_object_get_ex(obj, "frame_rate", &tmp)) {
        perf->frame_rate = json_object_get_int(tmp);
    } else {
        perf->frame_rate = 60;
    }
    
    if (json_object_object_get_ex(obj, "enable_prediction", &tmp)) {
        perf->enable_prediction = json_object_get_boolean(tmp);
    } else {
        perf->enable_prediction = true;
    }
    
    if (json_object_object_get_ex(obj, "prediction_window_ms", &tmp)) {
        perf->prediction_window_ms = json_object_get_int(tmp);
    } else {
        perf->prediction_window_ms = 16;
    }
    
    if (json_object_object_get_ex(obj, "enable_scroll_smoothing", &tmp)) {
        perf->enable_scroll_smoothing = json_object_get_boolean(tmp);
    } else {
        perf->enable_scroll_smoothing = true;
    }
    
    return 0;
}

static int parse_observability(json_object *obj, telescope_observability_t *obs) {
    json_object *tmp;
    
    if (json_object_object_get_ex(obj, "enable_metrics", &tmp)) {
        obs->enable_metrics = json_object_get_boolean(tmp);
    } else {
        obs->enable_metrics = true;
    }
    
    if (json_object_object_get_ex(obj, "metrics_interval_ms", &tmp)) {
        obs->metrics_interval_ms = json_object_get_int(tmp);
    } else {
        obs->metrics_interval_ms = 1000;
    }
    
    if (json_object_object_get_ex(obj, "metrics_file", &tmp)) {
        obs->metrics_file = strdup(json_object_get_string(tmp));
    } else {
        obs->metrics_file = NULL;
    }
    
    if (json_object_object_get_ex(obj, "log_level", &tmp)) {
        const char *level = json_object_get_string(tmp);
        if (strcmp(level, "error") == 0) {
            obs->log_level = 0;
        } else if (strcmp(level, "warn") == 0) {
            obs->log_level = 1;
        } else if (strcmp(level, "info") == 0) {
            obs->log_level = 2;
        } else if (strcmp(level, "debug") == 0) {
            obs->log_level = 3;
        } else if (strcmp(level, "trace") == 0) {
            obs->log_level = 4;
        } else {
            obs->log_level = 2;
        }
    } else {
        obs->log_level = 2;
    }
    
    return 0;
}

static int parse_lens(json_object *obj, telescope_lens_config_t *lens) {
    json_object *tmp;
    json_object *fallback_array;
    
    if (json_object_object_get_ex(obj, "type", &tmp)) {
        const char *type_str = json_object_get_string(tmp);
        if (strcmp(type_str, "waypipe") == 0) {
            lens->type = TELESCOPE_LENS_WAYPIPE;
        } else if (strcmp(type_str, "sunshine") == 0) {
            lens->type = TELESCOPE_LENS_SUNSHINE;
        } else if (strcmp(type_str, "moonlight") == 0) {
            lens->type = TELESCOPE_LENS_MOONLIGHT;
        } else if (strcmp(type_str, "auto") == 0) {
            lens->type = TELESCOPE_LENS_AUTO;
        } else {
            lens->type = TELESCOPE_LENS_AUTO;
        }
    } else {
        lens->type = TELESCOPE_LENS_AUTO;
    }
    
    if (json_object_object_get_ex(obj, "fallback", &fallback_array)) {
        size_t len = json_object_array_length(fallback_array);
        lens->fallback = calloc(len, sizeof(telescope_lens_t));
        lens->fallback_count = len;
        
        for (size_t i = 0; i < len; i++) {
            json_object *item = json_object_array_get_idx(fallback_array, i);
            const char *item_str = json_object_get_string(item);
            if (strcmp(item_str, "waypipe") == 0) {
                lens->fallback[i] = TELESCOPE_LENS_WAYPIPE;
            } else if (strcmp(item_str, "sunshine") == 0) {
                lens->fallback[i] = TELESCOPE_LENS_SUNSHINE;
            } else if (strcmp(item_str, "moonlight") == 0) {
                lens->fallback[i] = TELESCOPE_LENS_MOONLIGHT;
            }
        }
    } else {
        lens->fallback = NULL;
        lens->fallback_count = 0;
    }
    
    return 0;
}

int telescope_config_load(const char *config_path, struct telescope_config **config_out) {
    json_object *root;
    json_object *tmp;
    struct telescope_config *config;
    
    if (!config_path || !config_out) {
        return -EINVAL;
    }
    
    root = json_object_from_file(config_path);
    if (!root) {
        return -errno;
    }
    
    config = calloc(1, sizeof(struct telescope_config));
    if (!config) {
        json_object_put(root);
        return -ENOMEM;
    }
    
    /* Parse connection */
    if (!json_object_object_get_ex(root, "connection", &tmp)) {
        free(config);
        json_object_put(root);
        return -EINVAL;
    }
    if (parse_connection(tmp, &config->connection) < 0) {
        free(config);
        json_object_put(root);
        return -EINVAL;
    }
    
    /* Parse application */
    if (!json_object_object_get_ex(root, "application", &tmp)) {
        free(config);
        json_object_put(root);
        return -EINVAL;
    }
    if (parse_application(tmp, &config->application) < 0) {
        free(config);
        json_object_put(root);
        return -EINVAL;
    }
    
    /* Parse performance (optional) */
    if (json_object_object_get_ex(root, "performance", &tmp)) {
        parse_performance(tmp, &config->performance);
    } else {
        config->performance.profile = TELESCOPE_PROFILE_BALANCED;
        config->performance.target_latency_ms = 50;
        config->performance.frame_rate = 60;
        config->performance.enable_prediction = true;
        config->performance.prediction_window_ms = 16;
        config->performance.enable_scroll_smoothing = true;
    }
    
    /* Parse observability (optional) */
    if (json_object_object_get_ex(root, "observability", &tmp)) {
        parse_observability(tmp, &config->observability);
    } else {
        config->observability.enable_metrics = true;
        config->observability.metrics_interval_ms = 1000;
        config->observability.metrics_file = NULL;
        config->observability.log_level = 2;
    }
    
    /* Parse lens (optional) */
    if (json_object_object_get_ex(root, "lens", &tmp)) {
        parse_lens(tmp, &config->lens);
    } else {
        config->lens.type = TELESCOPE_LENS_AUTO;
        config->lens.fallback = NULL;
        config->lens.fallback_count = 0;
    }
    
    json_object_put(root);
    *config_out = config;
    return 0;
}
#else
int telescope_config_load(const char *config_path, struct telescope_config **config_out) {
    (void)config_path;
    (void)config_out;
    /* Built without json-c support. */
    return -ENOTSUP;
}
#endif

void telescope_config_free(struct telescope_config *config) {
    if (!config) {
        return;
    }
    
    free(config->connection.remote_host);
    free(config->connection.ssh_user);
    free(config->connection.ssh_key_path);
    free(config->connection.compression);
    free(config->connection.video_codec);
    
    free(config->application.executable);
    if (config->application.args) {
        for (size_t i = 0; i < config->application.args_count; i++) {
            free(config->application.args[i]);
        }
        free(config->application.args);
    }
    if (config->application.env) {
        for (size_t i = 0; i < config->application.env_count; i++) {
            free(config->application.env[i]);
        }
        free(config->application.env);
    }
    free(config->application.working_directory);
    
    free(config->observability.metrics_file);
    
    if (config->lens.fallback) {
        free(config->lens.fallback);
    }
    
    free(config);
}

