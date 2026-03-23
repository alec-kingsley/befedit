#pragma once
#ifndef BUFFER_H
#define BUFFER_H

#include "terminal.h"
#include <stdint.h>

typedef struct Buffer Buffer;

/**
 * Send command to buffer.
 *
 * `is_simulated` is for if the user is not entering the command, for example if
 * an action is being re-done.
 */
void buffer_cmd(Buffer *self, key_t cmd, bool is_simulated);

/**
 * Display buffer in a region on the screen.
 * Do not overwrite anything else on the screen.
 *
 * (0, 0) represents the top left corner.
 */
void buffer_display(Buffer *self, uint16_t top_offset, uint16_t left_offset,
                    uint16_t row_ct, uint16_t col_ct);

/**
 * Get name of buffer.
 */
char *buffer_name(Buffer *self);

/**
 * Save buffer to file.
 */
void buffer_save(Buffer *self);

/**
 * Return `true` iff `self` has been modified.
 */
bool buffer_is_modified(Buffer *self);

/**
 * Create a new `Buffer` object.
 * Return `NULL` on failure.
 *
 * The contents of `Buffer` will be initialized to be the contents of
 * `filename` if the file exists, else it will be set to empty.
 */
Buffer *buffer_create(const char *filename);

/**
 * Destroy `self`.
 * If `self` is `NULL`, does nothing.
 */
void buffer_destroy(Buffer *self);

#endif