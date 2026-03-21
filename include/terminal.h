#pragma once
#ifndef TERMINAL_H
#define TERMINAL_H

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
 * Append cursor move to `(row, col)` command to `display`
 */
void move_cursor(StringBuilder *display, uint16_t row, uint16_t col);

/**
 * Enable raw mode for terminal io.
 * Saves settings to restore at program exit.
 */
void enable_raw_mode(void);

#endif
