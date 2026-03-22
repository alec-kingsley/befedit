#pragma once
#ifndef TERMINAL_H
#define TERMINAL_H

#include "string_builder.h"
#include <stdint.h>

#define CTRL(k) ((k) & 0x1f)
enum special_key {
    /* guarantee each special key needs 2bytes */
    ARROW_LEFT = 0x100,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    DEL_KEY,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN
};
#define ESC_KEY 27
#define BACKSPACE 0x7f

typedef uint16_t key_t;

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
 * Return `true` iff `key` is printable.
 */
bool key_is_printable(key_t key);

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
