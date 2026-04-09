#include "queue.h"
#include "reporter.h"
#include <stdio.h>

#define FILENAME "queue.c"

typedef struct Node Node;

struct Node {
    void *val;
    Node *next;
};

struct Queue {
    Node *head;
    void (*free_fun)(void *);
};

size_t queue_len(Queue *self) {
    Node *ptr = self->head;
    size_t len = 0;
    while (ptr != NULL) {
        ptr = ptr->next;
        len++;
    }
    return len;
}

bool queue_is_empty(Queue *self) {
    return self->head == NULL;
}

bool queue_enqueue(Queue *self, void *val) {
    Node *trav = self->head;
    Node *new = malloc(sizeof(Node));
    if (new == NULL) goto queue_enqueue_fail;

    new->val = val;
    new->next = NULL;

    if (trav == NULL) {
        self->head = new;
    } else {
        while (trav->next != NULL) {
            trav = trav->next;
        }
        trav->next = new;
    }
    return true;

queue_enqueue_fail:
    return false;
}

void *queue_dequeue(Queue *self) {
    Node *old_head = self->head;
    void *val;
    if (old_head == NULL) {
        report_logic_error("attempt to dequeue from empty queue");
        exit(1);
    }
    val = old_head->val;
    self->head = old_head->next;
    free(old_head);
    return val;
}

void *queue_peek(Queue *self) {
    if (self->head == NULL) {
        report_logic_error("attempt to peek at empty stack");
        exit(1);
    }
    return self->head->val;
}

Queue *queue_create(void (*free_fun)(void *)) {
    Queue *new = malloc(sizeof(Queue));
    if (new == NULL) goto queue_create_fail;

    new->head = NULL;
    new->free_fun = free_fun;
    return new;

queue_create_fail:
    report_system_error(FILENAME ": memory allocation failure");
    queue_destroy(new);
    return NULL;
}

void queue_destroy(Queue *self) {
    Node *node;
    if (self) {
        while (self->head != NULL) {
            node = queue_dequeue(self);
            if (self->free_fun != NULL) {
                self->free_fun(node);
            }
        }
        free(self);
    }
}
