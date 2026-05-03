#pragma once
#ifndef EDITOR_H
#define EDITOR_H

#include "buffer.h"
#include "keystroke.h"
#include "vector.h"

typedef struct Editor Editor;

/**
 * Top left corner position of current selection.
 */
vector_t editor_selection_top_left(Editor *self);

/**
 * Bottom right corner position of current selection.
 */
vector_t editor_selection_bottom_right(Editor *self);

/**
 * Current cursor position.
 */
vector_t editor_cursor(Editor *self);

/**
 * Current momentum.
 */
vector_t editor_momentum(Editor *self);

/**
 * Get char at the currently open buffer's position `pos`.
 */
char editor_get(Editor *self, vector_t pos);

/**
 * Register a macro to be executed by `ctrl-{key}`
 * Executes at position `pos`.
 */
void editor_register_macro(Editor *self, vector_t pos, key_t key);

/**
 * Execute a keystroke in `self`.
 */
bool editor_execute_keystroke(Editor *self, Keystroke *keystroke);

/**
 * Run editor.
 */
void editor_run(Editor *self);

/**
 * Open buffer.
 */
void editor_open(Editor *self, const char *name);

/**
 * Create a new `Editor` object.
 * Return `NULL` on failure.
 */
Editor *editor_create(void);

/**
 * Destroy `self`.
 * If `self` is `NULL`, does nothing.
 */
void editor_destroy(Editor *self);

#endif
