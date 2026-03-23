#include "terminal.h"
#include "colors.h"
#include "reporter.h"
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#define FILENAME "terminal.c"

struct {
    uint16_t row_ct;
    uint16_t col_ct;
    struct termios orig_termios;
} g_term;

uint16_t get_row_ct(void) {
    return g_term.row_ct;
}

uint16_t get_col_ct(void) {
    return g_term.col_ct;
}

void update_window_size(void) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        report_system_error(FILENAME ": failed to get window size");
        exit(1);
    }

    g_term.col_ct = ws.ws_col;
    g_term.row_ct = ws.ws_row;
}

bool key_is_printable(key_t key) {
    return key < 0x80 && key != BACKSPACE && key > 0x1f;
}

static key_t get_key_seq(void) {
    char seq[3];
    if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
    if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';
    if (seq[0] == '[') {
        if (seq[1] >= '0' && seq[1] <= '9') {
            if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
            if (seq[2] == '~') {
                switch (seq[1]) {
                case '1': return HOME_KEY;
                case '3': return DEL_KEY;
                case '4': return END_KEY;
                case '5': return PAGE_UP;
                case '6': return PAGE_DOWN;
                case '7': return HOME_KEY;
                case '8': return END_KEY;
                }
            }
        } else {
            switch (seq[1]) {
            case 'A': return ARROW_UP;
            case 'B': return ARROW_DOWN;
            case 'C': return ARROW_RIGHT;
            case 'D': return ARROW_LEFT;
            case 'H': return HOME_KEY;
            case 'F': return END_KEY;
            }
        }
    } else if (seq[0] == 'O') {
        switch (seq[1]) {
        case 'H': return HOME_KEY;
        case 'F': return END_KEY;
        }
    }
    return '\x1b';
}

key_t get_key(void) {
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) {
            report_system_error(FILENAME ": failed to get user input");
            exit(1);
        }
    }
    if (c == '\x1b') {
        return get_key_seq();
    } else {
        return c;
    }
}

void move_cursor(StringBuilder *display, uint16_t row, uint16_t col) {
    char fmt[16];
    sprintf(fmt, "\x1b[%hu;%huH", row, col);
    string_builder_append(display, fmt);
}

static void disable_raw_mode(void) {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_term.orig_termios) == -1) {
        printf(CLEAR_SCREEN RESET_CURSOR SHOW_CURSOR);
        report_system_error(FILENAME ": failed to disable raw mode");
        exit(1);
    }
}

void enable_raw_mode(void) {
    struct termios raw;

    if (tcgetattr(STDIN_FILENO, &g_term.orig_termios) == -1) {
        report_system_error(FILENAME ": failed to get terminal attributes");
        exit(1);
    }
    atexit(disable_raw_mode);

    raw = g_term.orig_termios;

    /* disable ctrl+s and ctrl+q */
    raw.c_iflag &= ~IXON;

    /* ensure 8th bit preserved */
    raw.c_iflag &= ~ISTRIP;

    /* disable ctrl+v */
    raw.c_lflag &= ~IEXTEN;

    /* disable showing charcters as they're typed */
    raw.c_lflag &= ~ECHO;

    /* disable ctrl+c and ctrl+z */
    raw.c_lflag &= ~ISIG;

    /* disable canonical mode (read byte-by-byte) */
    raw.c_lflag &= ~ICANON;

    /* disable output processing */
    raw.c_oflag &= ~OPOST;

    /* ensure 8 bits per character */
    raw.c_cflag |= CS8;

    /* set minimum bytes to read for read() to return */
    raw.c_cc[VMIN] = 0;

    /* time in deciseconds before read() should return */
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        report_system_error(FILENAME ": failed to enter raw mode");
        exit(1);
    }
}
