#pragma once
#ifndef COMMAND_H
#define COMMAND_H

#include <stdlib.h>

typedef enum {
    WRITE,
    WRITE_QUIT,
    QUIT,
    FORCE_QUIT,
    WRITE_ALL,
    WRITE_QUIT_ALL,
    QUIT_ALL,
    FORCE_QUIT_ALL,
    NEXT,
    PREVIOUS,
    CLEAN_WHITESPACE,
    OPEN,
    CONFIG_OPEN,
    CONFIG_RELOAD,
    EMPTY,  /* no command given */
    UNKNOWN /* unrecognized command */
} command_t;

typedef struct Command Command;

/**
 * Get # of arguments (includes command itself, like argc)
 */
size_t command_arg_ct(Command *self);

/**
 * Get argument from command.
 * command_arg(self, 0) is the command itself.
 * Do not free. `self` still owns it.
 */
char *command_get_arg(Command *self, size_t index);

/**
 * Get base command.
 */
command_t command_get_command(Command *self);

/**
 * Create a new `Command` object.
 * Return `NULL` on failure.
 */
Command *command_create(const char *cmd);

/**
 * Destroy `self`.
 * If `self` is `NULL`, does nothing.
 */
void command_destroy(Command *self);

#endif
