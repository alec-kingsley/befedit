#pragma once
#ifndef BUFFER_H
#define BUFFER_H

#include "direction.h"
#include "key.h"
#include "keystroke.h"
#include "string_builder.h"
#include "vector.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum { NORMAL, INSERT, SELECT, COMMAND } mode_t;

typedef struct Buffer Buffer;

/**
 * Top left corner position of current selection.
 */
vector_t buffer_selection_top_left(Buffer *self);

/**
 * Bottom right corner position of current selection.
 */
vector_t buffer_selection_bottom_right(Buffer *self);

/**
 * Current cursor position.
 */
vector_t buffer_cursor(Buffer *self);

/**
 * Get char at the currently open buffer's position `pos`.
 */
char buffer_get(Buffer *self, vector_t pos);

/**
 * Send command to buffer.
 *
 * `is_simulated` is for if the user is not entering the command, for example if
 * an action is being re-done.
 */
void buffer_cmd(Buffer *self, key_t cmd, bool is_simulated);

/**
 * Remove extraneous whitespace at file edges.
 */
void buffer_clean_whitespace(Buffer *self);

/**
 * Display buffer in a region on the screen.
 * Do not overwrite anything else on the screen.
 *
 * (0, 0) represents the top left corner.
 */
void buffer_build_display(Buffer *self, StringBuilder *display,
                          uint16_t top_offset, uint16_t left_offset,
                          uint16_t row_ct, uint16_t col_ct);

/**
 * Get current 1-indexed row.
 */
size_t buffer_get_row(Buffer *self);

/**
 * Get current 1-indexed column.
 */
size_t buffer_get_col(Buffer *self);

/**
 * Get current buffer momentum.
 */
direction_t buffer_get_momentum(Buffer *self);

/**
 * Get name of buffer.
 */
char *buffer_name(Buffer *self);

/**
 * Save buffer to file.
 * Return `true` on success.
 */
bool buffer_save(Buffer *self);

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
Buffer *buffer_create(const char *filename, Keystroke **yanked);

/**
 * Destroy `self`.
 * If `self` is `NULL`, does nothing.
 */
void buffer_destroy(Buffer *self);

#endif