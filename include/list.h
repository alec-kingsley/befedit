#pragma once
#ifndef LIST_H
#define LIST_H

#include <stdbool.h>
#include <stdlib.h>

/**
 * List data structure.
 */
typedef struct List List;

/**
 * Get the length of the `List`.
 */
size_t list_len(List *self);

/**
 * Return true iff `list` is empty.
 */
bool list_is_empty(List *self);

/**
 * Insert `val` into `self`.
 * Return `true` iff successful.
 */
bool list_insert(List *self, void *val, size_t index);

/**
 * Retrieve an element from position `index` in `self`.
 */
void *list_get(List *self, size_t index);

/**
 * Retrieve an element from `self` and remove it.
 */
void *list_remove(List *self, size_t index);

/**
 * Create a new `List`.
 * Return `NULL` on failure.
 *
 * If `free_fun` is null, it will not be called.
 */
List *list_create(void (*free_fun)(void *));

/**
 * Destroy the `List`.
 */
void list_destroy(List *self);

#endif
