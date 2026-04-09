#pragma once
#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>
#include <stdlib.h>

/**
 * Queue data structure.
 */
typedef struct Queue Queue;

/**
 * Get the length of the `Queue`.
 */
size_t queue_len(Queue *self);

/**
 * Return true iff `queue` is empty.
 */
bool queue_is_empty(Queue *self);

/**
 * Enqueue an item to the top of the `Queue`.
 * Return `true` iff successful.
 */
bool queue_enqueue(Queue *self, void *val);

/**
 * Retrieve an element from the top of the `queue` and remove it.
 */
void *queue_dequeue(Queue *self);

/**
 * Retrieve an element from the top of `self`.
 * Does not modify `self`.
 */
void *queue_peek(Queue *self);

/**
 * Create a new `Queue`.
 * Return `NULL` on failure.
 *
 * If `free_fun` is null, it will not be called.
 */
Queue *queue_create(void (*free_fun)(void *));

/**
 * Destroy the `Queue`.
 */
void queue_destroy(Queue *self);

#endif
