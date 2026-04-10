#pragma once
#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <stdbool.h>
#include "editor.h"

typedef struct Interpreter Interpreter;

/**
 * Get output from `self`.
 * Output owned by `self`, so must not be used after `self` is destroyed.
 */
char *interpreter_get_output(Interpreter *self);

/**
 * True iff `self` is had an error.
 */
bool *interpreter_is_poisoned_ref(Interpreter *self);

/**
 * Run interpreter.
 */
void interpreter_run(Interpreter *self);

/**
 * Create a new interpreter.
 * Return NULL if error occured.
 */
Interpreter *interpreter_create(const char *fname, Editor *editor);

/**
 * Destroy the interpreter.
 * Do nothing if `self` is NULL.
 */
void interpreter_destroy(Interpreter *self);

#endif
