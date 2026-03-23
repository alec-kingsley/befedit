#include "stack.h"
#include "reporter.h"
#include <stdio.h>

#define FILENAME "stack.c"

typedef struct Node Node;

struct Node {
    void *val;
    Node *next;
};

struct Stack {
    Node *head;
    void (*free_fun)(void *);
};

size_t stack_len(Stack *self) {
    Node *ptr = self->head;
    size_t len = 0;
    while (ptr != NULL) {
        ptr = ptr->next;
        len++;
    }
    return len;
}

bool stack_is_empty(Stack *self) {
    return self->head == NULL;
}

bool stack_push(Stack *self, void *val) {
    Node *new = malloc(sizeof(Node));
    if (new == NULL) goto stack_push_fail;

    new->val = val;
    new->next = self->head;
    self->head = new;
    return true;

stack_push_fail:
    return false;
}

void *stack_peek(Stack *self) {
    if (self->head == NULL) {
        report_logic_error("attempt to peek at empty stack");
        exit(1);
    }
    return self->head->val;
}

void *stack_get(Stack *self, size_t index) {
    size_t i = 0;
    Node *node = self->head;
    while (i < index) {
        if (node == NULL) {
            report_logic_error("attempt to get past stack end");
            exit(1);
        }
        node = node->next;
        i++;
    }
    if (node == NULL) {
        report_logic_error("attempt to get past stack end");
        exit(1);
    }
    return node->val;
}

void *stack_pop(Stack *self) {
    Node *old_head = self->head;
    void *val;
    if (old_head == NULL) {
        report_logic_error("attempt to pop from empty stack");
        exit(1);
    }
    val = old_head->val;
    self->head = old_head->next;
    free(old_head);
    return val;
}

Stack *stack_create(void (*free_fun)(void *)) {
    Stack *new = malloc(sizeof(Stack));
    if (new == NULL) goto stack_create_fail;

    new->head = NULL;
    new->free_fun = free_fun;
    return new;

stack_create_fail:
    report_system_error(FILENAME ": memory allocation failure");
    stack_destroy(new);
    return NULL;
}

void stack_destroy(Stack *self) {
    if (self != NULL) {
        if (self->free_fun != NULL) {
            while (self->head != NULL) {
                self->free_fun(stack_pop(self));
            }
        }
        free(self);
    }
}
