#include "action.h"

struct Action {
    direction_t momentum;
    vector_t *pos;
    Keystroke *keystroke;
};

Keystroke *action_get_keystroke(Action *self) {
    return self->keystroke;
}

vector_t action_get_pos(Action *self) {
    return *self->pos;
}

void action_set_pos(Action *self, vector_t *pos) {
    self->pos = pos;
}

direction_t action_get_momentum(Action *self) {
    return self->momentum;
}

Action *action_create(Keystroke *keystroke, vector_t *pos,
                      direction_t momentum) {
    Action *self = malloc(sizeof(Action));
    if (self == NULL) goto action_create_fail;

    self->keystroke = keystroke;
    self->pos = pos;
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
