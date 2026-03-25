#include "buffer.h"
#include "action.h"
#include "colors.h"
#include "direction.h"
#include "keystroke.h"
#include "reporter.h"
#include "stack.h"
#include "string_builder.h"
#include "terminal.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define FILENAME "buffer.c"

typedef struct {
    bool is_some;
    direction_t unwrap;
} option_direction_t;

static option_direction_t read_direction(key_t key) {
    option_direction_t direction;
    direction.is_some = true;
    switch (key) {
    case 'h':
    case ARROW_LEFT: direction.unwrap = LEFT; break;
    case 'j':
    case ARROW_DOWN: direction.unwrap = DOWN; break;
    case 'k':
    case ARROW_UP: direction.unwrap = UP; break;
    case 'l':
    case ARROW_RIGHT: direction.unwrap = RIGHT; break;
    default: direction.is_some = false; break;
    }
    return direction;
}

struct Buffer {
    bool new_file;
    char *filename;
    StringBuilder *contents;
    /* direction cursor should move in insert mode */
    direction_t momentum;
    size_t cursor_row;
    size_t cursor_col;
    /* # of rows on the top of the screen hidden */
    uint16_t top_offset;
    /* # of rows on the left of the screen hidden */
    uint16_t left_offset;
    bool is_modified;

    Stack *redo_stack; /* Stack<Action> */
    Stack *undo_stack; /* Stack<Action> */
    /* index within stacks to redo after an undo */
    size_t stack_idx;

    Keystroke *current_redo_keystroke;
    Keystroke *current_undo_keystroke;
    uint16_t current_action_row;
    uint16_t current_action_col;
    uint16_t current_action_momentum;

    mode_t mode;

    size_t selection_start_row;
    size_t selection_start_col;
};

static void begin_recording_action(Buffer *self) {
    self->current_redo_keystroke = keystroke_create();
    self->current_undo_keystroke = keystroke_create();
    self->current_action_row = self->cursor_row;
    self->current_action_col = self->cursor_col;
    self->current_action_momentum = self->momentum;
}

static void push_current_action(Buffer *self) {
    const Action *redo_action = action_create(
        self->current_redo_keystroke, self->current_action_row,
        self->current_action_col, self->current_action_momentum);

    const Action *undo_action
        = action_create(self->current_undo_keystroke, self->cursor_row,
                        self->cursor_col, reverse_direction(self->momentum));

    while (self->stack_idx > 0) {
        action_destroy(stack_pop(self->redo_stack));
        action_destroy(stack_pop(self->undo_stack));
        self->stack_idx--;
    }

    /* don't leave mutable references hanging around */
    self->current_redo_keystroke = NULL;
    self->current_undo_keystroke = NULL;

    stack_push(self->redo_stack, (void *)redo_action);
    stack_push(self->undo_stack, (void *)undo_action);
}

static void follow_direction(Buffer *self, direction_t direction) {
    switch (direction) {
    case LEFT:
        if (self->cursor_col > 0) {
            self->cursor_col--;
        }
        break;
    case DOWN: self->cursor_row++; break;
    case UP:
        if (self->cursor_row > 0) {
            self->cursor_row--;
        }
        break;
    case RIGHT: self->cursor_col++; break;
    }
}

static void follow_momentum(Buffer *self) {
    follow_direction(self, self->momentum);
}

static void follow_reverse_momentum(Buffer *self) {
    follow_direction(self, reverse_direction(self->momentum));
}

static void execute_direction(Buffer *self, direction_t direction) {
    if (direction != self->momentum) {
        self->momentum = direction;
    } else {
        follow_momentum(self);
    }
}

static void prepend_undo_direction(Buffer *self, direction_t direction) {
    direction_t from_direction = reverse_direction(direction);
    direction_t to_direction = reverse_direction(self->momentum);

    switch (angle_degrees_between(from_direction, to_direction)) {
    case 0:
        keystroke_prepend_key(self->current_undo_keystroke,
                              direction_as_key(to_direction));
        break;
    case 90:
        keystroke_prepend_key(self->current_undo_keystroke,
                              direction_as_key(to_direction));
        keystroke_prepend_key(self->current_undo_keystroke,
                              direction_as_key(to_direction));
        keystroke_prepend_key(
            self->current_undo_keystroke,
            direction_as_key(reverse_direction(from_direction)));
        keystroke_prepend_key(
            self->current_undo_keystroke,
            direction_as_key(reverse_direction(from_direction)));
        break;
    case 180:
        keystroke_prepend_key(self->current_undo_keystroke,
                              direction_as_key(to_direction));
        keystroke_prepend_key(
            self->current_undo_keystroke,
            direction_as_key(reverse_direction(from_direction)));
        keystroke_prepend_key(
            self->current_undo_keystroke,
            direction_as_key(reverse_direction(from_direction)));
        break;
    default:
        report_logic_error(FILENAME
                           ": cannot handle direction angle difference");
    }
}

