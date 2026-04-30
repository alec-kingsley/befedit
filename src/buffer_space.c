#include "buffer_space.h"
#include "list.h"
#include "reporter.h"
#include "string_builder.h"
#include <stdio.h>
#include <string.h>

#define FILENAME "buffer_space.c"

#define INITIAL_LINE_CT 128

struct BufferSpace {
    const char *fname;
    StringBuilder **lines;
    size_t line_ct;

    /* borders of buffer space */
    vector_t buffer_top_left;
    vector_t buffer_bottom_right;

    /* when the user requests a coordinate, it gets stored here */
    /* TODO - replace with dynlist */
    List *coordinates; /* List<vector_t*> */

    bool is_new_file;
};

static void buffer_space_remove_spaces_from_line_ends(BufferSpace *self) {
    int32_t i, j;
    char c;
    StringBuilder *line;
    for (i = self->buffer_top_left.y; i <= self->buffer_bottom_right.y; i++) {
        line = self->lines[i];
        j = string_builder_len(line) - 1;
        while (j >= 0) {
            c = string_builder_get_char(line, j);
            if (c != ' ' && c != '\t') {
                string_builder_restrict(line, 0, j + 1);
                break;
            }
            j--;
        }
    }
}

void buffer_space_clean_whitespace(BufferSpace *self) {
    while (self->buffer_top_left.y > 0) {
        buffer_space_remove_row(self, 0);
    }
    while (self->buffer_top_left.x > 0) {
        buffer_space_remove_col(self, 0);
    }
    buffer_space_remove_spaces_from_line_ends(self);
}

vector_t *buffer_space_get_coordinate(BufferSpace *self, vector_t pos) {
    vector_t *coordinate = malloc(sizeof(vector_t));
    if (!coordinate) {
        report_system_error(FILENAME ": memory allocation failure");
        return NULL;
    }
    *coordinate = pos;
    list_insert(self->coordinates, coordinate, 0);
    return coordinate;
}

/**
 * Double the available size in `lines`.
 */
static void expand_lines(BufferSpace *self, bool at_end) {
    const size_t new_line_ct = self->line_ct * 2;
    void *new;
    size_t i;

    new = realloc(self->lines, new_line_ct * sizeof(StringBuilder *));
    if (!new) {
        /* allocation failed */
        free(self->lines);
        report_system_error(FILENAME ": memory allocation failure");
        exit(1);
    }
    self->lines = new;
    if (at_end) {
        for (i = self->line_ct; i < new_line_ct; i++) {
            self->lines[i] = string_builder_create();
        }
    } else {
        memmove(&self->lines[self->line_ct], self->lines,
                self->line_ct * sizeof(StringBuilder *));
        for (i = 0; i < self->line_ct; i++) {
            self->lines[i] = string_builder_create();
        }
    }
    self->line_ct = new_line_ct;
}

static void buffer_space_ensure_y_exists(BufferSpace *self, int32_t y) {
    bool at_end;
    if (y < 0) {
        at_end = false;
        expand_lines(self, at_end);
    } else if ((size_t)y >= self->line_ct) {
        at_end = true;
        expand_lines(self, at_end);
    }
}

static void buffer_space_ensure_x_exists(BufferSpace *self, int32_t x) {
    size_t i, j;
    if (x < 0) {
        for (i = 0; i < self->line_ct; i++) {
            for (j = 0; j < (size_t)(-x); j++) {
                string_builder_insert_char(self->lines[i], 0, ' ');
            }
        }
    }
}

static void shrink_buffer_corners_to_fit(BufferSpace *self) {
    char c;
    vector_t new_top_left = {INT32_MAX, INT32_MAX};
    vector_t new_bottom_right = {INT32_MIN, INT32_MIN};
    vector_t pos;
    for (pos.y = self->buffer_top_left.y; pos.y <= self->buffer_bottom_right.y;
         pos.y++) {
        for (pos.x = self->buffer_top_left.x;
             pos.x <= self->buffer_bottom_right.x; pos.x++) {
            c = buffer_space_get(self, pos);
            if (c != ' ') {
                if (pos.y < new_top_left.y) {
                    new_top_left.y = pos.y;
                }
                if (pos.y > new_bottom_right.y) {
                    new_bottom_right.y = pos.y;
                }
                if (pos.x < new_top_left.x) {
                    new_top_left.x = pos.x;
                }
                if (pos.x > new_bottom_right.x) {
                    new_bottom_right.x = pos.x;
                }
            }
        }
    }
    if (new_top_left.x != INT32_MAX && new_top_left.y != INT32_MAX
        && new_bottom_right.x != INT32_MIN && new_bottom_right.y != INT32_MIN) {
        self->buffer_top_left = new_top_left;
        self->buffer_bottom_right = new_bottom_right;
    }
}

