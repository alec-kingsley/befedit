#pragma once
#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "editor.h"
#include <stdbool.h>

typedef struct Interpreter Interpreter;

/**
 * Get output from `self`.
 * Output owned by `self`, so must not be used after `self` is destroyed.
 */
char *interpreter_get_output(Interpreter *self);

/**
 * True iff `self` is had an error.
 */
volatile bool *interpreter_is_poisoned_ref(Interpreter *self);

/**
 * Run interpreter.
 */
int interpreter_run(Interpreter *self);

/**
 * Spawn an IP for a macro defined by the BFDT fingerprint.
 */
void interpreter_spawn_macro_ip(Interpreter *self, vector_t pos);

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
