#pragma once
#ifndef DIRECTION_H
#define DIRECTION_H

#include "terminal.h"

typedef enum { UP, DOWN, LEFT, RIGHT } direction_t;

direction_t reverse_direction(direction_t direction);

key_t direction_as_key(direction_t direction);

#endif
