#pragma once
#ifndef EDITOR_H
#define EDITOR_H

#include "buffer.h"
#include "keystroke.h"
#include "vector.h"

typedef struct Editor Editor;

/**
 * Register a macro to be executed by `ctrl-{key}`
 * Executes at position `pos`.
 */
void editor_registor_macro(Editor *self, vector_t pos, key_t key);

/**
 * Execute a keystroke in `self`.
 */
bool editor_execute_keystroke(Editor *self, Keystroke *keystroke);

/**
 * Run editor.
 */
void editor_run(Editor *self);

/**
 * Add buffer to `self`.
 */
void editor_add_buffer(Editor *self, Buffer *buffer);

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
