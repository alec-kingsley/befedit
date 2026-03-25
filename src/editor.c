#include "editor.h"
#include "buffer.h"
#include "colors.h"
#include "list.h"
#include "reporter.h"
#include "string_builder.h"
#include "terminal.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define FILENAME "editor.c"

struct Editor {
    /* Current buffer. Copy of address in `buffers`, but saved to improve speed
     */
    Buffer *buffer;
    List *buffers; /* List<Buffer> */
    size_t buffer_idx;
    StringBuilder *status_message;
    bool status_message_is_error;
};

static void reset_status_message(Editor *self) {
    string_builder_set(self->status_message, "");
    self->status_message_is_error = false;
}

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

static void build_status_message(StringBuilder *display,
                                 const char *status_message,
                                 bool status_message_is_error) {
    const uint16_t row_ct = get_row_ct(), col_ct = get_col_ct();
    uint16_t i;
    move_cursor(display, row_ct, 0);
    if (status_message_is_error) {
        string_builder_append(display, RED);
    }
    if (strlen(status_message) > col_ct) {
        for (i = 0; i < col_ct - strlen("..."); i++) {
            string_builder_append_char(display, status_message[i]);
        }
        string_builder_append(display, "...");
    } else {
        string_builder_append(display, status_message);
    }
    if (status_message_is_error) {
        string_builder_append(display, RESET);
    }
}

static void update_screen(Editor *self, mode_t mode) {
    const uint16_t top_offset = 0, left_offset = 0;
    uint16_t row_ct, col_ct;
    StringBuilder *display = string_builder_create();
    update_window_size();
    row_ct = get_row_ct(), col_ct = get_col_ct();

    string_builder_append(display, CLEAR_SCREEN RESET_CURSOR SHOW_CURSOR);
    build_footer(display, mode);
    build_status_message(display,
                         string_builder_to_string(self->status_message),
                         self->status_message_is_error);

    write(STDOUT_FILENO, string_builder_to_string(display),
          string_builder_len(display));
    string_builder_destroy(display);
    buffer_display(self->buffer, top_offset, left_offset, row_ct - 2, col_ct);
}

static void run_command(Editor *self, const char *cmd) {
    Buffer *buffer = self->buffer;
    bool should_delete = false;
    if (strcmp(cmd, "w") == 0) {
        if (buffer_save(buffer)) {
            string_builder_set(self->status_message, buffer_name(self->buffer));
            string_builder_append(self->status_message, " written");
        } else {
            self->status_message_is_error = true;
            string_builder_set(self->status_message,
                               "failed to write to buffer");
        }
    } else if (strcmp(cmd, "x") == 0 || strcmp(cmd, "wq") == 0) {
        should_delete = true;
        if (buffer_save(buffer)) {
            string_builder_set(self->status_message, buffer_name(self->buffer));
            string_builder_append(self->status_message, " written");
        } else {
            self->status_message_is_error = true;
            string_builder_set(self->status_message,
                               "failed to write to buffer");
        }
    } else if (strcmp(cmd, "q!") == 0) {
        should_delete = true;
    } else if (strcmp(cmd, "q") == 0) {
        if (!buffer_is_modified(buffer)) {
            should_delete = true;
        } else {
            self->status_message_is_error = true;
            string_builder_set(self->status_message,
                               "cannot close modified buffer");
        }
    } else if (strcmp(cmd, "n") == 0) {
        self->buffer_idx++;
        self->buffer_idx %= list_len(self->buffers);
        self->buffer = list_get(self->buffers, self->buffer_idx);
    } else {
        self->status_message_is_error = true;
        string_builder_set(self->status_message, "unrecognized command: ");
        string_builder_append(self->status_message, cmd);
    }
    if (should_delete) {
        buffer_destroy(self->buffer);
        list_remove(self->buffers, self->buffer_idx);
        if (self->buffer_idx > 0) {
            self->buffer_idx--;
        }
        if (!list_is_empty(self->buffers)) {
            self->buffer = list_get(self->buffers, self->buffer_idx);
        }
    }
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

static void command_mode(Editor *self) {
    key_t key;
    StringBuilder *cmd = string_builder_create();
    bool keep_running = true;
    bool run_cmd = true;
    while (keep_running) {
        update_screen(self, COMMAND);
        display_command(string_builder_to_string(cmd));
        key = get_key();
        switch (key) {
        case ESC_KEY:
            run_cmd = false;
            keep_running = false;
            break;
        case '\n': keep_running = false; break;
        case BACKSPACE:
            if (string_builder_len(cmd) != 0) {
                string_builder_restrict(cmd, 0, -1);
            }
            break;
        default:
            if (key_is_printable(key)) {
                string_builder_append_char(cmd, key);
            }
            break;
        }
    }
    if (run_cmd) {
        run_command(self, string_builder_to_string(cmd));
    }
    string_builder_destroy(cmd);
}

void editor_add_buffer(Editor *self, Buffer *buffer) {
    list_insert(self->buffers, buffer, list_len(self->buffers));
}

void editor_run(Editor *self) {
    key_t key;
    bool keep_running = true;
    mode_t mode = NORMAL;
    if (list_len(self->buffers) == 0) {
        self->buffer = buffer_create("");
        if (!self->buffer) goto editor_run_fail;
        list_insert(self->buffers, self->buffer, 0);
    } else {
        self->buffer = list_get(self->buffers, 0);
    }
    enable_raw_mode();

    while (keep_running) {
        update_screen(self, mode);
        key = get_key();
        reset_status_message(self);
        if (key == ':' && mode != INSERT) {
            command_mode(self);
            keep_running = !list_is_empty(self->buffers);
        } else {
            if (key == 'i' && mode != INSERT) {
                mode = INSERT;
            } else if (key == ESC_KEY && mode != NORMAL) {
                mode = NORMAL;
            } else if (key == 'v' && mode == NORMAL) {
                mode = SELECT;
            }
            buffer_cmd(self->buffer, key, false);
        }
    }
editor_run_fail:
    printf(CLEAR_SCREEN RESET_CURSOR SHOW_CURSOR);
}

Editor *editor_create(void) {
    Editor *self = calloc(1, sizeof(Editor));
    if (!self) {
        report_system_error(FILENAME ": memory allocation failure");
        goto editor_create_fail;
    }
    self->buffers = list_create((void (*)(void *))buffer_destroy);
    if (!self->buffers) goto editor_create_fail;
    self->status_message = string_builder_create();
    if (!self->status_message) goto editor_create_fail;

    reset_status_message(self);
    self->buffer_idx = 0;

    return self;
editor_create_fail:
    editor_destroy(self);
    return NULL;
}

void editor_destroy(Editor *self) {
    if (self) {
        list_destroy(self->buffers);
        string_builder_destroy(self->status_message);
        free(self);
    }
}
