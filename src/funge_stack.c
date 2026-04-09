#include "funge_stack.h"
#include "reporter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define FILENAME "funge_stack.c"

struct FungeStack {
    size_t size;       /* size of funge stack */
    size_t capacity;   /* size in memory */
    funge_cell_t *val; /* funge stack itself */
};

#define INITIAL_SIZE (32 * sizeof(funge_cell_t))

/**
 * Double the available size in the `FungeStack`.
 * Return `true` iff successful.
 */
static bool expand(FungeStack *self) {
    void *new;
    const size_t new_size = self->capacity * 2;

    new = realloc(self->val, new_size);
    if (!new) {
        /* allocation failed */
        free(self->val);
        report_system_error(FILENAME ": memory allocation failure");
        return false;
    }
    self->capacity = new_size;
    self->val = new;
    return true;
}

size_t funge_stack_size(FungeStack *self) {
    return self->size;
}

funge_cell_t funge_stack_pop(FungeStack *self) {
    funge_cell_t val;
    if (self->size == 0) {
        return 0;
    } else {
        val = self->val[self->size - 1];
        self->size--;
        return val;
    }
}

funge_cell_t funge_stack_get(FungeStack *self, size_t i) {
    if (i >= self->size) {
        return 0;
    } else {
        return self->val[self->size - i - 1];
    }
}

bool funge_stack_push(FungeStack *self, funge_cell_t cell) {
    const size_t new_size = (self->size + 1) * sizeof(funge_cell_t);
    while (self->capacity < new_size) {
        if (!expand(self)) {
            return false;
        }
    }
    self->val[self->size] = cell;
    self->size++;
    return true;
}

FungeStack *funge_stack_clone(FungeStack *self) {
    FungeStack *new = malloc(sizeof(FungeStack));
    if (!new) goto funge_stack_clone_fail;

    new->size = self->size;
    new->capacity = self->capacity;

    new->val = malloc(self->capacity);
    if (!new->val) goto funge_stack_clone_fail;

    memcpy(new->val, self->val, self->size * sizeof(funge_cell_t));

    return new;

funge_stack_clone_fail:
    report_system_error(FILENAME ": memory allocation failure");
    funge_stack_destroy(new);
    return NULL;
}

FungeStack *funge_stack_create(void) {
    FungeStack *new = malloc(sizeof(FungeStack));
    if (!new) goto funge_stack_create_fail;

    new->size = 0;
    new->capacity = INITIAL_SIZE;

    new->val = malloc(INITIAL_SIZE);
    if (!new->val) goto funge_stack_create_fail;

    return new;

funge_stack_create_fail:
    report_system_error(FILENAME ": memory allocation failure");
    funge_stack_destroy(new);
    return NULL;
}

void funge_stack_destroy(FungeStack *self) {
    if (self) {
        free(self->val);
        free(self);
    }
}
