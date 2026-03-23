#include "action.h"

struct Action {
    direction_t momentum;
    uint16_t row;
    uint16_t col;
    Keystroke *keystroke;
};

Keystroke *action_get_keystroke(Action *self) {
    return self->keystroke;
}

uint16_t action_get_row(Action *self) {
    return self->row;
}

uint16_t action_get_col(Action *self) {
    return self->col;
}

direction_t action_get_momentum(Action *self) {
    return self->momentum;
}

Action *action_create(Keystroke *keystroke, uint16_t row, uint16_t col,
                      direction_t momentum) {
    Action *self = malloc(sizeof(Action));
    if (self == NULL) goto action_create_fail;

    self->keystroke = keystroke;
    self->row = row;
    self->col = col;
    self->momentum = momentum;

    return self;
action_create_fail:
    action_destroy(self);
    return NULL;
}

void action_destroy(Action *self) {
    if (self) {
        keystroke_destroy(self->keystroke);
        free(self);
    }
}