/**
 * `put` that explicitly cannot shrink the buffer space.
 */
static void buffer_space_put_cant_shrink(BufferSpace *self, vector_t pos,
                                         char c) {
    StringBuilder *line;
    if (c != ' ') {
        if (pos.x < self->buffer_top_left.x) {
            self->buffer_top_left.x = pos.x;
        }
        if (pos.x > self->buffer_bottom_right.x) {
            self->buffer_bottom_right.x = pos.x;
        }
        if (pos.y < self->buffer_top_left.y) {
            self->buffer_top_left.y = pos.y;
        }
        if (pos.y > self->buffer_bottom_right.y) {
            self->buffer_bottom_right.y = pos.y;
        }
    }
    buffer_space_ensure_y_exists(self, pos.y);
    buffer_space_ensure_x_exists(self, pos.x);
    line = self->lines[pos.y];
    while ((int32_t)string_builder_len(line) <= pos.x) {
        string_builder_append_char(line, ' ');
    }
    string_builder_set_char(line, pos.x, c);
}

void buffer_space_put(BufferSpace *self, vector_t pos, char n) {
    buffer_space_put_cant_shrink(self, pos, n);
    if (n == ' ') {
        if (pos.x == self->buffer_top_left.x
            || pos.x == self->buffer_bottom_right.x
            || pos.y == self->buffer_top_left.y
            || pos.y == self->buffer_bottom_right.y) {
            shrink_buffer_corners_to_fit(self);
        }
    }
}

/**
 * add `offset` to all rows >= `row` in `self->coordinates`
 */
static void update_row_coordinates(BufferSpace *self, int32_t row,
                                   int32_t offset) {
    size_t i;
    vector_t *vector;
    for (i = 0; i < list_len(self->coordinates); i++) {
        vector = list_get(self->coordinates, i);
        if (vector->y >= row) {
            vector->y += offset;
        }
    }
}

/**
 * add `offset` to all cols >= `col` in `self->coordinates`
 */
static void update_col_coordinates(BufferSpace *self, int32_t col,
                                   int32_t offset) {
    size_t i;
    vector_t *vector;
    for (i = 0; i < list_len(self->coordinates); i++) {
        vector = list_get(self->coordinates, i);
        if (vector->x >= col) {
            vector->x += offset;
        }
    }
}

void buffer_space_insert_row(BufferSpace *self, int32_t row) {
    StringBuilder *string_builder;
    buffer_space_ensure_y_exists(self, row);
    if ((size_t)self->buffer_bottom_right.y == self->line_ct) {
        buffer_space_ensure_y_exists(self, self->line_ct);
    }
    /* guaranteed empty */
    string_builder = self->lines[self->line_ct - 1];
    memmove(&self->lines[row + 1], &self->lines[row],
            (self->line_ct - row - 1) * sizeof(StringBuilder *));
    self->lines[row] = string_builder;
    if (self->buffer_bottom_right.y >= row) {
        self->buffer_bottom_right.y++;
    }
    update_row_coordinates(self, row == 0 ? INT32_MIN : row, 1);
    if (self->buffer_top_left.y >= row) {
        self->buffer_top_left.y++;
    }
    if (self->buffer_bottom_right.y >= row) {
        self->buffer_bottom_right.y++;
    }
}

void buffer_space_remove_row(BufferSpace *self, int32_t row) {
    string_builder_destroy(self->lines[row]);
    memmove(&self->lines[row], &self->lines[row + 1],
            (self->line_ct - row - 1) * sizeof(StringBuilder *));
    update_row_coordinates(self, row == 0 ? INT32_MIN : 0, -1);
    self->line_ct--;
    if (self->buffer_top_left.y >= row) {
        self->buffer_top_left.y--;
    }
    if (self->buffer_bottom_right.y >= row) {
        self->buffer_bottom_right.y--;
    }
}

