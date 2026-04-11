#include "funge_space.h"
#include "funge_line.h"
#include "reporter.h"
#include <stdio.h>
#include <string.h>

#define FILENAME "funge_space.c"

#define INITIAL_LINE_CT 128

struct FungeSpace {
    FungeLine **lines;
    size_t line_ct;

    /* index of lines[0][0] */
    vector_t top_left;

    /* borders of funge space */
    vector_t funge_top_left;
    vector_t funge_bottom_right;
};

/**
 * Double the available size in `lines`.
 */
static void expand_lines(FungeSpace *self) {
    const size_t new_line_ct = self->line_ct * 2;
    void *new;
    size_t i;

    new = realloc(self->lines, new_line_ct * sizeof(FungeLine *));
    if (!new) {
        /* allocation failed */
        free(self->lines);
        report_system_error(FILENAME ": memory allocation failure");
        exit(1);
    }
    self->lines = new;
    for (i = self->line_ct; i < new_line_ct; i++) {
        self->lines[i] = funge_line_create();
    }
    self->line_ct = new_line_ct;
}

static void funge_space_ensure_y_exists(FungeSpace *self, int32_t y) {
    size_t distance;
    size_t i;
    if (y < self->top_left.y) {
        distance = self->top_left.y - y;
        if (self->line_ct + distance == self->line_ct) {
            expand_lines(self);
        }
        for (i = self->line_ct - distance; i < self->line_ct; i++) {
            funge_line_destroy(self->lines[i]);
        }
        memmove(&self->lines[distance], self->lines,
                (self->line_ct - distance) * sizeof(FungeLine *));
        for (i = 0; i < distance; i++) {
            self->lines[i] = funge_line_create();
        }
        self->top_left.y = y;
    } else {
        distance = y - self->top_left.y;
        if (distance >= self->line_ct) {
            expand_lines(self);
        }
    }
}

static void funge_space_ensure_x_exists(FungeSpace *self, int32_t x) {
    size_t i, j;
    size_t distance;
    if (x < self->top_left.x) {
        distance = self->top_left.x - x;
        for (i = 0; i < self->line_ct; i++) {
            for (j = 0; j < distance; j++) {
                funge_line_insert(self->lines[i], 0, ' ');
            }
        }
        self->top_left.x = x;
    }
}

static void shrink_funge_corners_to_fit(FungeSpace *self) {
    char c;
    vector_t new_top_left = {INT32_MAX, INT32_MAX};
    vector_t new_bottom_right = {INT32_MIN, INT32_MIN};
    vector_t pos;
    for (pos.y = self->funge_top_left.y; pos.y < self->funge_bottom_right.y;
         pos.y++) {
        for (pos.x = self->funge_top_left.x; pos.x < self->funge_bottom_right.x;
             pos.x++) {
            c = funge_space_get(self, pos);
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
        self->funge_top_left = new_top_left;
        self->funge_bottom_right = new_bottom_right;
    }
}

/**
 * `put` that explicitly cannot shrink the funge space.
 */
static void funge_space_put_cant_shrink(FungeSpace *self, vector_t pos,
                                        funge_cell_t n) {
    FungeLine *line;
    if (n != ' ') {
        if (pos.x < self->funge_top_left.x) {
            self->funge_top_left.x = pos.x;
        }
        if (pos.x > self->funge_bottom_right.x) {
            self->funge_bottom_right.x = pos.x;
        }
        if (pos.y < self->funge_top_left.y) {
            self->funge_top_left.y = pos.y;
        }
        if (pos.y > self->funge_bottom_right.y) {
            self->funge_bottom_right.y = pos.y;
        }
    }
    funge_space_ensure_y_exists(self, pos.y);
    funge_space_ensure_x_exists(self, pos.x);
    line = self->lines[pos.y - self->top_left.y];
    while ((int32_t)funge_line_len(line) <= pos.x - self->top_left.x) {
        funge_line_append(line, ' ');
    }
    funge_line_set(line, pos.x - self->top_left.x, n);
}

void funge_space_put(FungeSpace *self, vector_t pos, funge_cell_t n) {
    funge_space_put_cant_shrink(self, pos, n);
    if (n == ' ') {
        if (pos.x == self->funge_top_left.x
            || pos.x == self->funge_bottom_right.x
            || pos.y == self->funge_top_left.y
            || pos.y == self->funge_bottom_right.y) {
            /* TODO - call when it actually works */
            (void)shrink_funge_corners_to_fit;
        }
    }
}

funge_cell_t funge_space_get(FungeSpace *self, vector_t pos) {
    FungeLine *line;
    if (pos.x < self->funge_top_left.x || pos.x > self->funge_bottom_right.x
        || pos.y < self->funge_top_left.y
        || pos.y > self->funge_bottom_right.y) {
        return 0;
    } else {
        line = self->lines[pos.y - self->top_left.y];
        if ((int32_t)funge_line_len(line) <= pos.x - self->top_left.x) {
            return ' ';
        }
        return funge_line_get(line, pos.x - self->top_left.x);
    }
}

vector_t funge_space_top_left(FungeSpace *self) {
    return self->funge_top_left;
}

vector_t funge_space_bottom_right(FungeSpace *self) {
    return self->funge_bottom_right;
}

/* return true iff file was not empty */
static bool read_file_to_funge_space(FungeSpace *self, FILE *file) {
    size_t n;
    uint8_t c;
    vector_t pos = {0, 0};
    self->top_left.x = 0;
    self->top_left.y = 0;

    while ((n = fread(&c, 1, sizeof(char), file)) == 1) {
        if (c == '\r' || c == '\f') continue;
        if (c == '\n') {
            pos.y++;
            pos.x = 0;
        } else {
            funge_space_put_cant_shrink(self, pos, c);
            pos.x++;
        }
    }
    return pos.y != 0 || pos.x != 0;
}

/* TODO - throw error if empty */
FungeSpace *funge_space_create(const char *fname) {
    FungeSpace *self = calloc(1, sizeof(FungeSpace));
    FILE *file = NULL;
    size_t i;

    if (!self) {
        report_system_error(FILENAME ": memory allocation failure");
        goto funge_space_create_fail;
    }

    self->lines = calloc(INITIAL_LINE_CT, sizeof(FungeLine *));
    if (!self->lines) {
        report_system_error(FILENAME ": memory allocation failure");
        goto funge_space_create_fail;
    }

    self->line_ct = INITIAL_LINE_CT;
    for (i = 0; i < INITIAL_LINE_CT; i++) {
        self->lines[i] = funge_line_create();
        if (!self->lines[i]) goto funge_space_create_fail;
    }

    file = fopen(fname, "r");
    if (!file) {
        report_system_error(FILENAME ": failed to open file");
        goto funge_space_create_fail;
    }

    self->funge_top_left.x = 0;
    self->funge_top_left.y = 0;
    self->funge_bottom_right.x = 0;
    self->funge_bottom_right.y = 0;
    if (!read_file_to_funge_space(self, file)) {
        report_system_error(FILENAME ": empty file");
        goto funge_space_create_fail;
    }
    fclose(file);

    return self;
funge_space_create_fail:
    funge_space_destroy(self);
    if (file) fclose(file);
    return NULL;
}

void funge_space_destroy(FungeSpace *self) {
    size_t i;
    if (self) {
        for (i = 0; i < self->line_ct; i++) {
            funge_line_destroy(self->lines[i]);
        }
        free(self->lines);
        free(self);
    }
}
