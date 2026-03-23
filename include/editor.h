#pragma once
#ifndef EDITOR_H
#define EDITOR_H

#include "buffer.h"

typedef struct Editor Editor;

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
