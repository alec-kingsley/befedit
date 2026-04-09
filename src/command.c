#include "command.h"
#include "reporter.h"
#include <stdbool.h>
#include <string.h>

#define FILENAME "command.c"

struct Command {
    size_t arg_ct;
    command_t command;
    /* same as original command but spaces are nulled */
    char *arguments;
    /* length of arguments buffer */
    size_t arguments_len;
};

size_t command_arg_ct(Command *self) {
    return self->arg_ct;
}

char *command_get_arg(Command *self, size_t index) {
    bool last_was_null = true;
    char *ptr;
    size_t i = 0;
    for (ptr = self->arguments;
         ptr - self->arguments < (long)self->arguments_len; ptr++) {
        if (*ptr == '\0') {
            last_was_null = true;
        } else {
            if (last_was_null) {
                if (i == index) {
                    return ptr;
                }
                i++;
            }
            last_was_null = false;
        }
    }
    report_logic_error(FILENAME ": attempt to reach command out of bounds");
    exit(1);
}

command_t command_get_command(Command *self) {
    return self->command;
}

static void init_command(Command *self) {
    char *cmd;
    if (self->arg_ct == 0) {
        self->command = EMPTY;
        return;
    }
    cmd = command_get_arg(self, 0);
    if (strcmp(cmd, "w") == 0 || strcmp(cmd, "write") == 0) {
        self->command = WRITE;
    } else if (strcmp(cmd, "wq") == 0 || strcmp(cmd, "x") == 0
               || strcmp(cmd, "write-quit") == 0) {
        self->command = WRITE_QUIT;
    } else if (strcmp(cmd, "q") == 0 || strcmp(cmd, "quit") == 0) {
        self->command = QUIT;
    } else if (strcmp(cmd, "q!") == 0 || strcmp(cmd, "quit!") == 0) {
        self->command = FORCE_QUIT;
    } else if (strcmp(cmd, "wa") == 0 || strcmp(cmd, "write-all") == 0) {
        self->command = WRITE_ALL;
    } else if (strcmp(cmd, "wqa") == 0 || strcmp(cmd, "xa") == 0
               || strcmp(cmd, "write-quit-all") == 0) {
        self->command = WRITE_QUIT_ALL;
    } else if (strcmp(cmd, "qa") == 0 || strcmp(cmd, "quit-all") == 0) {
        self->command = QUIT_ALL;
    } else if (strcmp(cmd, "qa!") == 0 || strcmp(cmd, "quit-all!") == 0) {
        self->command = FORCE_QUIT_ALL;
    } else if (strcmp(cmd, "n") == 0 || strcmp(cmd, "next") == 0) {
        self->command = NEXT;
    } else if (strcmp(cmd, "cw") == 0 || strcmp(cmd, "clean-whitespace") == 0) {
        self->command = CLEAN_WHITESPACE;
    } else if (strcmp(cmd, "o") == 0 || strcmp(cmd, "open") == 0) {
        self->command = OPEN;
    } else if (strcmp(cmd, "config-open") == 0) {
        self->command = CONFIG_OPEN;
    } else if (strcmp(cmd, "config-reload") == 0) {
        self->command = CONFIG_RELOAD;
    } else {
        self->command = UNKNOWN;
    }
}

static void init_arguments(Command *self) {
    bool last_was_space = true;
    char *ptr;
    self->arg_ct = 0;
    for (ptr = self->arguments; *ptr; ptr++) {
        if (*ptr == ' ' || *ptr == '\t') {
            *ptr = '\0';
            last_was_space = true;
        } else {
            if (last_was_space) {
                self->arg_ct++;
            }
            last_was_space = false;
        }
    }
}

Command *command_create(const char *cmd) {
    Command *self = malloc(sizeof(Command));
    if (!self) goto command_create_fail;

    self->arguments_len = strlen(cmd);

    self->arguments = malloc(self->arguments_len + 1);
    if (!self->arguments) goto command_create_fail;

    self->arg_ct = 0;

    strcpy(self->arguments, cmd);
    init_arguments(self);
    init_command(self);

    return self;
command_create_fail:
    report_system_error(FILENAME ": memory allocation failure");
    command_destroy(self);
    return NULL;
}

void command_destroy(Command *self) {
    if (self) {
        free(self->arguments);
        free(self);
    }
}
