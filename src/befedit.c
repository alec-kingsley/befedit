#include "colors.h"
#include "terminal.h"
#include "reporter.h"
#include "buffer.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define FILENAME "befedit.c"

static void update_screen(Buffer *buffer) {
    const uint16_t top_offset = 0, left_offset = 0;
    printf(CLEAR_SCREEN RESET_CURSOR SHOW_CURSOR);
    update_window_size();
    buffer_display(buffer, top_offset, left_offset, get_row_ct(), get_col_ct());
}

int main(int argc, char **argv) {
    char key;
    int read_status;
    bool keep_running = true;
    Buffer *buffer;
    enable_raw_mode();

    /* TODO - read multiple arguments */
    if (argc > 1) {
        buffer = buffer_create(argv[1]);
    } else {
        buffer = buffer_create("2dfile.txt");
    }
    if (!buffer) return 1;

    while (keep_running) {
        update_screen(buffer);
        read_status = read(STDIN_FILENO, &key, 1);
        if (read_status == 1) {
            switch (key) {
            case 'q':
                printf(CLEAR_SCREEN RESET_CURSOR SHOW_CURSOR);
                keep_running = false;
                break;
            default:
                buffer_normal_cmd(buffer, key);
                break;
            }
        } else if (read_status == -1 && errno != EAGAIN) {
            report_system_error(FILENAME ": failed to get user input");
            exit(1);
        }
    }

    return 0;
}


