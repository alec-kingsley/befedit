#include "keystroke.h"
#include "reporter.h"
#include <string.h>

#define FILENAME "keystroke.c"

struct Keystroke {
    size_t len;  /* length of keystroke */
    size_t size; /* size in memory */
    key_t *val;  /* keystroke itself */
};

#define INITIAL_SIZE (32 * sizeof(key_t))

/**
 * Double the available size in the `Keystroke`.
 * Return `true` iff successful.
 */
static bool expand(Keystroke *self) {
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

size_t keystroke_len(Keystroke *self) {
    return self->len;
}

key_t keystroke_get_key(Keystroke *self, int32_t index) {
    size_t pos = index < 0 ? self->len + index : (size_t)index;
    return self->val[pos];
}

bool keystroke_append_key(Keystroke *self, key_t key) {
    const size_t new_size = (self->len + 1) * sizeof(key_t);
    while (self->size < new_size) {
        if (!expand(self)) {
            return false;
        }
    }
    self->val[self->len] = key;
    self->len++;
    return true;
}

bool keystroke_prepend_key(Keystroke *self, key_t key) {
    const size_t new_size = (self->len + 1) * sizeof(key_t);
    while (self->size < new_size) {
        if (!expand(self)) {
            return false;
        }
    }
    memmove(self->val + 1, self->val, self->len * sizeof(key_t));
    self->val[0] = key;
    self->len++;
    return true;
}

Keystroke *keystroke_create(void) {
    Keystroke *new = malloc(sizeof(Keystroke));
    if (!new) goto keystroke_create_fail;

    new->len = 0;
    new->size = INITIAL_SIZE;

    new->val = malloc(INITIAL_SIZE);
    if (!new->val) goto keystroke_create_fail;

    return new;

keystroke_create_fail:
    report_system_error(FILENAME ": memory allocation failure");
    keystroke_destroy(new);
    return NULL;
}

void keystroke_destroy(Keystroke *self) {
    if (self) {
        free(self->val);
        free(self);
    }
}
