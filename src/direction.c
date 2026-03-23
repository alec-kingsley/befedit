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
