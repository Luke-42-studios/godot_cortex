/**
 * c_node_internal.c - Pure C implementation
 *
 * This file contains only C code - no C++ features.
 */

#include "c_node_internal.h"
#include <string.h>
#include <stdio.h>

void c_node_init(C_NodeData* data) {
    if (!data) return;

    memset(data->message, 0, C_NODE_MESSAGE_MAX_LEN);
    strncpy(data->message, "Hello from C!", C_NODE_MESSAGE_MAX_LEN - 1);
    data->counter = 0;
    data->speed = 1.0f;
    data->active = false;
}

void c_node_cleanup(C_NodeData* data) {
    if (!data) return;

    /* Nothing to free since we use fixed-size arrays */
    memset(data, 0, sizeof(C_NodeData));
}

void c_node_set_message(C_NodeData* data, const char* message) {
    if (!data || !message) return;

    strncpy(data->message, message, C_NODE_MESSAGE_MAX_LEN - 1);
    data->message[C_NODE_MESSAGE_MAX_LEN - 1] = '\0';
}

const char* c_node_get_message(const C_NodeData* data) {
    if (!data) return "";
    return data->message;
}

int32_t c_node_update(C_NodeData* data, float delta) {
    if (!data || !data->active) return data ? data->counter : 0;

    /* Increment counter based on speed and delta time */
    /* Using integer increment scaled by speed */
    data->counter += (int32_t)(data->speed * 60.0f * delta);

    return data->counter;
}

void c_node_reset_counter(C_NodeData* data) {
    if (!data) return;
    data->counter = 0;
}

void c_node_set_speed(C_NodeData* data, float speed) {
    if (!data) return;
    data->speed = speed;
}

float c_node_get_speed(const C_NodeData* data) {
    if (!data) return 0.0f;
    return data->speed;
}

void c_node_set_active(C_NodeData* data, bool active) {
    if (!data) return;
    data->active = active;
}

bool c_node_is_active(const C_NodeData* data) {
    if (!data) return false;
    return data->active;
}

int32_t c_node_get_counter(const C_NodeData* data) {
    if (!data) return 0;
    return data->counter;
}
