#define _POSIX_C_SOURCE 200809L

#include "editor.h"
#include "buffer.h"
#include "colors.h"
#include "command.h"
#include "direction.h"
#include "interpreter.h"
#include "list.h"
#include "pthread.h"
#include "reporter.h"
#include "string_builder.h"
#include "terminal.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
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
    char *config_path;
    mode_t saved_mode; /* for command mode */
    mode_t mode;

    /* macro for ctrl keys A-Z */
    Keystroke *macros[26];

    StringBuilder *cmd;
};

void editor_registor_macro(Editor *self, Keystroke *macro, key_t key) {
    const size_t i = key - 'A';
    if (i >= 26) {
        report_logic_error(FILENAME ": invalid macro");
        exit(1);
    }
    if (self->macros[i]) {
        keystroke_destroy(self->macros[i]);
    }
    self->macros[i] = macro;
}

static void reset_status_message(Editor *self) {
    string_builder_set(self->status_message, "");
    self->status_message_is_error = false;
}

static void build_command_footer(StringBuilder *display, Editor *self) {
    uint16_t col;

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
        if (string_builder_len(self->cmd) + 1 == col) break;
        string_builder_append_char(display,
                                   string_builder_get_char(self->cmd, col - 1));
    }

    string_builder_append(display, RESET);
}

