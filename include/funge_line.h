#pragma once
#ifndef FUNGE_LINE_H
#define FUNGE_LINE_H

#include "funge_stack.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct FungeLine FungeLine;

/**
 * Return length of `funge_line`.
 */
size_t funge_line_len(FungeLine *self);

/**
 * Set the char at the specified index.
 * If `index` is negative, it starts from the end, where -1 is the last char.
 */
void funge_line_set(FungeLine *self, int32_t index, funge_cell_t c);

/**
 * Get the char at the specified index.
 * If `index` is negative, it starts from the end, where -1 is the last char.
 */
funge_cell_t funge_line_get(FungeLine *self, int32_t index);

/**
 * Insert `other` to `funge_line` at index `index`.
 * Return `true` on success.
 */
bool funge_line_insert(FungeLine *self, size_t index, funge_cell_t other);

/**
 * Append `other` to `funge_line`.
 * Return `true` on success.
 */
bool funge_line_append(FungeLine *self, funge_cell_t other);

/**
 * Create a new `FungeLine` object.
 * Return `NULL` on failure.
 */
FungeLine *funge_line_create(void);

/**
 * Destroy `self`.
 * If `self` is `NULL`, does nothing.
 */
void funge_line_destroy(FungeLine *self);

#endif
