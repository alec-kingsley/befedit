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
 * Set the char at the specified index.
 * If `index` is negative, it starts from the end, where -1 is the last char.
 */
void string_builder_set_char(StringBuilder *self, int32_t index, char c);

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
 * Insert `other` to `string_builder` at index `index`.
 * Return `true` on success.
 */
bool string_builder_insert_char(StringBuilder *self, size_t index, char);

/**
 * Insert `other` to `string_builder` at index `index`.
 * Return `true` on success.
 */
bool string_builder_insert(StringBuilder *self, size_t index, const char *other);

/**
 * Append `other` to `string_builder`.
 * Return `true` on success.
 */
bool string_builder_append_char(StringBuilder *self, char other);

/**
 * Append `other` to `string_builder`.
 * Return `true` on success.
 */
bool string_builder_append(StringBuilder *self, const char *other);

/**
 * Append `other` to `string_builder`.
 * `other` must have length `n`.
 * This is for if `other` may contain null bytes.
 * Return `true` on success.
 */
bool string_builder_append_bytes(StringBuilder *self, const char *other, size_t n);

/**
 * Restrict the range of the `string_builder` to [start, end).
 * If `end` is less than 1, then it will be places from the end.
 * For example, `string_builder_restrict(original, 0, 0)` would do
 * nothing to `original`.
 *
 * Prerequisites:
 * `start` < |string_builder|
 * `end` <= |string_builder| OR `end` > -|string_builder|
 */
void string_builder_restrict(StringBuilder *self, size_t start, int32_t end);

/**
 * Remove a character range from `self`.
 * Includes `start` not `end`.
 */
void string_builder_remove_range(StringBuilder *self, size_t start, size_t end);

/**
 * Safely print `self`.
 */
void string_builder_print(StringBuilder *self);

/**
 * Create a new `StringBuilder` object.
 * Return `NULL` on failure.
 */
StringBuilder *string_builder_create(void);

/**
 * Destroy `self`.
 * If `self` is `NULL`, does nothing.
 */
void string_builder_destroy(StringBuilder *self);

#endif
