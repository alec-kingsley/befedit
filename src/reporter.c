#include "reporter.h"
#include "colors.h"
#include <stdio.h>
#include <stdlib.h>

void report_system_error(const char *error) {
    fprintf(stderr, RED "SYSTEM ERROR: " RESET "%s\n", error);
}

void report_logic_error(const char *error) {
    fprintf(stderr, RED "LOGIC ERROR: " RESET "%s\n", error);
    /* not safe to continue */
    exit(1);
}

void report_error(const char *error) {
    printf(RED "ERROR: " RESET "%s\n", error);
}

void report_warning(const char *error) {
    printf(ORANGE "WARNING: " RESET "%s\n", error);
}
