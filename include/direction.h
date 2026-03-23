#pragma once
#ifndef DIRECTION_H
#define DIRECTION_H

typedef enum { UP, DOWN, LEFT, RIGHT } direction_t;

direction_t reverse_direction(direction_t direction);

#endif

