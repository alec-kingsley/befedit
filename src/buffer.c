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
};

void buffer_normal_cmd(Buffer *self, char cmd) {
    bool direction_cmd = true;
    direction_t direction;
    switch (cmd) {
        case 'h':
            direction = LEFT;
            break;
        case 'j':
            direction = DOWN;
            break;
        case 'k':
            direction = UP;
            break;
        case 'l':
            direction = RIGHT;
            break;
        default:
            direction_cmd = false;
    }
    if (direction_cmd) {
        if (direction != self->momentum) {
            self->momentum = direction;
        } else {
            switch (direction) {
                case LEFT:
                    if (self->cursor_col > 0) {
                        self->cursor_col--;
                    }
                    break;
                case DOWN:
                    self->cursor_row++;
                    break;
                case UP:
                    if (self->cursor_row > 0) {
                        self->cursor_row--;
                    }
                    break;
                case RIGHT:
                    self->cursor_col++;
                    break;
            }
        }
    }
}

void buffer_display(Buffer *self, uint16_t top_offset, uint16_t left_offset,
                    uint16_t row_ct, uint16_t col_ct) {
    uint16_t row = 0, col = 0;
    size_t contents_idx = 0;
    const size_t contents_len = string_builder_len(self->contents);
    StringBuilder *display = string_builder_create();
    char contents_char[2];
    contents_char[1] = '\0';

    move_cursor(display, top_offset + 1, left_offset + 1);
    while (row < row_ct && contents_idx < contents_len) {
        *contents_char
            = string_builder_get_char(self->contents, contents_idx++);
        if (*contents_char == '\n') {
            row++;
            col = 0;
            move_cursor(display, row + top_offset + 1, col + left_offset + 1);
            continue;
        }
        col++;
        if (col >= col_ct) continue;
        string_builder_append(display, contents_char);
    }
    move_cursor(display, top_offset + 1 + self->cursor_row - self->top_offset,
                left_offset + 1 + self->cursor_col - self->left_offset);
    write(STDOUT_FILENO, string_builder_to_string(display),
          string_builder_len(display));
    string_builder_destroy(display);
}

#define CHUNK_SIZE 128

/**
 * Initialize buffer. Return `true` if new file.
 */
static bool init_buffer(Buffer *self) {
    FILE *file = fopen(self->filename, "r");
    char chunk[CHUNK_SIZE + 1];
    chunk[CHUNK_SIZE] = '\0';

    if (file) {
        while (fread(chunk, 1, CHUNK_SIZE, file)) {
            string_builder_append(self->contents, chunk);
        }
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
