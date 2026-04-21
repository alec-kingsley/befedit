#pragma once
#ifndef ACTION_H
#define ACTION_H

#include "direction.h"
#include "keystroke.h"
#include "vector.h"
#include <stdint.h>

typedef struct Action Action;

/**
 * Get keystroke.
 */
Keystroke *action_get_keystroke(Action *self);

/**
 * Get pos.
 */
vector_t action_get_pos(Action *self);

/**
 * Set pos.
 * Action does NOT own `pos`.
 */
void action_set_pos(Action *self, vector_t *pos);

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
 * Action does NOT own `pos`.
 *
 * `pos.y` and `pos.x` can be negative in the case of being off-screen.
 */
Action *action_create(Keystroke *keystroke, vector_t *pos,
                      direction_t momentum);

/**
 * Destroy `self`.
 * If `self` is `NULL`, does nothing.
 */
void action_destroy(Action *self);

#endif