static void buffer_insert_cmd(Buffer *self, key_t cmd, bool is_simulated) {
    uint16_t row = 0, col = 0;
    size_t contents_idx = 0;
    size_t contents_len = string_builder_len(self->contents);
    option_direction_t direction;
    if (!is_simulated) {
        keystroke_append_key(self->current_redo_keystroke, cmd);
    }
    if (cmd == BACKSPACE) {
        follow_reverse_momentum(self);
    } else if (!key_is_printable(cmd)) {
        if (cmd == ESC_KEY) {
            if (!is_simulated) {
                keystroke_prepend_key(self->current_undo_keystroke, 'i');
                keystroke_prepend_key(
                    self->current_undo_keystroke,
                    direction_as_key(reverse_direction(self->momentum)));
                push_current_action(self);
            }
            self->mode = NORMAL;
        } else {
            direction = read_direction(cmd);
            if (direction.is_some) {
                if (!is_simulated) {
                    prepend_undo_direction(self, direction.unwrap);
                }
                execute_direction(self, direction.unwrap);
            }
        }
        /* TODO - allow arrow keys to work */
        return;
    }
    self->is_modified = true;
    while (row < self->cursor_row) {
        if (contents_idx == contents_len) {
            string_builder_append_char(self->contents, '\n');
            contents_len++;
            row++;
        } else if (string_builder_get_char(self->contents, contents_idx)
                   == '\n') {
            row++;
        }
        contents_idx++;
    }
    while (col < self->cursor_col) {
        if (contents_idx == contents_len) {
            string_builder_append_char(self->contents, ' ');
            contents_len++;
        } else if (string_builder_get_char(self->contents, contents_idx)
                   == '\n') {
            string_builder_insert(self->contents, contents_idx, " ");
            contents_len++;
        }
        col++;
        contents_idx++;
    }

    if (contents_idx == contents_len
        || string_builder_get_char(self->contents, contents_idx) == '\n') {
        string_builder_insert(self->contents, contents_idx, " ");
        contents_len++;
    }
    if (cmd == BACKSPACE) {
        /* TODO - handle current undo keystroke */
        string_builder_set_char(self->contents, contents_idx, ' ');
    } else {
        if (!is_simulated) {
            keystroke_prepend_key(
                self->current_undo_keystroke,
                string_builder_get_char(self->contents, contents_idx));
        }
        string_builder_set_char(self->contents, contents_idx, cmd);
        follow_momentum(self);
    }
}

static void execute_keystroke(Buffer *self, Keystroke *keystroke,
                              bool is_simulated) {
    size_t i;
    for (i = 0; i < keystroke_len(keystroke); i++) {
        buffer_cmd(self, keystroke_get_key(keystroke, i), is_simulated);
    }
}

static void redo_last_action(Buffer *self) {
    Action *action = stack_peek(self->redo_stack);
    Keystroke *keystroke = action_get_keystroke(action);
    execute_keystroke(self, keystroke, false);
}

static void simulate_action(Buffer *self, Action *action) {
    Keystroke *keystroke = action_get_keystroke(action);
    self->cursor_col = action_get_col(action);
    self->cursor_row = action_get_row(action);
    self->momentum = action_get_momentum(action);
    execute_keystroke(self, keystroke, true);
}

static void undo(Buffer *self) {
    Action *undo_action, *redo_action;
    if (stack_len(self->undo_stack) > self->stack_idx) {
        undo_action = stack_get(self->undo_stack, self->stack_idx);
        redo_action = stack_get(self->redo_stack, self->stack_idx);
        simulate_action(self, undo_action);
        self->cursor_col = action_get_col(redo_action);
        self->cursor_row = action_get_row(redo_action);
        self->stack_idx++;
    }
}

static void redo(Buffer *self) {
    if (self->stack_idx > 0) {
        simulate_action(self, stack_get(self->redo_stack, self->stack_idx - 1));
        self->stack_idx--;
    }
}

static StringBuilder *snatch_horizontal_line(Buffer *self) {
    size_t contents_idx = 0;
    size_t row = 0;
    const size_t contents_len = string_builder_len(self->contents);
    StringBuilder *line = string_builder_create();
    while (contents_idx < contents_len && row < self->cursor_row) {
        if (string_builder_get_char(self->contents, contents_idx) == '\n') {
            row++;
        }
        contents_idx++;
    }
    if (row == self->cursor_row) {
        while (contents_idx < contents_len) {
            if (string_builder_get_char(self->contents, contents_idx) == '\n') {
                break;
            }
            string_builder_append_char(
                line, string_builder_get_char(self->contents, contents_idx));
            contents_idx++;
        }
    }
    return line;
}

