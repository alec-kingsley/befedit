#pragma once
#ifndef KEYSTROKE_H
#define KEYSTROKE_H

#include "terminal.h"
#include <stdbool.h>

typedef struct Keystroke Keystroke;

/**
 * Get the length of `self`.
 */
size_t keystroke_len(Keystroke *self);

/**
 * Get the key at the specified index.
 * If `index` is negative, it starts from the end, where -1 is the last char.
 */
key_t keystroke_get_key(Keystroke *self, int32_t index);

/**
 * Append a key to `self`.
 * Return `true` on success.
 */
bool keystroke_append_key(Keystroke *self, key_t key);

/**
 * Prepend a key to `self`.
 * Return `true` on success.
 */
bool keystroke_prepend_key(Keystroke *self, key_t key);

/**
 * Create a new `Keystroke` object.
 * Return `NULL` on failure.
 */
Keystroke *keystroke_create(void);

/**
 * Destroy `self`.
 * If `self` is `NULL`, does nothing.
 */
void keystroke_destroy(Keystroke *self);

#endif
