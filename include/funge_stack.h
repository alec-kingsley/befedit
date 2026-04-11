#pragma once
#ifndef FUNGE_STACK_H
#define FUNGE_STACK_H

#include "funge_cell.h"
#include <stdbool.h>
#include <stdlib.h>

typedef struct FungeStack FungeStack;

/**
 * Pop cell from funge stack.
 */
funge_cell_t funge_stack_pop(FungeStack *self);

/**
 * Get a cell from the stack.
 */
funge_cell_t funge_stack_get(FungeStack *self, size_t i);

/**
 * Push cell to funge stack.
 * Return `true` iff successful.
 */
bool funge_stack_push(FungeStack *self, funge_cell_t cell);

/**
 * Get size of funge stack.
 */
size_t funge_stack_size(FungeStack *self);

/**
 * Create a clone of `self`.
 */
FungeStack *funge_stack_clone(FungeStack *self);

/**
 * Create a new funge stack.
 * Return NULL if error occured.
 */
FungeStack *funge_stack_create(void);

/**
 * Destroy the funge stack.
 * Do nothing if `self` is NULL.
 */
void funge_stack_destroy(FungeStack *self);

#endif
