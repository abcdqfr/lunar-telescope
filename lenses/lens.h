#ifndef LENS_H
#define LENS_H

#include <stdint.h>
#include <stdbool.h>
#include "../core/telescope.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Transport Lens API
 *
 * Abstracts different transport mechanisms (waypipe, sunshine, moonlight)
 * behind a unified interface for session management and optimization.
 */

/* Forward declarations */
struct lens_session;
struct lens_config;

/**
 * Lens operations structure
 */
typedef struct {
    /**
     * Create a new lens session
     *
     * @param config Configuration for the session
     * @param session_out Output session handle
     * @return 0 on success, negative error code on failure
     */
    int (*create)(const struct telescope_config *config,
                 struct lens_session **session_out);
    
    /**
     * Start the lens session (launch remote application)
     *
     * @param session Session handle
     * @return 0 on success, negative error code on failure
     */
    int (*start)(struct lens_session *session);
    
    /**
     * Stop the lens session
     *
     * @param session Session handle
     * @return 0 on success, negative error code on failure
     */
    int (*stop)(struct lens_session *session);
    
    /**
     * Destroy lens session
     *
     * @param session Session handle
     */
    void (*destroy)(struct lens_session *session);
    
    /**
     * Get lens-specific metrics
     *
     * @param session Session handle
     * @param metrics_out Output metrics
     * @return 0 on success, negative error code on failure
     */
    int (*get_metrics)(const struct lens_session *session,
                      struct telescope_metrics *metrics_out);
} lens_ops_t;

/**
 * Lens session handle
 */
struct lens_session {
    telescope_lens_t type;
    const lens_ops_t *ops;
    void *private_data;  /* Lens-specific data */
    pid_t process_pid;
    bool running;
};

/**
 * Get lens operations for a specific lens type
 *
 * @param type Lens type
 * @return Pointer to operations structure, or NULL if not supported
 */
const lens_ops_t *lens_get_ops(telescope_lens_t type);

/**
 * Create lens session
 *
 * @param type Lens type
 * @param config Configuration
 * @param session_out Output session handle
 * @return 0 on success, negative error code on failure
 */
int lens_session_create(telescope_lens_t type,
                       const struct telescope_config *config,
                       struct lens_session **session_out);

/**
 * Start lens session
 */
int lens_session_start(struct lens_session *session);

/**
 * Stop lens session
 */
int lens_session_stop(struct lens_session *session);

/**
 * Destroy lens session
 */
void lens_session_destroy(struct lens_session *session);

/**
 * Get lens metrics
 */
int lens_session_get_metrics(const struct lens_session *session,
                            struct telescope_metrics *metrics_out);

#ifdef __cplusplus
}
#endif

#endif /* LENS_H */

