#pragma once
#ifndef EDITOR_H
#define EDITOR_H

#include "keystroke.h"
#include "buffer.h"

typedef struct Editor Editor;

/**
 * Register a macro to be executed by `ctrl-{key}`
 * Prereq - `key` is a capital letter.
 */
void editor_registor_macro(Editor *self, Keystroke *macro, key_t key);

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
