#pragma once
#ifndef FUNGE_SPACE_H
#define FUNGE_SPACE_H

#include "funge_stack.h"
#include "vector.h"

typedef struct FungeSpace FungeSpace;

/**
 * Put a value in funge space.
 */
void funge_space_put(FungeSpace *self, vector_t pos, funge_cell_t n);

/**
 * Get a value from funge space.
 */
funge_cell_t funge_space_get(FungeSpace *self, vector_t pos);

/**
 * Get top left corner of funge space.
 */
vector_t funge_space_top_left(FungeSpace *self);

/**
 * Get bottom right corner of funge space.
 */
vector_t funge_space_bottom_right(FungeSpace *self);

/**
 * Create a funge space, loaded from file with name `fname`.
 * Return NULL on failure.
 */
FungeSpace *funge_space_create(const char *fname);

/**
 * Destroy `self`.
 * Does nothing if `self` is NULL.
 */
void funge_space_destroy(FungeSpace *self);

#endif
