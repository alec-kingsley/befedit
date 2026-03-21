#ifndef REPORTER_H
#define REPORTER_H

/**
 * Report an error to the user.
 * (system's fault)
 */
void report_system_error(const char *error);

/**
 * Report an error to the user.
 * (programmer's fault)
 */
void report_logic_error(const char *error);

/**
 * Report an error to the user.
 * (user's fault)
 */
void report_error(const char *error);

/**
 * Report a warning to the user.
 * (user's fault)
 */
void report_warning(const char *warning);

#endif
