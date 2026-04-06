#pragma once
#ifndef ACTION_H
#define ACTION_H

#include "direction.h"
#include "keystroke.h"
#include <stdint.h>

typedef struct Action Action;

/**
 * Get keystroke.
 */
Keystroke *action_get_keystroke(Action *self);

/**
 * Get row.
 */
int32_t action_get_row(Action *self);

/**
 * Set row.
 */
void action_set_row(Action *self, int32_t row);

/**
 * Get column.
 */
int32_t action_get_col(Action *self);

/**
 * Set column.
 */
void action_set_col(Action *self, int32_t col);

/**
 * Get momentum.
 */
direction_t action_get_momentum(Action *self);

/**
 * Create a new `Action` object.
 * Return `NULL` on failure.
 *
 * Ownership of `keystroke` is passed to Action, and Action should be
 * expected to destroy it at `action_destroy`
 *
 * `row` and `column` can be negative in the case of being off-screen.
 */
Action *action_create(Keystroke *keystroke, int32_t row, int32_t col,
                      direction_t momentum);

/**
 * Destroy `self`.
 * If `self` is `NULL`, does nothing.
 */
void action_destroy(Action *self);

#endif
