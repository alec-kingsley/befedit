#include "reporter.h"
#include "colors.h"
#include "terminal.h"
#include <stdlib.h>

void report_system_error(const char *error) {
    append_terminal_error(RED "SYSTEM ERROR: " RESET);
    append_terminal_error(error);
    append_terminal_error("\n");
}

void report_logic_error(const char *error) {
    append_terminal_error(RED "LOGIC ERROR: " RESET);
    append_terminal_error(error);
    append_terminal_error("\n");

    /* not safe to continue */
    exit(1);
}

void report_error(const char *error) {
    append_terminal_error(RED "ERROR: " RESET);
    append_terminal_error(error);
    append_terminal_error("\n");
}

void report_warning(const char *error) {
    append_terminal_error(ORANGE "WARNING: " RESET);
    append_terminal_error(error);
    append_terminal_error("\n");
}
