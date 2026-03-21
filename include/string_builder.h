#pragma once
#ifndef STRING_BUILDER_H
#define STRING_BUILDER_H
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct StringBuilder StringBuilder;

/**
 * Return length of `string_builder`.
 */
size_t string_builder_len(StringBuilder *self);

/**
 * Return string contained by `string_builder`.
 */
char *string_builder_to_string(StringBuilder *self);

/**
 * Get the char at the specified index.
 * If `index` is negative, it starts from the end, where -1 is the last char.
 */
char string_builder_get_char(StringBuilder *self, int32_t index);

/**
 * Set `string_builder`'s value to `new`.
 * Return `true` on success.
 */
bool string_builder_set(StringBuilder *self, const char *new);

/**
 * Append `other` to `string_builder`.
 * Return `true` on success.
 */
bool string_builder_append(StringBuilder *self, const char *other);

/**
 * Create a new StringBuilder.
 * Return NULL if error occured.
 */
StringBuilder *string_builder_create(void);

/**
 * Destroy the StringBuilder.
 */
void string_builder_destroy(StringBuilder *self);

#endif