static StringBuilder *snatch_vertical_line(Buffer *self) {
    size_t contents_idx;
    size_t col = 0;
    const size_t contents_len = string_builder_len(self->contents);
    StringBuilder *line = string_builder_create();
    for (contents_idx = 0; contents_idx < contents_len; contents_idx++) {
        if (string_builder_get_char(self->contents, contents_idx) == '\n') {
            if (col <= self->cursor_col) {
                string_builder_append_char(line, ' ');
            }
            col = 0;
            continue;
        }
        if (col == self->cursor_col) {
            string_builder_append_char(
                line, string_builder_get_char(self->contents, contents_idx));
        }
        col++;
    }
    return line;
}

/**
 * Get string representation of current line following momentum.
 */
static StringBuilder *snatch_line(Buffer *self) {
    if (angle_degrees_between(self->momentum, DOWN) == 90) {
        return snatch_horizontal_line(self);
    } else {
        return snatch_vertical_line(self);
    }
}

/**
 * Jump to one or the other end of the line.
 * Jump to start of the line if `start` is true, else the other end.
 */
static void jump_line_end(Buffer *self, bool start) {
    StringBuilder *line = snatch_line(self);
    size_t line_end;
    size_t i;
    const size_t line_len = string_builder_len(line);
    char c;
    bool is_line_start
        = start == (self->momentum == RIGHT || self->momentum == DOWN);

    line_end = 0;

    for (i = is_line_start ? 0 : line_len - 1;
         (is_line_start && i < line_len) || (!is_line_start && i > 0);
         i += is_line_start ? 1 : -1) {
        c = string_builder_get_char(line, i);
        if (c != ' ' && c != '\t') {
            line_end = i;
            break;
        }
    }

    if (angle_degrees_between(self->momentum, DOWN) == 90) {
        self->cursor_col = line_end;
    } else {
        self->cursor_row = line_end;
    }
    string_builder_destroy(line);
}

static void buffer_normal_cmd(Buffer *self, key_t cmd, bool is_simulated) {
    option_direction_t direction = read_direction(cmd);

    if (direction.is_some) {
        execute_direction(self, direction.unwrap);
    } else {
        switch (cmd) {
        case 'v':
            self->selection_start_col = self->cursor_col;
            self->selection_start_row = self->cursor_row;
            self->mode = SELECT;
            break;
        case 'i':
        case 'a':
        case 'A':
        case 'I':
            if (!is_simulated) {
                begin_recording_action(self);
                if (cmd == 'a') {
                    follow_momentum(self);
                } else if (cmd == 'A') {
                    jump_line_end(self, false);
                } else if (cmd == 'I') {
                    jump_line_end(self, true);
                }
                keystroke_append_key(self->current_redo_keystroke, cmd);
                keystroke_prepend_key(self->current_undo_keystroke, ESC_KEY);
            }
            self->mode = INSERT;
            break;
        case '$': jump_line_end(self, false); break;
        case '^': jump_line_end(self, true); break;
        case '.': redo_last_action(self); break;
        case 'u': undo(self); break;
        case 'U': redo(self); break;
        default: break;
        }
    }
}

static void buffer_select_cmd(Buffer *self, key_t cmd) {
    option_direction_t direction = read_direction(cmd);
    if (direction.is_some) {
        self->momentum = direction.unwrap;
        follow_momentum(self);
    } else if (cmd == ESC_KEY) {
        self->mode = NORMAL;
    }
}

void buffer_cmd(Buffer *self, key_t cmd, bool is_simulated) {
    switch (self->mode) {
    case INSERT: buffer_insert_cmd(self, cmd, is_simulated); break;
    case NORMAL: buffer_normal_cmd(self, cmd, is_simulated); break;
    case SELECT: buffer_select_cmd(self, cmd); break;
    default: report_logic_error(FILENAME ": weird mode");
    }
}

/**
 * Move frame as necessary to fit cursor.
 */
static void fit_frame_to_cursor(Buffer *self, uint16_t row_ct,
                                uint16_t col_ct) {
    const uint16_t min_row = self->top_offset;
    const uint16_t max_row = min_row + row_ct - 1;
    const uint16_t min_col = self->left_offset;
    const uint16_t max_col = min_col + col_ct - 1;

    if (self->cursor_row < min_row) {
        self->top_offset = self->cursor_row;
    } else if (self->cursor_row > max_row) {
        self->top_offset = self->cursor_row - row_ct + 1;
    }

    if (self->cursor_col < min_col) {
        self->left_offset = self->cursor_col;
    } else if (self->cursor_col > max_col) {
        self->left_offset = self->cursor_col - col_ct + 1;
    }
}

