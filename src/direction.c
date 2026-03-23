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
