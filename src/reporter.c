#include "reporter.h"
#include "colors.h"
#include <stdio.h>
#include <stdlib.h>

void report_system_error(const char *error) {
    printf(CLEAR_SCREEN RESET_CURSOR SHOW_CURSOR);
    fprintf(stderr, RED "SYSTEM ERROR: " RESET "%s\r\n", error);
}

void report_logic_error(const char *error) {
    printf(CLEAR_SCREEN RESET_CURSOR SHOW_CURSOR);
    fprintf(stderr, RED "LOGIC ERROR: " RESET "%s\r\n", error);
    /* not safe to continue */
    exit(1);
}

void report_error(const char *error) {
    printf(CLEAR_SCREEN RESET_CURSOR SHOW_CURSOR);
    printf(RED "ERROR: " RESET "%s\r\n", error);
}

void report_warning(const char *error) {
    printf(CLEAR_SCREEN RESET_CURSOR SHOW_CURSOR);
    printf(ORANGE "WARNING: " RESET "%s\r\n", error);
}
