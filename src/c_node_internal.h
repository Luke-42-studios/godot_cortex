/**
 * c_node_internal.h - Pure C data structures and functions
 *
 * This header contains only C code - no C++ features.
 * It can be included from both C and C++ files.
 */

#ifndef C_NODE_INTERNAL_H
#define C_NODE_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* Maximum length for the message string */
#define C_NODE_MESSAGE_MAX_LEN 256

/**
 * C_NodeData - Pure C struct holding node state
 */
typedef struct C_NodeData {
    char message[C_NODE_MESSAGE_MAX_LEN];
    int32_t counter;
    float speed;
    bool active;
} C_NodeData;

/**
 * Initialize node data with defaults
 */
void c_node_init(C_NodeData* data);

/**
 * Cleanup node data
 */
void c_node_cleanup(C_NodeData* data);

/**
 * Set the message string
 */
void c_node_set_message(C_NodeData* data, const char* message);

/**
 * Get the message string (returns pointer to internal buffer)
 */
const char* c_node_get_message(const C_NodeData* data);

/**
 * Update the node (called each frame when active)
 * Returns the new counter value
 */
int32_t c_node_update(C_NodeData* data, float delta);

/**
 * Reset the counter to zero
 */
void c_node_reset_counter(C_NodeData* data);

/**
 * Set the speed multiplier
 */
void c_node_set_speed(C_NodeData* data, float speed);

/**
 * Get the speed multiplier
 */
float c_node_get_speed(const C_NodeData* data);

/**
 * Set active state
 */
void c_node_set_active(C_NodeData* data, bool active);

/**
 * Get active state
 */
bool c_node_is_active(const C_NodeData* data);

/**
 * Get current counter value
 */
int32_t c_node_get_counter(const C_NodeData* data);

#ifdef __cplusplus
}
#endif

#endif /* C_NODE_INTERNAL_H */
