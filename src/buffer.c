#include "buffer.h"
#include "direction.h"
#include "reporter.h"
#include "string_builder.h"
#include "terminal.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define FILENAME "buffer.c"

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
};

static void follow_momentum(Buffer *self) {
    switch (self->momentum) {
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

static void follow_reverse_momentum(Buffer *self) {
    switch (self->momentum) {
    case RIGHT:
        if (self->cursor_col > 0) {
            self->cursor_col--;
        }
        break;
    case UP: self->cursor_row++; break;
    case DOWN:
        if (self->cursor_row > 0) {
            self->cursor_row--;
        }
        break;
    case LEFT: self->cursor_col++; break;
    }
}

void buffer_insert_cmd(Buffer *self, key_t cmd) {
    uint16_t row = 0, col = 0;
    size_t contents_idx = 0;
    size_t contents_len = string_builder_len(self->contents);
    if (cmd == BACKSPACE) {
        follow_reverse_momentum(self);
    } else if (!key_is_printable(cmd)) {
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
        string_builder_set_char(self->contents, contents_idx, ' ');
    } else {
        string_builder_set_char(self->contents, contents_idx, cmd);
        follow_momentum(self);
    }
}

void buffer_normal_cmd(Buffer *self, key_t cmd) {
    bool direction_cmd = true;
    direction_t direction;
    switch (cmd) {
    case 'h':
    case ARROW_LEFT: direction = LEFT; break;
    case 'j':
    case ARROW_DOWN: direction = DOWN; break;
    case 'k':
    case ARROW_UP: direction = UP; break;
    case 'l':
    case ARROW_RIGHT: direction = RIGHT; break;
    default: direction_cmd = false;
    }
    if (direction_cmd) {
        if (direction != self->momentum) {
            self->momentum = direction;
        } else {
            follow_momentum(self);
        }
    }
}

/**
 * Move frame as necessary to fit cursor.
 */
static void fit_frame_to_cursor(Buffer *self, uint16_t row_ct, uint16_t col_ct) {
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

void buffer_display(Buffer *self, uint16_t top_offset, uint16_t left_offset,
                    uint16_t row_ct, uint16_t col_ct) {
    uint16_t row = 0, col = 0;
    size_t contents_idx = 0;
    const size_t contents_len = string_builder_len(self->contents);
    StringBuilder *display = string_builder_create();
    char contents_char;
    fit_frame_to_cursor(self, row_ct, col_ct);
    move_cursor(display, top_offset + 1, left_offset + 1);
    while (row < row_ct + self->top_offset && contents_idx < contents_len) {
        contents_char = string_builder_get_char(self->contents, contents_idx++);
        if (contents_char == '\n') {
            row++;
            col = 0;
            move_cursor(display, row + top_offset - self->top_offset + 1, left_offset + 1);
        } else {
            if (col >= col_ct + self->left_offset) continue;
            if (contents_char == '\t') {
                /* TODO - how should tabs be displayed? is this best way? */
                contents_char = ' ';
            }
            if (col >= self->left_offset && row >= self->top_offset) {
                string_builder_append_char(display, contents_char);
            }
            col++;
        }
    }
    move_cursor(display, top_offset + 1 + self->cursor_row - self->top_offset,
                left_offset + 1 + self->cursor_col - self->left_offset);
    write(STDOUT_FILENO, string_builder_to_string(display),
          string_builder_len(display));
    string_builder_destroy(display);
}

void buffer_save(Buffer *self) {
    FILE *file = fopen(self->filename, "w");
    if (!file) {
        report_system_error(FILENAME ": failed to open file");
        exit(1);
    }
    fwrite(string_builder_to_string(self->contents), sizeof(char),
           string_builder_len(self->contents), file);
    fclose(file);
    self->is_modified = false;
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

    return self;
buffer_create_fail:
    buffer_destroy(self);
    return NULL;
}

void buffer_destroy(Buffer *self) {
    if (self) {
        free(self->filename);
        string_builder_destroy(self->contents);
    }
}
