#pragma once
#ifndef KEY_H
#define KEY_H

#include <stdbool.h>
#include <stdint.h>

typedef uint16_t key_t;

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

/**
 * Return `true` iff `key` is printable
 * as a char.
 */
bool key_is_printable(key_t key);

/**
 * Print key, or name of key if not printable.
 */
void print_key(key_t key);

#endif
