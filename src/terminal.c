#include "terminal.h"
#include "colors.h"
#include "key.h"
#include "reporter.h"
#include "string_builder.h"
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#define FILENAME "terminal.c"

StringBuilder *g_error_buffer = NULL;

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
        /* try again with stdin */
        if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
            report_system_error(FILENAME ": failed to get window size");
            exit(1);
        }
    }

    g_term.col_ct = ws.ws_col;
    g_term.row_ct = ws.ws_row;
}

/**
 * backlog of extraneous characters to be read next.
 * this happens when `esc` causes a look-ahead, and it turns
 * out not to be a meaningful character sequence.
 */
char g_buf[3];
uint8_t g_buf_len = 0;

static void fill_g_buf(void) {
    while (g_buf_len < sizeof(g_buf) / sizeof(g_buf[0])) {
        if (read(STDIN_FILENO, &g_buf[g_buf_len], 1) != 1) return;
        g_buf_len++;
    }
}

/* remove `count` chars from start of g_buf */
static void g_buf_remove(uint8_t count) {
    uint8_t i;
    for (i = 0; i < sizeof(g_buf) / sizeof(g_buf[0]) - count; i++) {
        g_buf[i] = g_buf[i + count];
    }
    g_buf_len -= count;
}

static key_t get_key_seq(void) {
    key_t result = ESC_KEY;
    uint8_t seq_len = 2;

    fill_g_buf();

    if (g_buf_len > 1 && g_buf[0] == '[') {
        if (g_buf[1] >= '0' && g_buf[1] <= '9') {
            if (g_buf_len > 2 && g_buf[2] == '~') {
                seq_len = 3;
                if (g_buf[2] == '~') {
                    switch (g_buf[1]) {
                    case '1': result = HOME_KEY; break;
                    case '3': result = DEL_KEY; break;
                    case '4': result = END_KEY; break;
                    case '5': result = PAGE_UP; break;
                    case '6': result = PAGE_DOWN; break;
                    case '7': result = HOME_KEY; break;
                    case '8': result = END_KEY; break;
                    }
                }
            }
        } else {
            switch (g_buf[1]) {
            case 'A': result = ARROW_UP; break;
            case 'B': result = ARROW_DOWN; break;
            case 'C': result = ARROW_RIGHT; break;
            case 'D': result = ARROW_LEFT; break;
            case 'H': result = HOME_KEY; break;
            case 'F': result = END_KEY; break;
            }
        }
    } else if (g_buf[0] == 'O') {
        switch (g_buf[1]) {
        case 'H': result = HOME_KEY; break;
        case 'F': result = END_KEY; break;
        }
    }
    if (result != ESC_KEY) {
        g_buf_remove(seq_len);
    }
    return result;
}

key_t get_key(void) {
    int nread;
    char c;
    if (g_buf_len != 0) {
        c = g_buf[0];
        g_buf_remove(1);

    } else {
        while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
            if (nread == -1 && errno != EAGAIN) {
                report_system_error(FILENAME ": failed to get user input");
                exit(1);
            }
        }
    }
    if (c == ESC_KEY) {
        return get_key_seq();
    } else {
        return c;
    }
}

void flush_stdin(void) {
    char c;
    while (read(STDIN_FILENO, &c, 1) == 1);    
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
    }
    if (g_error_buffer) {
        printf(CLEAR_SCREEN RESET_CURSOR SHOW_CURSOR);
        fflush(stdout);
        string_builder_print_error(g_error_buffer);
        string_builder_destroy(g_error_buffer);
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

void append_terminal_error(const char *error) {
    if (!g_error_buffer) {
        g_error_buffer = string_builder_create();
    }
    string_builder_append(g_error_buffer, error);
}
