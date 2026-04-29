#pragma once
#ifndef TERMINAL_H
#define TERMINAL_H

#include "key.h"
#include "string_builder.h"
#include <stdint.h>

/**
 * Read terminal window size.
 * Must be called before `get_row_ct` or `get_col_ct`.
 */
void update_window_size(void);

/**
 * Get # of rows in terminal.
 */
uint16_t get_row_ct(void);

/**
 * Get # of columns in terminal.
 */
uint16_t get_col_ct(void);

/**
 * Get a key press.
 */
key_t get_key(void);

/**
 * Flush stdin.
 */
void flush_stdin(void);

/**
 * Append cursor move to `(row, col)` command to `display`
 * Top left is (1, 1)
 */
void move_cursor(StringBuilder *display, uint16_t row, uint16_t col);

/**
 * Enable raw mode for terminal io.
 * Saves settings to restore at program exit.
 */
void enable_raw_mode(void);

/**
 * Append an error to the terminal to output at exit.
 */
void append_terminal_error(const char *error);

#endif
