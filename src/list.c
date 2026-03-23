#include "list.h"
#include "reporter.h"
#include <stdio.h>

#define FILENAME "list.c"

typedef struct Node Node;

struct Node {
    void *val;
    Node *next;
};

struct List {
    Node *head;
    void (*free_fun)(void *);
};

size_t list_len(List *self) {
    Node *ptr = self->head;
    size_t len = 0;
    while (ptr != NULL) {
        ptr = ptr->next;
        len++;
    }
    return len;
}

bool list_is_empty(List *self) {
    return self->head == NULL;
}

bool list_insert(List *self, void *val, size_t index) {
    size_t i;
    Node *trav;
    Node *new = malloc(sizeof(Node));
    if (new == NULL) goto list_insert_fail;

    new->val = val;
    if (index == 0) {
        new->next = self->head;
        self->head = new;
    } else {
        i = 1;
        trav = self->head;
        while (i < index) {
            trav = trav->next;
            if (trav == NULL) {
                report_logic_error("attempt to insert past end of list");
            }
            i++;
        }
        new->next = trav->next;
        trav->next = new;
    }
    return true;

list_insert_fail:
    return false;
}

void *list_get(List *self, size_t index) {
    size_t i = 0;
    Node *node = self->head;
    while (i < index) {
        if (node == NULL) {
            report_logic_error("attempt to get past list end");
            exit(1);
        }
        node = node->next;
        i++;
    }
    if (node == NULL) {
        report_logic_error("attempt to get past list end");
        exit(1);
    }
    return node->val;
}

void *list_remove(List *self, size_t index) {
    size_t i;
    Node *node = self->head;
    Node *trav;
    void *val;
    if (index == 0) {
        self->head = node->next;
    } else {
        i = 1;
        trav = node;
        node = node->next;
        while (i < index) {
            if (node == NULL) {
                report_logic_error("attempt to remove past list end");
                exit(1);
            }
            trav = node;
            node = node->next;
            i++;
        }
        if (node == NULL) {
            report_logic_error("attempt to remove past list end");
            exit(1);
        }
        trav->next = node->next;
    }
    val = node->val;
    free(node);
    return val;
}

List *list_create(void (*free_fun)(void *)) {
    List *new = malloc(sizeof(List));
    if (new == NULL) goto list_create_fail;

    new->head = NULL;
    new->free_fun = free_fun;
    return new;

list_create_fail:
    report_system_error(FILENAME ": memory allocation failure");
    list_destroy(new);
    return NULL;
}

void list_destroy(List *self) {
    if (self != NULL) {
        if (self->free_fun != NULL) {
            while (self->head != NULL) {
                self->free_fun(list_remove(self, 0));
            }
        }
        free(self);
    }
}
