#include "buffer.h"
#include "action.h"
#include "buffer_space.h"
#include "colors.h"
#include "direction.h"
#include "keystroke.h"
#include "reporter.h"
#include "stack.h"
#include "string_builder.h"
#include "terminal.h"
#include "vector.h"
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
    char *filename;

    BufferSpace *contents;

    /* direction cursor should move in insert mode */
    direction_t momentum;
    vector_t cursor_pos;

    /* # of rows on the top of the screen hidden */
    uint16_t top_offset;
    size_t rows_past_end;

    /* # of columns on the left of the screen hidden */
    uint16_t left_offset;
    bool is_modified;

    Stack *redo_stack; /* Stack<Action> */
    Stack *undo_stack; /* Stack<Action> */

    /* index within stacks to redo after an undo */
    size_t stack_idx;

    bool is_recording;
    Keystroke *current_redo_keystroke;
    Keystroke *current_undo_keystroke;
    vector_t *current_action_pos;
    uint16_t current_action_momentum;

    /* position of start of insert mode, used for <enter> */
    vector_t insert_start_pos;

    mode_t mode;
    bool is_replacing; /* for insert mode but just one char */

    vector_t selection_start_pos;

    /* not owned by buffer */
    Keystroke **yanked;
};

static void begin_recording_action(Buffer *self) {
    self->is_recording = true;
    self->current_redo_keystroke = keystroke_create();
    self->current_undo_keystroke = keystroke_create();
    self->current_action_pos
        = buffer_space_get_coordinate(self->contents, self->cursor_pos);
    self->current_action_momentum = self->momentum;
}

static void push_current_action(Buffer *self) {
    vector_t *undo_action_pos
        = buffer_space_get_coordinate(self->contents, self->cursor_pos);
    const Action *redo_action
        = action_create(self->current_redo_keystroke, self->current_action_pos,
                        self->current_action_momentum);

    const Action *undo_action
        = action_create(self->current_undo_keystroke, undo_action_pos,
                        reverse_direction(self->momentum));

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

    self->is_recording = false;
}

static void follow_direction(Buffer *self, direction_t direction) {
    switch (direction) {
    case LEFT:
        if (self->cursor_pos.x > 0) {
            self->cursor_pos.x--;
        }
        break;
    case DOWN: self->cursor_pos.y++; break;
    case UP:
        if (self->cursor_pos.y > 0) {
            self->cursor_pos.y--;
        }
        break;
    case RIGHT: self->cursor_pos.x++; break;
    }
}

static void follow_momentum(Buffer *self) {
    follow_direction(self, self->momentum);
}

static void add_row_top(Buffer *self) {
    /* int32_min so it's not between actions */
    buffer_space_insert_row(self->contents, 0);
}

static void add_column_left(Buffer *self) {
    buffer_space_insert_col(self->contents, 0);
}

static void force_follow_direction(Buffer *self, direction_t direction) {
    switch (direction) {
    case LEFT:
        if (self->cursor_pos.x > 0) {
            self->cursor_pos.x--;
        } else {
            add_column_left(self);
        }
        break;
    case DOWN: self->cursor_pos.y++; break;
    case UP:
        if (self->cursor_pos.y > 0) {
            self->cursor_pos.y--;
        } else {
            add_row_top(self);
        }
        break;
    case RIGHT: self->cursor_pos.x++; break;
    }
}

static void force_follow_momentum(Buffer *self) {
    force_follow_direction(self, self->momentum);
}

