#include "buffer.h"
#include "colors.h"
#include "string_builder.h"
#include "terminal.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define FILENAME "befedit.c"

typedef enum { NORMAL, INSERT, SELECT, COMMAND } mode_t;

static void build_footer(StringBuilder *display, mode_t mode) {
    uint16_t col;
    const char *mode_str = mode == NORMAL   ? "NOR"
                           : mode == INSERT ? "INS"
                                            : "SEL";

    /* fill empty footer */
    move_cursor(display, get_row_ct() - 1, 0);
    string_builder_append(display, HIGHLIGHT);
    for (col = 0; col < get_col_ct(); col++) {
        string_builder_append_char(display, ' ');
    }

    /* write mode */
    move_cursor(display, get_row_ct() - 1, 0);
    string_builder_append(display, mode_str);
    string_builder_append(display, RESET);
}

static void update_screen(Buffer *buffer, mode_t mode) {
    const uint16_t top_offset = 0, left_offset = 0;
    uint16_t row_ct, col_ct;
    StringBuilder *display = string_builder_create();
    update_window_size();
    row_ct = get_row_ct(), col_ct = get_col_ct();

    string_builder_append(display, CLEAR_SCREEN RESET_CURSOR SHOW_CURSOR);
    build_footer(display, mode);

    write(STDOUT_FILENO, string_builder_to_string(display),
          string_builder_len(display));
    string_builder_destroy(display);
    buffer_display(buffer, top_offset, left_offset, row_ct - 2, col_ct);
}

/**
 * Return `true` iff command should cause program to exit.
 */
static bool run_command(Buffer *buffer, const char *cmd) {
    bool should_exit = false;
    if (strcmp(cmd, "w") == 0) {
        buffer_save(buffer);
    } else if (strcmp(cmd, "x") == 0 || strcmp(cmd, "wq") == 0) {
        buffer_save(buffer);
        should_exit = true;
    } else if (strcmp(cmd, "q!") == 0) {
        should_exit = true;
    } else if (strcmp(cmd, "q") == 0) {
        if (!buffer_is_modified(buffer)) {
            should_exit = true;
        }
    }
    return should_exit;
}

static void display_command(const char *cmd) {
    uint16_t col;
    StringBuilder *display = string_builder_create();

    /* fill empty footer */
    move_cursor(display, get_row_ct() - 1, 0);
    string_builder_append(display, HIGHLIGHT);
    for (col = 0; col < get_col_ct(); col++) {
        string_builder_append_char(display, ' ');
    }

    /* write mode */
    move_cursor(display, get_row_ct() - 1, 0);
    string_builder_append_char(display, ':');
    for (col = 1; col < get_col_ct(); col++) {
        if (cmd[col - 1] == '\0') break;
        string_builder_append_char(display, cmd[col - 1]);
    }

    string_builder_append(display, RESET);
    write(STDOUT_FILENO, string_builder_to_string(display),
          string_builder_len(display));
    string_builder_destroy(display);
}

/**
 * Return `true` iff command should cause program to exit.
 */
static bool command_mode(Buffer *buffer) {
    key_t key;
    StringBuilder *cmd = string_builder_create();
    bool keep_running = true;
    bool run_cmd = true;
    bool should_exit = false;
    while (keep_running) {
        update_screen(buffer, COMMAND);
        display_command(string_builder_to_string(cmd));
        key = get_key();
        switch (key) {
        case ESC_KEY:
            run_cmd = false;
            keep_running = false;
            break;
        case '\n': keep_running = false; break;
        default:
            if (key_is_printable(key)) {
                string_builder_append_char(cmd, key);
            }
            break;
        }
    }
    if (run_cmd) {
        should_exit = run_command(buffer, string_builder_to_string(cmd));
    }
    string_builder_destroy(cmd);
    return should_exit;
}

static void insert_mode(Buffer *buffer) {
    key_t key;
    bool keep_running = true;

    while (keep_running) {
        update_screen(buffer, INSERT);
        key = get_key();
        switch (key) {
        case ESC_KEY: keep_running = false; break;
        default: buffer_insert_cmd(buffer, key); break;
        }
    }
}

int main(int argc, char **argv) {
    key_t key;
    bool keep_running = true;
    Buffer *buffer;
    enable_raw_mode();

    /* TODO - read multiple arguments */
    if (argc > 1) {
        buffer = buffer_create(argv[1]);
    } else {
        buffer = buffer_create("2dfile.txt");
    }
    if (!buffer) return 1;

    while (keep_running) {
        update_screen(buffer, NORMAL);
        key = get_key();
        switch (key) {
        case ':': keep_running = !command_mode(buffer); break;
        case 'i': insert_mode(buffer); break;
        default: buffer_normal_cmd(buffer, key); break;
        }
    }
    printf(CLEAR_SCREEN RESET_CURSOR SHOW_CURSOR);

    return 0;
}
