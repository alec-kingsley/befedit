#pragma once
#ifndef DIRECTION_H
#define DIRECTION_H

#include "terminal.h"

typedef enum { UP, DOWN, LEFT, RIGHT } direction_t;

direction_t reverse_direction(direction_t direction);

key_t direction_as_key(direction_t direction);

direction_t rotate_90_degrees(direction_t direction);

uint8_t angle_degrees_between(direction_t a, direction_t b);

#endif
