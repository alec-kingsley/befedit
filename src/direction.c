#include "direction.h"
#include "reporter.h"
#include <stdlib.h>

#define FILENAME "direction.c"

direction_t reverse_direction(direction_t direction) {
    switch (direction) {
    case UP: return DOWN;
    case DOWN: return UP;
    case RIGHT: return LEFT;
    case LEFT: return RIGHT;
    }
    report_logic_error(FILENAME ": unknown direction");
    exit(1);
}

key_t direction_as_key(direction_t direction) {
    switch (direction) {
    case UP: return ARROW_UP;
    case DOWN: return ARROW_DOWN;
    case RIGHT: return ARROW_RIGHT;
    case LEFT: return ARROW_LEFT;
    }
    report_logic_error(FILENAME ": unknown direction");
    exit(1);
}

direction_t rotate_90_degrees(direction_t direction) {
    switch (direction) {
    case UP: return RIGHT;
    case DOWN: return LEFT;
    case RIGHT: return DOWN;
    case LEFT: return UP;
    }
    report_logic_error(FILENAME ": unknown direction");
    exit(1);
}

uint8_t angle_degrees_between(direction_t a, direction_t b) {
    uint8_t angle;
    if (a == b) {
        angle = 0;
    } else if (rotate_90_degrees(rotate_90_degrees(a)) == b) {
        angle = 180;
    } else {
        angle = 90;
    }
    return angle;
}