void buffer_space_insert_col(BufferSpace *self, int32_t col) {
    StringBuilder *line;
    size_t i;
    for (i = 0; i < self->line_ct; i++) {
        line = self->lines[i];
        if (string_builder_len(line) > (size_t)col) {
            string_builder_insert_char(line, col, ' ');
        }
    }
    update_col_coordinates(self, col == 0 ? INT32_MIN : 0, 1);
    if (self->buffer_top_left.x >= col) {
        self->buffer_top_left.x++;
    }
    if (self->buffer_bottom_right.x >= col) {
        self->buffer_bottom_right.x++;
    }
}

void buffer_space_remove_col(BufferSpace *self, int32_t col) {
    StringBuilder *line;
    size_t i;
    for (i = 0; i < self->line_ct; i++) {
        line = self->lines[i];
        if (string_builder_len(line) > (size_t)col) {
            string_builder_remove_char(line, col);
        }
    }
    update_col_coordinates(self, col == 0 ? INT32_MIN : 0, -1);
    if (self->buffer_top_left.x >= col) {
        self->buffer_top_left.x--;
    }
    if (self->buffer_bottom_right.x >= col) {
        self->buffer_bottom_right.x--;
    }
}

/**
 * Write buffer space to file.
 */
void buffer_space_write(BufferSpace *self, FILE *file) {
    size_t i;
    /* if you compare i to `self->line_ct` instead, it will print a bunch of
     * extra lines */
    for (i = 0; i <= (size_t)self->buffer_bottom_right.y; i++) {
        fwrite(string_builder_to_string(self->lines[i]), sizeof(char),
               string_builder_len(self->lines[i]), file);
        /* TODO - add setting for \r\n instead */
        fwrite("\n", sizeof(char), strlen("\n"), file);
    }
}

char buffer_space_get(BufferSpace *self, vector_t pos) {
    StringBuilder *line;
    if (pos.x < self->buffer_top_left.x || pos.x > self->buffer_bottom_right.x
        || pos.y < self->buffer_top_left.y
        || pos.y > self->buffer_bottom_right.y) {
        return ' ';
    } else {
        line = self->lines[pos.y];
        if ((int32_t)string_builder_len(line) <= pos.x) {
            return ' ';
        }
        return string_builder_get_char(line, pos.x);
    }
}

vector_t buffer_space_top_left(BufferSpace *self) {
    return self->buffer_top_left;
}

vector_t buffer_space_bottom_right(BufferSpace *self) {
    return self->buffer_bottom_right;
}

static void read_file_to_buffer_space(BufferSpace *self, FILE *file) {
    size_t n;
    uint8_t c;
    vector_t pos = {0, 0};

    while ((n = fread(&c, 1, sizeof(char), file)) == 1) {
        if (c == '\r') continue;
        if (c == '\n') {
            pos.y++;
            pos.x = 0;
        } else {
            buffer_space_put_cant_shrink(self, pos, c);
            pos.x++;
        }
    }
}

BufferSpace *buffer_space_create(const char *fname) {
    const vector_t origin = {0, 0};
    BufferSpace *self = calloc(1, sizeof(BufferSpace));
    FILE *file = NULL;
    size_t i;

    if (!self) {
        report_system_error(FILENAME ": memory allocation failure");
        goto buffer_space_create_fail;
    }

    self->lines = calloc(INITIAL_LINE_CT, sizeof(StringBuilder *));
    if (!self->lines) {
        report_system_error(FILENAME ": memory allocation failure");
        goto buffer_space_create_fail;
    }

    self->line_ct = INITIAL_LINE_CT;
    for (i = 0; i < INITIAL_LINE_CT; i++) {
        self->lines[i] = string_builder_create();
        if (!self->lines[i]) goto buffer_space_create_fail;
    }

    self->buffer_top_left = origin;
    self->buffer_bottom_right = origin;

    file = fopen(fname, "r");
    if (!file) {
        self->is_new_file = true;
    } else {
        read_file_to_buffer_space(self, file);
        fclose(file);
    }

    self->fname = fname;

    self->coordinates = list_create(free);

    return self;
buffer_space_create_fail:
    buffer_space_destroy(self);
    if (file) fclose(file);
    return NULL;
}

void buffer_space_destroy(BufferSpace *self) {
    size_t i;
    if (self) {
        for (i = 0; i < self->line_ct; i++) {
            string_builder_destroy(self->lines[i]);
        }
        free(self->lines);
        list_destroy(self->coordinates);
        free(self);
    }
}
