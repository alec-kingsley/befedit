#include "funge_line.h"
#include "reporter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define FILENAME "funge_line.c"

struct FungeLine {
    size_t len;        /* length of funge line */
    size_t size;       /* size in memory */
    funge_cell_t *val; /* string itself */
};

#define INITIAL_SIZE (32 * sizeof(funge_cell_t))

/**
 * Double the available size in the `FungeLine`.
 * Return `true` iff successful.
 */
static bool expand(FungeLine *self) {
    void *new;
    const size_t new_size = self->size * 2;

    new = realloc(self->val, new_size);
    if (!new) {
        /* allocation failed */
        free(self->val);
        report_system_error(FILENAME ": memory allocation failure");
        return false;
    }
    self->size = new_size;
    self->val = new;
    return true;
}

size_t funge_line_len(FungeLine *self) {
    return self->len;
}

funge_cell_t funge_line_get(FungeLine *self, int32_t index) {
    size_t pos = index < 0 ? self->len + index : (size_t)index;
    return self->val[pos];
}

void funge_line_set(FungeLine *self, int32_t index, funge_cell_t c) {
    size_t pos = index < 0 ? self->len + index : (size_t)index;
    self->val[pos] = c;
}

bool funge_line_insert(FungeLine *self, size_t index, funge_cell_t other) {
    const size_t new_size = (self->len + 1) * sizeof(funge_cell_t);
    while (self->size < new_size) {
        if (!expand(self)) {
            return false;
        }
    }
    memmove(self->val + index + 1, self->val + index,
            (self->len - index) * sizeof(funge_cell_t));
    memcpy(self->val + index, &other, 1 * sizeof(funge_cell_t));
    self->len++;
    return true;
}

bool funge_line_append(FungeLine *self, funge_cell_t other) {
    const size_t new_size = (self->len + 1) * sizeof(funge_cell_t);
    while (self->size < new_size) {
        if (!expand(self)) {
            return false;
        }
    }
    self->val[self->len] = other;
    self->len++;
    return true;
}

FungeLine *funge_line_create(void) {
    FungeLine *new = malloc(sizeof(FungeLine));
    if (!new) goto funge_line_create_fail;

    new->len = 0;
    new->size = INITIAL_SIZE;

    new->val = malloc(INITIAL_SIZE);
    if (!new->val) goto funge_line_create_fail;

    return new;

funge_line_create_fail:
    report_system_error(FILENAME ": memory allocation failure");
    funge_line_destroy(new);
    return NULL;
}

void funge_line_destroy(FungeLine *self) {
    if (self) {
        free(self->val);
        free(self);
    }
}
