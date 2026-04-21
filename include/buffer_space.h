#pragma once
#include <stdbool.h>
#ifndef BUFFER_SPACE_H
#define BUFFER_SPACE_H

#include "vector.h"
#include <stdio.h>

typedef struct BufferSpace BufferSpace;

/**
 * Get a reference to a vector in the buffer space that will update as lines
 * are added and removed.
 */
vector_t *buffer_space_get_coordinate(BufferSpace *self, vector_t pos);

/**
 * Clean whitespace at edges in `self`.
 */
void buffer_space_clean_whitespace(BufferSpace *self);

/**
 * Put a value in buffer space.
 */
void buffer_space_put(BufferSpace *self, vector_t pos, char c);

/**
 * Get a value from buffer space.
 */
char buffer_space_get(BufferSpace *self, vector_t pos);

/**
 * Insert/remove row/col from `self`.
 */
void buffer_space_insert_row(BufferSpace *self, int32_t row);
void buffer_space_remove_row(BufferSpace *self, int32_t row);
void buffer_space_insert_col(BufferSpace *self, int32_t col);
void buffer_space_remove_col(BufferSpace *self, int32_t col);

/**
 * Write buffer space to file.
 */
void buffer_space_write(BufferSpace *self, FILE *file);

/**
 * Get top left corner of buffer space.
 */
vector_t buffer_space_top_left(BufferSpace *self);

/**
 * Get bottom right corner of buffer space.
 */
vector_t buffer_space_bottom_right(BufferSpace *self);

/**
 * `true` iff `self` is a new file.
 */
bool buffer_space_is_new_file(BufferSpace *self);

/**
 * Create a buffer space, loaded from buffer with name `fname`.
 * Return NULL on failure.
 */
BufferSpace *buffer_space_create(const char *fname);

/**
 * Destroy `self`.
 * Does nothing if `self` is NULL.
 */
void buffer_space_destroy(BufferSpace *self);

#endif
