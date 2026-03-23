#include "string_builder.h"
#include "reporter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILENAME "string_builder.c"

struct StringBuilder {
    size_t len;  /* length of string */
    size_t size; /* size in memory */
    char *val;   /* string itself */
};

#define INITIAL_SIZE (32 * sizeof(char))

/**
 * Double the available size in the `StringBuilder`.
 * Return `true` iff successful.
 */
static bool expand(StringBuilder *self) {
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

size_t string_builder_len(StringBuilder *self) { return self->len; }

char *string_builder_to_string(StringBuilder *self) {
    if (self->len + 1 > self->size) {
        expand(self);
    }
    /* null-terminate string for safety */
    self->val[self->len] = '\x00';
    return self->val;
}

char string_builder_get_char(StringBuilder *self, int32_t index) {
    size_t pos = index < 0 ? self->len + index : (size_t)index;
    return self->val[pos];
}

void string_builder_set_char(StringBuilder *self, int32_t index, char c) {
    size_t pos = index < 0 ? self->len + index : (size_t)index;
    self->val[pos] = c;
}

bool string_builder_set(StringBuilder *self, const char *new) {
    const size_t new_len = strlen(new);
    while (self->size < new_len) {
        if (!expand(self)) {
            return false;
        }
    }
    memcpy(self->val, new, new_len);
    self->len = new_len;
    return true;
}

bool string_builder_insert(StringBuilder *self, size_t index,
                           const char *other) {
    const size_t other_len = strlen(other);
    const size_t new_size = (self->len + other_len) * sizeof(char);
    while (self->size < new_size) {
        if (!expand(self)) {
            return false;
        }
    }
    memmove(self->val + index + other_len, self->val + index,
            self->len - index);
    memcpy(self->val + index, other, other_len);
    self->len += other_len;
    return true;
}

bool string_builder_append_char(StringBuilder *self, char other) {
    const size_t new_size = (self->len + 1) * sizeof(char);
    while (self->size < new_size) {
        if (!expand(self)) {
            return false;
        }
    }
    self->val[self->len] = other;
    self->len++;
    return true;
}

bool string_builder_append(StringBuilder *self, const char *other) {
    const size_t other_len = strlen(other);
    const size_t new_size = (self->len + other_len) * sizeof(char);
    while (self->size < new_size) {
        if (!expand(self)) {
            return false;
        }
    }
    memcpy(self->val + self->len, other, other_len);
    self->len += other_len;
    return true;
}

void string_builder_restrict(StringBuilder *self, size_t start, int32_t end) {
    const size_t end_pos = end > 0 ? (size_t)end : self->len + end;
    const size_t new_len = end_pos - start;
    memmove(self->val, self->val + start, new_len);
    self->len = new_len;
}

StringBuilder *string_builder_create(void) {
    StringBuilder *new = malloc(sizeof(StringBuilder));
    if (!new) goto string_builder_create_fail;

    new->len = 0;
    new->size = INITIAL_SIZE;

    new->val = malloc(INITIAL_SIZE);
    if (!new->val) goto string_builder_create_fail;

    return new;

string_builder_create_fail:
    report_system_error(FILENAME ": memory allocation failure");
    string_builder_destroy(new);
    return NULL;
}

void string_builder_destroy(StringBuilder *self) {
    if (self) {
        free(self->val);
        free(self);
    }
}
