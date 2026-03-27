#include "key.h"
#include <stdio.h>

bool key_is_printable(key_t key) {
    return key < 0x80 && key != BACKSPACE && key > 0x1f;
}

void print_key(key_t key) {
    if (key_is_printable(key)) {
        putchar(key);
    } else if (CTRL('A') <= key && key <= CTRL('Z')) {
        printf("CTRL+%c", 'A' + key - CTRL('A'));
    } else {
        switch (key) {
        case ESC_KEY: printf("ESC"); break;
        case BACKSPACE: printf("BACKSPACE"); break;
        case ARROW_LEFT: printf("ARROW_LEFT"); break;
        case ARROW_RIGHT: printf("ARROW_RIGHT"); break;
        case ARROW_UP: printf("ARROW_UP"); break;
        case ARROW_DOWN: printf("ARROW_DOWN"); break;
        case DEL_KEY: printf("DEL"); break;
        case HOME_KEY: printf("HOME"); break;
        case END_KEY: printf("END"); break;
        case PAGE_UP: printf("PAGE_UP"); break;
        case PAGE_DOWN: printf("PAGE_DOWN"); break;
        }
    }
}
