#include "action.h"

struct Action {
    direction_t momentum;
    int16_t row;
    int16_t col;
    Keystroke *keystroke;
};

Keystroke *action_get_keystroke(Action *self) {
    return self->keystroke;
}

int32_t action_get_row(Action *self) {
    return self->row;
}

void action_set_row(Action *self, int32_t row) {
    self->row = row;
}

int32_t action_get_col(Action *self) {
    return self->col;
}

void action_set_col(Action *self, int32_t col) {
    self->col = col;
}

direction_t action_get_momentum(Action *self) {
    return self->momentum;
}

Action *action_create(Keystroke *keystroke, int32_t row, int32_t col,
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