static bool is_selected(Buffer *self, size_t row, size_t col) {
    bool is_row_selected, is_col_selected;

    if (self->mode != SELECT) {
        return false;
    }
    is_row_selected
        = (self->selection_start_row <= row && row <= self->cursor_row)
          || (row <= self->selection_start_row && self->cursor_row <= row);
    is_col_selected
        = (self->selection_start_col <= col && col <= self->cursor_col)
          || (col <= self->selection_start_col && self->cursor_col <= col);
    return is_row_selected && is_col_selected;
}

void buffer_display(Buffer *self, uint16_t top_offset, uint16_t left_offset,
                    uint16_t row_ct, uint16_t col_ct) {
    uint16_t row = 0, col = 0;
    size_t contents_idx = 0;
    const size_t contents_len = string_builder_len(self->contents);
    StringBuilder *display = string_builder_create();
    char contents_char;
    fit_frame_to_cursor(self, row_ct, col_ct);
    move_cursor(display, top_offset + 1, left_offset + 1);
    while (row < row_ct + self->top_offset) {
        if (contents_idx < contents_len) {
            contents_char
                = string_builder_get_char(self->contents, contents_idx);
        } else {
            contents_char = '\n';
        }
        if (contents_char == '\n') {
            if (col == col_ct + self->left_offset) {
                row++;
                col = 0;
                move_cursor(display, row + top_offset - self->top_offset + 1,
                            left_offset + 1);
                contents_idx++;
                continue;
            }
        } else {
            contents_idx++;
        }
        if (contents_char == '\t' || contents_char == '\n') {
            /* TODO - how should tabs be displayed? is this best way? */
            contents_char = ' ';
        } else if (!isprint(contents_char)) {
            /* TODO - what about non-printable characters? */
            contents_char = '?';
        }
        if (self->left_offset <= col && col <= self->left_offset + col_ct
            && self->top_offset <= row) {
            if (is_selected(self, row, col)) {
                string_builder_append(display, HIGHLIGHT);
                string_builder_append_char(display, contents_char);
                string_builder_append(display, RESET);
            } else {
                string_builder_append_char(display, contents_char);
            }
        }
        col++;
    }
    move_cursor(display, top_offset + 1 + self->cursor_row - self->top_offset,
                left_offset + 1 + self->cursor_col - self->left_offset);
    write(STDOUT_FILENO, string_builder_to_string(display),
          string_builder_len(display));
    string_builder_destroy(display);
}

/**
 * Get name of buffer.
 */
char *buffer_name(Buffer *self) {
    return self->filename;
}

bool buffer_save(Buffer *self) {
    FILE *file = fopen(self->filename, "w");
    if (!file) {
        return false;
    }
    fwrite(string_builder_to_string(self->contents), sizeof(char),
           string_builder_len(self->contents), file);
    fclose(file);
    self->is_modified = false;
    return true;
}

bool buffer_is_modified(Buffer *self) {
    return self->is_modified;
}

#define CHUNK_SIZE 128

/**
 * Initialize buffer. Return `true` if new file.
 */
static bool init_buffer(Buffer *self) {
    FILE *file = fopen(self->filename, "r");
    char chunk[CHUNK_SIZE + 1];
    size_t n;

    if (file) {
        do {
            n = fread(chunk, 1, CHUNK_SIZE, file);
            chunk[n] = '\0';
            string_builder_append(self->contents, chunk);
        } while (n == CHUNK_SIZE);
        fclose(file);
    }
    return !!file;
}

Buffer *buffer_create(const char *filename) {
    Buffer *self = calloc(1, sizeof(Buffer));
    if (!self) {
        report_system_error(FILENAME ": memory allocation failure");
        goto buffer_create_fail;
    }

    self->filename = malloc(strlen(filename) + 1);
    if (!self->filename) {
        report_system_error(FILENAME ": memory allocation failure");
        goto buffer_create_fail;
    }
    strcpy(self->filename, filename);

    self->contents = string_builder_create();
    if (!self->contents) goto buffer_create_fail;

    self->new_file = init_buffer(self);

    self->cursor_row = 0;
    self->cursor_col = 0;
    self->top_offset = 0;
    self->left_offset = 0;
    self->momentum = RIGHT;
    self->is_modified = false;
    self->mode = NORMAL;

    self->redo_stack = stack_create((void (*)(void *))action_destroy);
    if (!self->redo_stack) goto buffer_create_fail;
    self->undo_stack = stack_create((void (*)(void *))action_destroy);
    if (!self->undo_stack) goto buffer_create_fail;
    self->stack_idx = 0;

    return self;
buffer_create_fail:
    buffer_destroy(self);
    return NULL;
}

void buffer_destroy(Buffer *self) {
    if (self) {
        free(self->filename);
        string_builder_destroy(self->contents);

        stack_destroy(self->redo_stack);
        stack_destroy(self->undo_stack);

        keystroke_destroy(self->current_redo_keystroke);
        keystroke_destroy(self->current_undo_keystroke);

        free(self);
    }
}
