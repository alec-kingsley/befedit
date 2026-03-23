#pragma once
#ifndef STACK_H
#define STACK_H

#include <stdbool.h>
#include <stdlib.h>

/**
 * Stack data structure.
 */
typedef struct Stack Stack;

/**
 * Get the length of the `Stack`.
 */
size_t stack_len(Stack *self);

/**
 * Return true iff `stack` is empty.
 */
bool stack_is_empty(Stack *self);

/**
 * Push an item to the top of the `Stack`.
 * Return `true` iff successful.
 */
bool stack_push(Stack *self, void *val);

/**
 * Retrieve an element from the top of the `stack`.
 */
void *stack_peek(Stack *self);

/**
 * Retrieve an element from position `index` in `self`.
 */
void *stack_get(Stack *self, size_t index);

/**
 * Retrieve an element from the top of the `stack` and remove it.
 */
void *stack_pop(Stack *self);

/**
 * Create a new `Stack`.
 * Return `NULL` on failure.
 *
 * If `free_fun` is null, it will not be called.
 */
Stack *stack_create(void (*free_fun)(void *));

/**
 * Destroy the `Stack`.
 */
void stack_destroy(Stack *self);

#endif