static void force_follow_reverse_momentum(Buffer *self) {
    force_follow_direction(self, reverse_direction(self->momentum));
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

static void execute_direction(Buffer *self, direction_t direction) {
    if (self->is_recording) {
        prepend_undo_direction(self, direction);
    }
    if (direction != self->momentum) {
        self->momentum = direction;
    } else {
        force_follow_momentum(self);
    }
}

static void buffer_insert_enter(Buffer *self) {
    direction_t forwards = self->momentum;
    direction_t backwards = reverse_direction(self->momentum);
    direction_t next_line = rotate_90_degrees(self->momentum);
    bool keep_following = true;
    execute_direction(self, next_line);
    execute_direction(self, next_line);
    execute_direction(self, backwards);
    while (keep_following) {
        switch (forwards) {
        case UP:
            keep_following = (self->cursor_pos.y < self->insert_start_pos.y);
            break;
        case DOWN:
            keep_following = (self->cursor_pos.y > self->insert_start_pos.y);
            break;
        case LEFT:
            keep_following = (self->cursor_pos.x < self->insert_start_pos.x);
            break;
        case RIGHT:
            keep_following = (self->cursor_pos.x > self->insert_start_pos.x);
            break;
        default: report_logic_error(FILENAME ": unknown direction"); exit(1);
        }
        if (keep_following) {
            execute_direction(self, backwards);
        }
    }
    execute_direction(self, forwards);
}

static void buffer_insert_cmd(Buffer *self, key_t cmd) {
    option_direction_t direction;
    if (self->is_recording) {
        keystroke_append_key(self->current_redo_keystroke, cmd);
    }
    if (cmd == BACKSPACE) {
        force_follow_reverse_momentum(self);
    } else if (cmd == '\n') {
        buffer_insert_enter(self);
        return;
    } else if (!key_is_printable(cmd)) {
        if (cmd == ESC_KEY) {
            if (self->is_recording) {
                keystroke_prepend_key(self->current_undo_keystroke, 'i');
                if (!self->is_replacing) {
                    keystroke_prepend_key(
                        self->current_undo_keystroke,
                        direction_as_key(reverse_direction(self->momentum)));
                }
                push_current_action(self);
            }
            self->is_replacing = false;
            self->mode = NORMAL;
        } else {
            direction = read_direction(cmd);
            if (direction.is_some) {
                execute_direction(self, direction.unwrap);
            }
        }
        return;
    }
    self->is_modified = true;

    if (cmd == BACKSPACE) {
        if (self->is_recording) {
            prepend_undo_direction(self, reverse_direction(self->momentum));
            self->momentum = reverse_direction(self->momentum);
            prepend_undo_direction(self, self->momentum);
            keystroke_prepend_key(
                self->current_undo_keystroke,
                buffer_space_get(self->contents, self->cursor_pos));
            prepend_undo_direction(self, reverse_direction(self->momentum));
            self->momentum = reverse_direction(self->momentum);
            prepend_undo_direction(self, self->momentum);
        }
        buffer_space_put(self->contents, self->cursor_pos, ' ');
    } else {
        if (self->is_recording) {
            keystroke_prepend_key(
                self->current_undo_keystroke,
                buffer_space_get(self->contents, self->cursor_pos));
        }
        buffer_space_put(self->contents, self->cursor_pos, cmd);
        if (!self->is_replacing) {
            force_follow_momentum(self);
        }
    }
    if (self->is_replacing) {
        buffer_insert_cmd(self, ESC_KEY);
    }
}

static void execute_keystroke(Buffer *self, Keystroke *keystroke,
                              bool is_simulated) {
    size_t i;
    key_t key;
    for (i = 0; i < keystroke_len(keystroke); i++) {
        key = keystroke_get_key(keystroke, i);
        buffer_cmd(self, key, is_simulated);
    }
}

static void redo_last_action(Buffer *self, bool is_simulated) {
    Action *action = stack_peek(self->redo_stack);
    Keystroke *keystroke = action_get_keystroke(action);
    execute_keystroke(self, keystroke, is_simulated);
}

static void simulate_action(Buffer *self, Action *action) {
    Keystroke *keystroke = action_get_keystroke(action);
    while (action_get_pos(action).x < 0) {
        add_column_left(self);
    }
    while (action_get_pos(action).y < 0) {
        add_row_top(self);
    }
    self->cursor_pos = action_get_pos(action);
    self->momentum = action_get_momentum(action);
    execute_keystroke(self, keystroke, true);
}

static void undo(Buffer *self) {
    Action *undo_action, *redo_action;
    if (stack_len(self->undo_stack) > self->stack_idx) {
        undo_action = stack_get(self->undo_stack, self->stack_idx);
        redo_action = stack_get(self->redo_stack, self->stack_idx);
        simulate_action(self, undo_action);

        while (action_get_pos(redo_action).x < 0) {
            add_column_left(self);
        }
        while (action_get_pos(redo_action).y < 0) {
            add_row_top(self);
        }
        self->cursor_pos = action_get_pos(redo_action);

        self->momentum = action_get_momentum(redo_action);
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
    StringBuilder *line = string_builder_create();
    vector_t pos;
    pos.y = self->cursor_pos.y;

    for (pos.x = 0; pos.x < buffer_space_bottom_right(self->contents).x;
         pos.x++) {
        string_builder_append_char(line, buffer_space_get(self->contents, pos));
    }
    return line;
}

static StringBuilder *snatch_vertical_line(Buffer *self) {
    StringBuilder *line = string_builder_create();
    vector_t pos;
    pos.x = self->cursor_pos.x;

    for (pos.y = 0; pos.y < buffer_space_bottom_right(self->contents).y;
         pos.y++) {
        string_builder_append_char(line, buffer_space_get(self->contents, pos));
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
        self->cursor_pos.x = line_end;
    } else {
        self->cursor_pos.y = line_end;
    }
    string_builder_destroy(line);
}

typedef struct {
    vector_t top_left;
    vector_t bottom_right;
} selection_t;

static selection_t get_selection(Buffer *self) {
    selection_t selection;
    if (self->mode == SELECT) {
        if (self->selection_start_pos.x < self->cursor_pos.x) {
            selection.top_left.x = self->selection_start_pos.x;
            selection.bottom_right.x = self->cursor_pos.x;
        } else {
            selection.top_left.x = self->cursor_pos.x;
            selection.bottom_right.x = self->selection_start_pos.x;
        }
        if (self->selection_start_pos.y < self->cursor_pos.y) {
            selection.top_left.y = self->selection_start_pos.y;
            selection.bottom_right.y = self->cursor_pos.y;
        } else {
            selection.top_left.y = self->cursor_pos.y;
            selection.bottom_right.y = self->selection_start_pos.y;
        }
    } else {
        selection.top_left = self->cursor_pos;
        selection.bottom_right = self->cursor_pos;
    }
    return selection;
}

static void yank_selection(Buffer *self) {
    selection_t selection = get_selection(self);
    vector_t pos;

    keystroke_destroy(*self->yanked);
    *self->yanked = keystroke_create();
    keystroke_append_key(*self->yanked, 'i');

    for (pos.y = selection.top_left.y; pos.y <= selection.bottom_right.y;
         pos.y++) {
        for (pos.x = selection.top_left.x; pos.x <= selection.bottom_right.x;
             pos.x++) {
            keystroke_append_key(*self->yanked,
                                 buffer_space_get(self->contents, pos));
        }
        keystroke_append_key(*self->yanked, '\n');
    }

    keystroke_append_key(*self->yanked, ESC_KEY);
}

static void cut_selection(Buffer *self, bool is_simulated) {
    Keystroke *delete_selection = keystroke_create();
    key_t key;
    size_t i;
    size_t yanked_len;
    selection_t selection = get_selection(self);
    yank_selection(self);
    yanked_len = keystroke_len(*self->yanked);

    keystroke_append_key(delete_selection, 'i');
    for (i = 1; i < yanked_len - 1; i++) {
        key = keystroke_get_key(*self->yanked, i);
        if (key == '\n' || (!isprint(key) && read_direction(key).is_some)) {
            keystroke_append_key(delete_selection, key);
        } else {
            keystroke_append_key(delete_selection, ' ');
        }
    }
    keystroke_append_key(delete_selection, ESC_KEY);

    self->cursor_pos = selection.top_left;
    self->momentum = RIGHT;

    execute_keystroke(self, delete_selection, is_simulated);
    keystroke_destroy(delete_selection);
}

/**
 * Either a normal or a select command.
 */
static void buffer_normal_cmd(Buffer *self, key_t cmd, bool is_simulated) {
    option_direction_t direction = read_direction(cmd);

    if (direction.is_some) {
        if (self->mode == NORMAL) {
            execute_direction(self, direction.unwrap);
        } else {
            self->momentum = direction.unwrap;
            follow_momentum(self);
        }
    } else {
        switch (cmd) {
        case ESC_KEY: self->mode = NORMAL; break;
        case 'v':
            if (self->mode == NORMAL) {
                self->selection_start_pos = self->cursor_pos;
                self->mode = SELECT;
            }
            break;
        case 'r':
        case 'i':
        case 'a':
        case 'A':
        case 'I':
            if (!is_simulated) {
                begin_recording_action(self);
            }
            if (cmd == 'a') {
                force_follow_momentum(self);
            } else if (cmd == 'A') {
                jump_line_end(self, false);
                force_follow_momentum(self);
            } else if (cmd == 'I') {
                jump_line_end(self, true);
            } else if (cmd == 'r') {
                self->is_replacing = true;
            }
            self->insert_start_pos = self->cursor_pos;
            if (!is_simulated) {
                keystroke_append_key(self->current_redo_keystroke, cmd);
                keystroke_prepend_key(self->current_undo_keystroke, ESC_KEY);
            }
            self->mode = INSERT;
            break;
        case '$': jump_line_end(self, false); break;
        case '^': jump_line_end(self, true); break;
        case '.': redo_last_action(self, is_simulated); break;
        case 'u':
            self->mode = NORMAL;
            undo(self);
            break;
        case 'U':
            self->mode = NORMAL;
            redo(self);
            break;
        case 'p':
            self->mode = NORMAL;
            if (*self->yanked) {
                execute_keystroke(self, *self->yanked, false);
            }
            break;
        case 'y': yank_selection(self); break;
        case 'd':
            if (!is_simulated) {
                begin_recording_action(self);
            }
            cut_selection(self, is_simulated);
            break;
        default: break;
        }
    }
}

void buffer_cmd(Buffer *self, key_t cmd, bool is_simulated) {
    if (self->mode == INSERT) {
        buffer_insert_cmd(self, cmd);
    } else {
        buffer_normal_cmd(self, cmd, is_simulated);
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

    if (self->cursor_pos.y < min_row) {
        self->top_offset = self->cursor_pos.y;
    } else if (self->cursor_pos.y > max_row) {
        self->top_offset = self->cursor_pos.y - row_ct + 1;
    }

    if (self->cursor_pos.x < min_col) {
        self->left_offset = self->cursor_pos.x;
    } else if (self->cursor_pos.x > max_col) {
        self->left_offset = self->cursor_pos.x - col_ct + 1;
    }
}

static bool is_selected(Buffer *self, vector_t pos) {
    bool is_row_selected, is_col_selected;

    if (self->mode != SELECT) {
        return false;
    }
    is_row_selected
        = (self->selection_start_pos.y <= pos.y && pos.y <= self->cursor_pos.y)
          || (pos.y <= self->selection_start_pos.y
              && self->cursor_pos.y <= pos.y);
    is_col_selected
        = (self->selection_start_pos.x <= pos.x && pos.x <= self->cursor_pos.x)
          || (pos.x <= self->selection_start_pos.x
              && self->cursor_pos.x <= pos.x);
    return is_row_selected && is_col_selected;
}

void buffer_build_display(Buffer *self, StringBuilder *display,
                          uint16_t top_offset, uint16_t left_offset,
                          uint16_t row_ct, uint16_t col_ct) {
    vector_t pos;
    char contents_char;

    fit_frame_to_cursor(self, row_ct, col_ct);
    move_cursor(display, top_offset + 1, left_offset + 1);

    for (pos.y = self->top_offset; pos.y < row_ct + self->top_offset; pos.y++) {
        move_cursor(display, pos.y + top_offset - self->top_offset + 1,
                    left_offset + 1);
        for (pos.x = self->left_offset; pos.x < col_ct + self->left_offset;
             pos.x++) {
            contents_char = buffer_space_get(self->contents, pos);
            if (contents_char == '\t' || contents_char == '\n'
                || contents_char == '\r') {
                /* TODO - how should tabs be displayed? is this best way? */
                contents_char = ' ';
            } else if (!isprint(contents_char)) {
                /* TODO - what about non-printable characters? */
                contents_char = '?';
            }
            if (is_selected(self, pos)) {
                string_builder_append(display, HIGHLIGHT);
                string_builder_append_char(display, contents_char);
                string_builder_append(display, RESET);
            } else {
                string_builder_append_char(display, contents_char);
            }
        }
    }

    move_cursor(display, top_offset + 1 + self->cursor_pos.y - self->top_offset,
                left_offset + 1 + self->cursor_pos.x - self->left_offset);
}

size_t buffer_get_row(Buffer *self) {
    return self->cursor_pos.y + 1;
}

size_t buffer_get_col(Buffer *self) {
    return self->cursor_pos.x + 1;
}

direction_t buffer_get_momentum(Buffer *self) {
    return self->momentum;
}

char *buffer_name(Buffer *self) {
    return self->filename;
}

void buffer_clean_whitespace(Buffer *self) {
    int32_t old_last_row = buffer_space_bottom_right(self->contents).y;
    int32_t new_last_row;
    int32_t rows_removed;
    buffer_space_clean_whitespace(self->contents);
    new_last_row = buffer_space_bottom_right(self->contents).y;
    rows_removed = old_last_row - new_last_row;
    if (self->cursor_pos.y >= rows_removed) {
        self->cursor_pos.y -= rows_removed;
    }
    if (self->top_offset >= rows_removed) {
        self->top_offset -= rows_removed;
    }
}

bool buffer_save(Buffer *self) {
    FILE *file = fopen(self->filename, "w");
    if (!file) {
        return false;
    }
    buffer_space_write(self->contents, file);
    fclose(file);
    self->is_modified = false;
    return true;
}

bool buffer_is_modified(Buffer *self) {
    return self->is_modified;
}

#define CHUNK_SIZE 128

Buffer *buffer_create(const char *filename, Keystroke **yanked) {
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

    self->contents = buffer_space_create(self->filename);
    if (!self->contents) goto buffer_create_fail;

    self->cursor_pos.y = 0;
    self->cursor_pos.x = 0;
    self->top_offset = 0;
    self->rows_past_end = 0;
    self->left_offset = 0;
    self->momentum = RIGHT;
    self->is_modified = false;
    self->mode = NORMAL;
    self->is_replacing = false;

    self->is_recording = false;
    self->current_redo_keystroke = NULL;
    self->current_undo_keystroke = NULL;

    self->redo_stack = stack_create((void (*)(void *))action_destroy);
    if (!self->redo_stack) goto buffer_create_fail;
    self->undo_stack = stack_create((void (*)(void *))action_destroy);
    if (!self->undo_stack) goto buffer_create_fail;
    self->stack_idx = 0;

    self->yanked = yanked;

    return self;
buffer_create_fail:
    buffer_destroy(self);
    return NULL;
}

void buffer_destroy(Buffer *self) {
    if (self) {
        free(self->filename);
        buffer_space_destroy(self->contents);

        stack_destroy(self->redo_stack);
        stack_destroy(self->undo_stack);

        keystroke_destroy(self->current_redo_keystroke);
        keystroke_destroy(self->current_undo_keystroke);

        free(self);
    }
}