static void build_footer(StringBuilder *display, mode_t mode,
                         direction_t momentum, size_t cursor_row,
                         size_t cursor_col) {
    uint16_t col;
    const char *mode_str = mode == NORMAL   ? "NORMAL"
                           : mode == INSERT ? "INSERT"
                                            : "SELECT";
    const char *arrow = momentum == LEFT    ? "←"
                        : momentum == UP    ? "↑"
                        : momentum == RIGHT ? "→"
                                            : "↓";
    char position[32];

    /* fill empty footer */
    move_cursor(display, get_row_ct() - 1, 0);
    string_builder_append(display, HIGHLIGHT);
    for (col = 0; col < get_col_ct(); col++) {
        string_builder_append_char(display, ' ');
    }

    /* write mode */
    move_cursor(display, get_row_ct() - 1, 0);
    string_builder_append(display, mode_str);

    sprintf(position, "%lu:%lu", cursor_row, cursor_col);

    /* write position */
    move_cursor(display, get_row_ct() - 1, get_col_ct() - strlen(position) - 2);
    string_builder_append(display, position);

    /* write momentum */
    move_cursor(display, get_row_ct() - 1, get_col_ct() - 1);
    string_builder_append(display, arrow);

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

static void update_screen(Editor *self) {
    const uint16_t top_offset = 0, left_offset = 0;
    uint16_t row_ct, col_ct;
    StringBuilder *display = string_builder_create();
    update_window_size();
    row_ct = get_row_ct(), col_ct = get_col_ct();

    string_builder_append(display, CLEAR_SCREEN RESET_CURSOR SHOW_CURSOR);

    build_status_message(display,
                         string_builder_to_string(self->status_message),
                         self->status_message_is_error);

    /* display order changes due to differing cursor positions */
    if (self->mode == COMMAND) {
        buffer_build_display(self->buffer, display, top_offset, left_offset,
                             row_ct - 2, col_ct);
        build_command_footer(display, self);
    } else {
        build_footer(display, self->mode, buffer_get_momentum(self->buffer),
                     buffer_get_row(self->buffer),
                     buffer_get_col(self->buffer));
        buffer_build_display(self->buffer, display, top_offset, left_offset,
                             row_ct - 2, col_ct);
    }

    string_builder_print(display);
    string_builder_destroy(display);
}

static void editor_force_quit(Editor *self) {
    buffer_destroy(self->buffer);
    list_remove(self->buffers, self->buffer_idx);
    if (self->buffer_idx > 0) {
        self->buffer_idx--;
    }
    if (!list_is_empty(self->buffers)) {
        self->buffer = list_get(self->buffers, self->buffer_idx);
    }
}

static void editor_force_quit_all(Editor *self) {
    while (!list_is_empty(self->buffers)) {
        buffer_destroy(list_remove(self->buffers, 0));
    }
    self->buffer_idx = 0;
}

static void editor_quit(Editor *self) {
    if (!buffer_is_modified(self->buffer)) {
        editor_force_quit(self);
    } else {
        self->status_message_is_error = true;
        string_builder_set(self->status_message,
                           "cannot close modified buffer");
    }
}

static void editor_quit_all(Editor *self) {
    bool should_delete_all = true;
    size_t i;
    for (i = 0; i < list_len(self->buffers) && should_delete_all; i++) {
        if (buffer_is_modified(list_get(self->buffers, i))) {
            should_delete_all = false;
        }
    }
    if (!should_delete_all) {
        self->status_message_is_error = true;
        string_builder_set(self->status_message,
                           "cannot close modified buffer");
    }
}

/**
 * Return true iff successful.
 */
static bool editor_write(Editor *self) {
    if (buffer_save(self->buffer)) {
        string_builder_set(self->status_message, buffer_name(self->buffer));
        string_builder_append(self->status_message, " written");
        return true;
    } else {
        self->status_message_is_error = true;
        string_builder_set(self->status_message, "failed to write buffer");
        return false;
    }
}

static void editor_write_quit(Editor *self) {
    if (editor_write(self)) {
        editor_force_quit(self);
    }
}

/**
 * Return true iff successful.
 */
static bool editor_write_all(Editor *self) {
    size_t i;
    bool all_successful = true;
    for (i = 0; i < list_len(self->buffers); i++) {
        if (buffer_is_modified(list_get(self->buffers, i))) {
            if (!buffer_save(list_get(self->buffers, i))) {
                all_successful = false;
            }
        }
    }
    if (all_successful) {
        string_builder_set(self->status_message, "wrote all buffers");
    } else {
        self->status_message_is_error = true;
        string_builder_set(self->status_message, "failed to write all buffers");
    }
    return all_successful;
}

static void editor_write_quit_all(Editor *self) {
    if (editor_write_all(self)) {
        editor_force_quit_all(self);
    }
}

static void editor_next(Editor *self) {
    self->buffer_idx++;
    self->buffer_idx %= list_len(self->buffers);
    self->buffer = list_get(self->buffers, self->buffer_idx);
}

static void editor_open(Editor *self, const char *name) {
    Buffer *buffer = buffer_create(name);
    self->buffer = buffer;
    editor_add_buffer(self, buffer);
    self->buffer_idx = list_len(self->buffers) - 1;
}

/**
 * Return `true` iff args look good.
 */
static bool check_args(Editor *self, Command *command) {
    size_t expected_arg_ct = 1;
    command_t cmd = command_get_command(command);
    switch (cmd) {
    case WRITE:
    case WRITE_QUIT:
    case FORCE_QUIT:
    case QUIT:
    case WRITE_ALL:
    case WRITE_QUIT_ALL:
    case QUIT_ALL:
    case FORCE_QUIT_ALL:
    case NEXT:
    case CLEAN_WHITESPACE: expected_arg_ct = 1; break;
    case OPEN: expected_arg_ct = 2; break;
    case CONFIG_OPEN:
    case CONFIG_RELOAD: expected_arg_ct = 1; break;
    case EMPTY: expected_arg_ct = 0; break;
    case UNKNOWN: break;
    }
    if (cmd != UNKNOWN && command_arg_ct(command) != expected_arg_ct) {
        self->status_message_is_error = true;
        string_builder_set(self->status_message, "incorrect # of args");
        return false;
    }
    return true;
}

static bool check_config_exists(Editor *self);
static void editor_load_config(Editor *self);

static void editor_config_open(Editor *self) {
    if (check_config_exists(self)) {
        editor_open(self, self->config_path);
    }
}

static void run_command(Editor *self) {
    const char *cmd = string_builder_to_string(self->cmd);
    Command *command = command_create(cmd);
    if (!command) return;
    if (check_args(self, command)) {
        switch (command_get_command(command)) {
        case WRITE: editor_write(self); break;
        case WRITE_QUIT: editor_write_quit(self); break;
        case FORCE_QUIT: editor_force_quit(self); break;
        case QUIT: editor_quit(self); break;
        case WRITE_ALL: editor_write_all(self); break;
        case WRITE_QUIT_ALL: editor_write_quit_all(self); break;
        case QUIT_ALL: editor_quit_all(self); break;
        case FORCE_QUIT_ALL: editor_force_quit_all(self); break;
        case NEXT: editor_next(self); break;
        case CLEAN_WHITESPACE: buffer_clean_whitespace(self->buffer); break;
        case OPEN: editor_open(self, command_get_arg(command, 1)); break;
        case CONFIG_OPEN: editor_config_open(self); break;
        case CONFIG_RELOAD: editor_load_config(self); break;
        case EMPTY:
            /* do nothing */
            break;
        case UNKNOWN:
            self->status_message_is_error = true;
            string_builder_set(self->status_message, "unrecognized command: ");
            string_builder_append(self->status_message,
                                  command_get_arg(command, 0));
        }
    }
    command_destroy(command);
}

void editor_add_buffer(Editor *self, Buffer *buffer) {
    list_insert(self->buffers, buffer, list_len(self->buffers));
}

#define CONFIG_PATH_FROM_HOME "/.config/befedit/config.b98"

static void set_config_path(Editor *self) {
    StringBuilder *config_path;
    const char *home_path = getenv("HOME");

    if (!home_path) {
        self->config_path = NULL;
        return;
    }

    config_path = string_builder_create();
    string_builder_set(config_path, home_path);
    string_builder_append(config_path, CONFIG_PATH_FROM_HOME);
    self->config_path = malloc(string_builder_len(config_path) + 1);
    strcpy(self->config_path, string_builder_to_string(config_path));
    string_builder_destroy(config_path);
}

/**
 * Return `true` iff exists and is readable.
 */
static bool check_config_exists(Editor *self) {
    FILE *config;

    if (!self->config_path) {
        self->status_message_is_error = true;
        string_builder_set(self->status_message,
                           "failed to find config, is $HOME set?");
        return false;
    }

    config = fopen(self->config_path, "r");
    if (config) {
        fclose(config);
        return true;
    } else {
        self->status_message_is_error = true;
        string_builder_set(self->status_message, "failed to find config at: ");
        string_builder_append(self->status_message, self->config_path);
        return false;
    }
}

typedef struct {
    Interpreter *interpreter;
    pthread_mutex_t *mutex;
    pthread_cond_t *cond;
    bool *finished;
} interpreter_thread_args_t;

static void *interpreter_run_wrapper(void *arg) {
    interpreter_thread_args_t *args = (interpreter_thread_args_t *)arg;
    interpreter_run(args->interpreter);

    pthread_mutex_lock(args->mutex);
    *args->finished = true;
    pthread_cond_signal(args->cond);
    pthread_mutex_unlock(args->mutex);

    return NULL;
}

#define CONFIG_TIMEOUT_SEC 2

static void editor_load_config(Editor *self) {
    interpreter_thread_args_t args;
    Interpreter *interpreter;
    pthread_t thread;
    struct timespec timeout;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
    bool finished = false;
    int ret;

    if (check_config_exists(self)) {
        interpreter = interpreter_create(self->config_path, self);
        if (interpreter == NULL) exit(1);

        args.interpreter = interpreter;
        args.mutex = &mutex;
        args.cond = &cond;
        args.finished = &finished;

        if (pthread_create(&thread, NULL, interpreter_run_wrapper, &args)) {
            report_system_error(FILENAME ": failed to spawn thread");
            exit(1);
        }
        if (clock_gettime(CLOCK_REALTIME, &timeout)) {
            report_system_error(FILENAME ": failed to get time");
            exit(1);
        }
        timeout.tv_sec += CONFIG_TIMEOUT_SEC;

        pthread_mutex_lock(&mutex);
        while (!finished) {
            ret = pthread_cond_timedwait(&cond, &mutex, &timeout);
            if (ret != 0) {
                /* stop the interpreter from running */
                /* TODO - is this guaranteed to stop it in time? */
                *(interpreter_is_poisoned_ref(interpreter)) = true;

                if (ret == ETIMEDOUT) {
                    break;
                } else {
                    report_system_error(FILENAME ": failed to wait");
                    exit(1);
                }
            }
        }
        pthread_mutex_unlock(&mutex);

        if (finished) {
            pthread_join(thread, NULL);
            if (*(interpreter_is_poisoned_ref(interpreter))) {
                self->status_message_is_error = true;
                string_builder_set(self->status_message, "config error: ");
            } else {
                string_builder_set(self->status_message, "");
            }
            string_builder_append(self->status_message,
                                  interpreter_get_output(interpreter));
        } else {
            pthread_detach(thread);
            /* timed out */
            self->status_message_is_error = true;
            string_builder_set(self->status_message, "config file timed out");
        }

        interpreter_destroy(interpreter);
    }
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
}

/**
 * Return `true` iff should keep running
 */
static bool editor_execute_key(Editor *self, key_t key) {
    reset_status_message(self);
    if (self->mode == COMMAND) {
        switch (key) {
        case ESC_KEY: self->mode = self->saved_mode; break;
        case '\n':
            self->mode = self->saved_mode;
            run_command(self);
            return !list_is_empty(self->buffers);
        case BACKSPACE:
            if (string_builder_len(self->cmd) != 0) {
                string_builder_restrict(self->cmd, 0, -1);
            }
            break;
        default:
            if (key_is_printable(key)) {
                string_builder_append_char(self->cmd, key);
            }
            break;
        }
        return true;
    } else if (key == ':' && self->mode != INSERT) {
        string_builder_set(self->cmd, "");
        self->saved_mode = self->mode;
        self->mode = COMMAND;
        return !list_is_empty(self->buffers);
    } else {

        if ((key == 'i' || key == 'I' || key == 'a' || key == 'A')
            && self->mode != INSERT) {
            self->mode = INSERT;
        } else if (key == ESC_KEY && self->mode != NORMAL) {
            self->mode = NORMAL;
        } else if (key == 'v' && self->mode == NORMAL) {
            self->mode = SELECT;
        }
        buffer_cmd(self->buffer, key, false);
        return true;
    }
}

static bool editor_execute_keystroke(Editor *self, Keystroke *keystroke) {
    size_t i;
    bool keep_running = true;
    for (i = 0; i < keystroke_len(keystroke) && keep_running; i++) {
        keep_running
            = editor_execute_key(self, keystroke_get_key(keystroke, i));
    }
    return keep_running;
}

void editor_run(Editor *self) {
    key_t key;
    bool keep_running = true;
    Keystroke *macro;
    if (list_len(self->buffers) == 0) {
        self->buffer = buffer_create("");
        if (!self->buffer) goto editor_run_fail;
        list_insert(self->buffers, self->buffer, 0);
    } else {
        self->buffer = list_get(self->buffers, 0);
    }
    enable_raw_mode();

    string_builder_set(self->status_message, "loading config...");
    update_screen(self);

    editor_load_config(self);

    while (keep_running) {
        update_screen(self);
        key = get_key();
        /* CTRL('J') and CTRL('M') are read as newline */
        /* TODO - is there a way to differentiate CTRL('J'), CTRL('M') and
         * newline? */
        if (CTRL('A') <= key && key <= CTRL('Z') && key != '\n') {
            macro = self->macros[key - CTRL('A')];
            if (macro) {
                keep_running = editor_execute_keystroke(self, macro);
            }
        } else {
            keep_running = editor_execute_key(self, key);
        }
    }
editor_run_fail:
    printf(CLEAR_SCREEN RESET_CURSOR SHOW_CURSOR);
}

Editor *editor_create(void) {
    size_t i;
    Editor *self = calloc(1, sizeof(Editor));
    if (!self) {
        report_system_error(FILENAME ": memory allocation failure");
        goto editor_create_fail;
    }

    self->buffers = list_create((void (*)(void *))buffer_destroy);
    if (!self->buffers) goto editor_create_fail;

    self->status_message = string_builder_create();
    if (!self->status_message) goto editor_create_fail;

    self->cmd = string_builder_create();
    if (!self->cmd) goto editor_create_fail;

    set_config_path(self);
    self->mode = NORMAL;

    for (i = 0; i < 26; i++) {
        self->macros[i] = NULL;
    }

    reset_status_message(self);
    self->buffer_idx = 0;

    return self;
editor_create_fail:
    editor_destroy(self);
    return NULL;
}

void editor_destroy(Editor *self) {
    size_t i;
    if (self) {
        list_destroy(self->buffers);
        string_builder_destroy(self->status_message);
        free(self->config_path);
        for (i = 0; i < 26; i++) {
            keystroke_destroy(self->macros[i]);
        }
        string_builder_destroy(self->cmd);
        free(self);
    }
}
