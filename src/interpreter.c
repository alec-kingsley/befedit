#include "interpreter.h"
#include "editor.h"
#include "funge_space.h"
#include "funge_stack.h"
#include "keystroke.h"
#include "queue.h"
#include "reporter.h"
#include "stack.h"
#include "string_builder.h"
#include "vector.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FILENAME "interpreter.c"

#define VERSION_NUMBER 1
#define HANDPRINT 0x534249 /* SBI */

/* TODO - change these if/when implemented */
#define T_IMPLEMENTED 1
#define I_IMPLEMENTED 0
#define O_IMPLEMENTED 0
#define EQ_IMPLEMENTED 1
#define UNBUFFERED_IO 1

/**
 * Taken from the official documentation:
 * https://quuxplusone.github.io/Fungus/docs/spec98.html#System
 *
 * 0 = Unavailable
 * 1 = Equivalent to C-language system() call behaviour
 * 2 = Equivalent to interpretation by a specific shell or program
 *
 *     This shell or program is specified by the interpreter but should ideally
 * be customizable by the interpreter-user, if applicable. Befunge programs that
 * run under this paradigm should document what program they expect to interpret
 * the string passed to =.
 *
 * 3 = Equivalent to interpretation by the same shell as started this Funge
 * interpreter, if applicable
 *
 *     If the interpreter supports this paradigm, then in this manner, the user
 * executing a Befunge source can easily choose which shell to use for =
 * instructions.
 */
#define OPERATING_PARADIGM 2

uint16_t g_next_ip_id = 0;

typedef struct {
    funge_cell_t id;
    /* functions for A through Z */
    /* NULL if not existant */
    void (*funcs[26])(Interpreter *self);
} fingerprint_t;

typedef struct {
    bool string_mode;

    /* for string mode */
    bool last_was_space;

    vector_t pos;

    vector_t momentum;

    Stack *stack_stack; /* Stack<FungeStack> */

    /* not included in `stack_stack` but technically its head */
    FungeStack *stack;

    vector_t storage_offset;

    /* semantic stack for A through Z */
    /* Stack<(*funcs[26])(Interpreter *self)> */
    Stack *semantics[26];

    uint32_t id;
} InstructionPointer;

static InstructionPointer *instruction_pointer_create(void);
static InstructionPointer *instruction_pointer_clone(InstructionPointer *self);
static void instruction_pointer_destroy(InstructionPointer *self);

struct Interpreter {
    FungeSpace *funge_space;

    /* ips queue'd up to move into `self->ip` */
    Queue *other_ips;

    InstructionPointer *ip;

    int return_code;

    StringBuilder *output;

    /* an error has been reported */
    /* volatile since calling thread can modify it */
    volatile bool is_poisoned;

    /* not owned by `self`, just a reference */
    Editor *editor;
};

static void execute_instruction(Interpreter *self, funge_cell_t instr);

static void reflect(Interpreter *self);

static void interpreter_error(Interpreter *self, const char *error) {
    self->is_poisoned = true;
    string_builder_set(self->output, error);
}

/**
 * =========================
 *  Fingerprint Definitions
 * =========================
 */

static void bfdt_k(Interpreter *self) {
    funge_cell_t key = funge_stack_pop(self->ip->stack);
    funge_cell_t y
        = funge_stack_pop(self->ip->stack) + self->ip->storage_offset.y;
    funge_cell_t x
        = funge_stack_pop(self->ip->stack) + self->ip->storage_offset.x;
    vector_t pos;
    pos.x = x;
    pos.y = y;

    funge_stack_push(self->ip->stack, funge_space_get(self->funge_space, pos));

    if ('a' <= key && key <= 'z') {
        key = key - 'a' + 'A';
    }

    if (key < 'A' || key > 'Z') {
        reflect(self);
    } else {
        editor_registor_macro(self->editor, pos, key);
    }
}

static void bool_a(Interpreter *self) {
    funge_cell_t a = funge_stack_pop(self->ip->stack);
    funge_cell_t b = funge_stack_pop(self->ip->stack);
    funge_stack_push(self->ip->stack, a && b);
}

static void bool_n(Interpreter *self) {
    funge_cell_t a = funge_stack_pop(self->ip->stack);
    funge_stack_push(self->ip->stack, !a);
}

static void bool_o(Interpreter *self) {
    funge_cell_t a = funge_stack_pop(self->ip->stack);
    funge_cell_t b = funge_stack_pop(self->ip->stack);
    funge_stack_push(self->ip->stack, a || b);
}

static void bool_x(Interpreter *self) {
    funge_cell_t a = funge_stack_pop(self->ip->stack);
    funge_cell_t b = funge_stack_pop(self->ip->stack);
    funge_stack_push(self->ip->stack, !!a ^ !!b);
}

static void modu_m(Interpreter *self) {
    /* signed-result modulo */
    funge_cell_t b = funge_stack_pop(self->ip->stack);
    funge_cell_t a = funge_stack_pop(self->ip->stack);
    funge_cell_t r;
    if (b == 0) {
        funge_stack_push(self->ip->stack, 0);
    } else {
        r = a % b;
        if (r > 0 && b < 0) {
            funge_stack_push(self->ip->stack, r + b);
        } else {
            funge_stack_push(self->ip->stack, r);
        }
    }
}

static void modu_u(Interpreter *self) {
    /* Sam Holden's unsigned-result modulo */
    /* Who is Sam Holden??? */
    funge_cell_t b = funge_stack_pop(self->ip->stack);
    funge_cell_t a = funge_stack_pop(self->ip->stack);
    if (b == 0) {
        funge_stack_push(self->ip->stack, 0);
    } else {
        if (a % b < 0) {
            funge_stack_push(self->ip->stack, a % b + (b > 0 ? b : -b));
        } else {
            funge_stack_push(self->ip->stack, a % b);
        }
    }
}

static void modu_r(Interpreter *self) {
    /* C-language integer remainder */
    funge_cell_t b = funge_stack_pop(self->ip->stack);
    funge_cell_t a = funge_stack_pop(self->ip->stack);
    if (b == 0) {
        funge_stack_push(self->ip->stack, 0);
    } else {
        funge_stack_push(self->ip->stack, a % b);
    }
}

static void roma_c(Interpreter *self) {
    funge_stack_push(self->ip->stack, 100);
}

static void roma_d(Interpreter *self) {
    funge_stack_push(self->ip->stack, 500);
}

static void roma_i(Interpreter *self) {
    funge_stack_push(self->ip->stack, 1);
}

static void roma_l(Interpreter *self) {
    funge_stack_push(self->ip->stack, 50);
}

static void roma_m(Interpreter *self) {
    funge_stack_push(self->ip->stack, 1000);
}

static void roma_v(Interpreter *self) {
    funge_stack_push(self->ip->stack, 5);
}

static void roma_x(Interpreter *self) {
    funge_stack_push(self->ip->stack, 10);
}

#define FINGERPRINT_ID(name)                                                   \
    (name[3] + name[2] * 0x100 + name[1] * 0x10000 + name[0] * 0x1000000)

static const fingerprint_t FINGERPRINTS[] = {
    {FINGERPRINT_ID("BFDT"),
     {
         NULL,   /* A */
         NULL,   /* B */
         NULL,   /* C */
         NULL,   /* D */
         NULL,   /* E */
         NULL,   /* F */
         NULL,   /* G */
         NULL,   /* H */
         NULL,   /* I */
         NULL,   /* J */
         bfdt_k, /* K */
         NULL,   /* L */
         NULL,   /* M */
         NULL,   /* N */
         NULL,   /* O */
         NULL,   /* P */
         NULL,   /* Q */
         NULL,   /* R */
         NULL,   /* S */
         NULL,   /* T */
         NULL,   /* U */
         NULL,   /* V */
         NULL,   /* W */
         NULL,   /* X */
         NULL    /* Y */
     }},
    {FINGERPRINT_ID("BOOL"),
     {
         NULL,   /* A */
         bool_a, /* B */
         NULL,   /* C */
         NULL,   /* D */
         NULL,   /* E */
         NULL,   /* F */
         NULL,   /* G */
         NULL,   /* H */
         NULL,   /* I */
         NULL,   /* J */
         NULL,   /* K */
         NULL,   /* L */
         NULL,   /* M */
         bool_n, /* N */
         bool_o, /* O */
         NULL,   /* P */
         NULL,   /* Q */
         NULL,   /* R */
         NULL,   /* S */
         NULL,   /* T */
         NULL,   /* U */
         NULL,   /* V */
         NULL,   /* W */
         bool_x, /* X */
         NULL    /* Y */
     }},
    {FINGERPRINT_ID("NULL"),
     {
         reflect, /* A */
         reflect, /* B */
         reflect, /* C */
         reflect, /* D */
         reflect, /* E */
         reflect, /* F */
         reflect, /* G */
         reflect, /* H */
         reflect, /* I */
         reflect, /* J */
         reflect, /* K */
         reflect, /* L */
         reflect, /* M */
         reflect, /* N */
         reflect, /* O */
         reflect, /* P */
         reflect, /* Q */
         reflect, /* R */
         reflect, /* S */
         reflect, /* T */
         reflect, /* U */
         reflect, /* V */
         reflect, /* W */
         reflect, /* X */
         reflect  /* Y */
     }},
    {FINGERPRINT_ID("MODU"),
     {
         NULL,   /* A */
         NULL,   /* B */
         NULL,   /* C */
         NULL,   /* D */
         NULL,   /* E */
         NULL,   /* F */
         NULL,   /* G */
         NULL,   /* H */
         NULL,   /* I */
         NULL,   /* J */
         NULL,   /* K */
         NULL,   /* L */
         modu_m, /* M */
         NULL,   /* N */
         NULL,   /* O */
         NULL,   /* P */
         NULL,   /* Q */
         modu_r, /* R */
         NULL,   /* S */
         NULL,   /* T */
         modu_u, /* U */
         NULL,   /* V */
         NULL,   /* W */
         NULL,   /* X */
         NULL    /* Y */
     }},
    {FINGERPRINT_ID("ROMA"),
     {
         NULL,   /* A */
         NULL,   /* B */
         roma_c, /* C */
         roma_d, /* D */
         NULL,   /* E */
         NULL,   /* F */
         NULL,   /* G */
         NULL,   /* H */
         roma_i, /* I */
         NULL,   /* J */
         NULL,   /* K */
         roma_l, /* L */
         roma_m, /* M */
         NULL,   /* N */
         NULL,   /* O */
         NULL,   /* P */
         NULL,   /* Q */
         NULL,   /* R */
         NULL,   /* S */
         NULL,   /* T */
         NULL,   /* U */
         roma_v, /* V */
         NULL,   /* W */
         roma_x, /* X */
         NULL    /* Y */
     }},
};

/**
 * ===========
 *  Main Code
 * ===========
 */

/**
 * ============================================================
 *  Begin algorithm taken from cfunge in turn from Elliot Hird
 * ============================================================
 */

static funge_cell_t wrap_frac(funge_cell_t n, funge_cell_t d) {
    return (n % d == 0 ? (n / d) : ((n + d) / d));
}

/**
 * a = value in x or y
 * r  = min in x or y
 * s  = max in x or y
 * da = delta in x or y
 */
static funge_cell_t wrap_frac_dir(funge_cell_t a, funge_cell_t r,
                                  funge_cell_t s, funge_cell_t da) {
    return (da > 0) ? wrap_frac(a - r, da) : wrap_frac(a - s, da);
}

static void wrap(Interpreter *self) {
    funge_cell_t m;
    vector_t bottom_right = funge_space_bottom_right(self->funge_space);
    vector_t top_left = funge_space_top_left(self->funge_space);
#define FUNGESPACE_FY                                                          \
    wrap_frac_dir(self->ip->pos.y, top_left.y, bottom_right.y,                 \
                  self->ip->momentum.y)
#define FUNGESPACE_FX                                                          \
    wrap_frac_dir(self->ip->pos.x, top_left.x, bottom_right.x,                 \
                  self->ip->momentum.x)
    if (self->ip->momentum.x == 0)
        m = FUNGESPACE_FY;
    else if (self->ip->momentum.y == 0)
        m = FUNGESPACE_FX;
    else {
        funge_cell_t fx = FUNGESPACE_FX;
        funge_cell_t fy = FUNGESPACE_FY;
        m = (fx < fy) ? fx : fy;
    }

    self->ip->pos.x = self->ip->pos.x - (m * self->ip->momentum.x);
    self->ip->pos.y = self->ip->pos.y - (m * self->ip->momentum.y);
#undef FUNGESPACE_FY
#undef FUNGESPACE_FX
}

/**
 * ==========================================================
 *  End algorithm taken from cfunge in turn from Elliot Hird
 * ==========================================================
 */

static void follow_momentum(Interpreter *self) {
    vector_t bottom_right = funge_space_bottom_right(self->funge_space);
    vector_t top_left = funge_space_top_left(self->funge_space);

    /* follow momentum */
    self->ip->pos.x += self->ip->momentum.x;
    self->ip->pos.y += self->ip->momentum.y;

    if ((self->ip->pos.x > bottom_right.x) || (self->ip->pos.x < top_left.x)
        || (self->ip->pos.y > bottom_right.y)
        || (self->ip->pos.y < top_left.y)) {
        wrap(self);
    }
}

static funge_cell_t next_instruction(Interpreter *self) {
    follow_momentum(self);
    return funge_space_get(self->funge_space, self->ip->pos);
}

static void reflect(Interpreter *self) {
    vector_t reflection;
    reflection.x = -self->ip->momentum.x;
    reflection.y = -self->ip->momentum.y;
    self->ip->momentum = reflection;
}

static void duplicate_top(Interpreter *self) {
    funge_cell_t top = funge_stack_pop(self->ip->stack);
    funge_stack_push(self->ip->stack, top);
    funge_stack_push(self->ip->stack, top);
}

static void update_momentum(Interpreter *self, int32_t new_x, int32_t new_y) {
    self->ip->momentum.x = new_x;
    self->ip->momentum.y = new_y;
}

static void logical_not(Interpreter *self) {
    funge_stack_push(self->ip->stack, !funge_stack_pop(self->ip->stack));
}

static void add(Interpreter *self) {
    funge_cell_t b = funge_stack_pop(self->ip->stack);
    funge_cell_t a = funge_stack_pop(self->ip->stack);
    funge_stack_push(self->ip->stack, a + b);
}

static void subtract(Interpreter *self) {
    funge_cell_t b = funge_stack_pop(self->ip->stack);
    funge_cell_t a = funge_stack_pop(self->ip->stack);
    funge_stack_push(self->ip->stack, a - b);
}

static void multiply(Interpreter *self) {
    funge_cell_t b = funge_stack_pop(self->ip->stack);
    funge_cell_t a = funge_stack_pop(self->ip->stack);
    funge_stack_push(self->ip->stack, a * b);
}

static void divide(Interpreter *self) {
    funge_cell_t b = funge_stack_pop(self->ip->stack);
    funge_cell_t a = funge_stack_pop(self->ip->stack);
    if (b == 0) {
        /* TODO - technically this should prompt user */
        funge_stack_push(self->ip->stack, 0);
    } else {
        funge_stack_push(self->ip->stack, a / b);
    }
}

static void remainder(Interpreter *self) {
    funge_cell_t b = funge_stack_pop(self->ip->stack);
    funge_cell_t a = funge_stack_pop(self->ip->stack);
    if (b == 0) {
        /* TODO - technically this should prompt user */
        funge_stack_push(self->ip->stack, 0);
    } else {
        funge_stack_push(self->ip->stack, a % b);
    }
}

static void swap(Interpreter *self) {
    funge_cell_t b = funge_stack_pop(self->ip->stack);
    funge_cell_t a = funge_stack_pop(self->ip->stack);
    funge_stack_push(self->ip->stack, b);
    funge_stack_push(self->ip->stack, a);
}

static void greater(Interpreter *self) {
    funge_cell_t b = funge_stack_pop(self->ip->stack);
    funge_cell_t a = funge_stack_pop(self->ip->stack);
    funge_stack_push(self->ip->stack, a > b);
}

static void string_mode(Interpreter *self) {
    self->ip->string_mode = true;
    self->ip->last_was_space = false;
}

static void fetch_character(Interpreter *self) {
    funge_cell_t instr = next_instruction(self);
    funge_stack_push(self->ip->stack, instr);
}

static void comment(Interpreter *self) {
    funge_cell_t instr = ' ';
    while (instr != ';') {
        instr = next_instruction(self);
    }
}

static void turn_left(Interpreter *self) {
    vector_t new_momentum;
    new_momentum.x = self->ip->momentum.y;
    new_momentum.y = -self->ip->momentum.x;
    self->ip->momentum = new_momentum;
}

static void turn_right(Interpreter *self) {
    vector_t new_momentum;
    new_momentum.x = -self->ip->momentum.y;
    new_momentum.y = self->ip->momentum.x;
    self->ip->momentum = new_momentum;
}

static void east_west_if(Interpreter *self) {
    funge_cell_t cell = funge_stack_pop(self->ip->stack);
    if (cell == 0) {
        update_momentum(self, 1, 0);
    } else {
        update_momentum(self, -1, 0);
    }
}

static void north_south_if(Interpreter *self) {
    funge_cell_t cell = funge_stack_pop(self->ip->stack);
    if (cell == 0) {
        update_momentum(self, 0, 1);
    } else {
        update_momentum(self, 0, -1);
    }
}

static void compare(Interpreter *self) {
    funge_cell_t b = funge_stack_pop(self->ip->stack);
    funge_cell_t a = funge_stack_pop(self->ip->stack);
    if (a > b) {
        turn_right(self);
    } else if (a < b) {
        turn_left(self);
    }
}

static void absolute_delta(Interpreter *self) {
    funge_cell_t y = funge_stack_pop(self->ip->stack);
    funge_cell_t x = funge_stack_pop(self->ip->stack);
    self->ip->momentum.x = x;
    self->ip->momentum.y = y;
}

/* to prevent overallocating in infinite loop */
#define MAX_OUTPUT_LEN 256

static void output_character(Interpreter *self) {
    if (string_builder_len(self->output) < MAX_OUTPUT_LEN) {
        string_builder_append_char(self->output,
                                   funge_stack_pop(self->ip->stack));
    }
}

static void output_integer(Interpreter *self) {
    char value[16];
    if (string_builder_len(self->output) < MAX_OUTPUT_LEN) {
        sprintf(value, "%i ", funge_stack_pop(self->ip->stack));
        string_builder_append(self->output, value);
    }
}

static void input_character(Interpreter *self) {
    interpreter_error(self, "input not allowed");
}

static void input_integer(Interpreter *self) {
    interpreter_error(self, "input not allowed");
}

static void go_away(Interpreter *self) {
    uint8_t val = (rand() >> 4);

    self->ip->momentum.x = val & 2 ? 0 : (val & 1 ? 1 : -1);
    self->ip->momentum.y = val & 2 ? (val & 1 ? 1 : -1) : 0;
}

static void begin_block(Interpreter *self) {
    FungeStack *temp_stack = funge_stack_create();
    funge_cell_t n = funge_stack_pop(self->ip->stack);
    funge_cell_t i;

    vector_t preserved_ip_pos = self->ip->pos;

    for (i = 0; i < n; i++) {
        funge_stack_push(temp_stack, funge_stack_pop(self->ip->stack));
    }
    for (i = 0; i > n; i--) {
        funge_stack_push(self->ip->stack, '\0');
    }
    funge_stack_push(self->ip->stack, self->ip->storage_offset.x);
    funge_stack_push(self->ip->stack, self->ip->storage_offset.y);
    stack_push(self->ip->stack_stack, self->ip->stack);
    self->ip->stack = funge_stack_create();
    while (funge_stack_size(temp_stack) != 0) {
        funge_stack_push(self->ip->stack, funge_stack_pop(temp_stack));
    }
    funge_stack_destroy(temp_stack);

    follow_momentum(self);

    self->ip->storage_offset.x = self->ip->pos.x;
    self->ip->storage_offset.y = self->ip->pos.y;

    self->ip->pos = preserved_ip_pos;
}

static void end_block(Interpreter *self) {
    FungeStack *temp_stack;
    funge_cell_t n = funge_stack_pop(self->ip->stack);
    funge_cell_t i;

    if (!stack_is_empty(self->ip->stack_stack)) {
        temp_stack = funge_stack_create();

        self->ip->storage_offset.y
            = funge_stack_pop(stack_peek(self->ip->stack_stack));
        self->ip->storage_offset.x
            = funge_stack_pop(stack_peek(self->ip->stack_stack));

        for (i = 0; i < n; i++) {
            funge_stack_push(temp_stack, funge_stack_pop(self->ip->stack));
        }
        for (i = 0; i > n; i--) {
            funge_stack_pop(stack_peek(self->ip->stack_stack));
        }
        funge_stack_destroy(self->ip->stack);
        self->ip->stack = stack_pop(self->ip->stack_stack);
        while (funge_stack_size(temp_stack) != 0) {
            funge_stack_push(self->ip->stack, funge_stack_pop(temp_stack));
        }
        funge_stack_destroy(temp_stack);

    } else {
        reflect(self);
    }
}

static void clear_stack(Interpreter *self) {
    while (funge_stack_size(self->ip->stack) != 0) {
        funge_stack_pop(self->ip->stack);
    }
}

static void put(Interpreter *self) {
    funge_cell_t y
        = funge_stack_pop(self->ip->stack) + self->ip->storage_offset.y;
    funge_cell_t x
        = funge_stack_pop(self->ip->stack) + self->ip->storage_offset.x;
    funge_cell_t n = funge_stack_pop(self->ip->stack);
    vector_t pos;
    pos.x = x;
    pos.y = y;

    funge_space_put(self->funge_space, pos, n);
}

static void get(Interpreter *self) {
    funge_cell_t y
        = funge_stack_pop(self->ip->stack) + self->ip->storage_offset.y;
    funge_cell_t x
        = funge_stack_pop(self->ip->stack) + self->ip->storage_offset.x;
    vector_t pos;
    pos.x = x;
    pos.y = y;

    funge_stack_push(self->ip->stack, funge_space_get(self->funge_space, pos));
}

static void store(Interpreter *self) {
    funge_cell_t n = funge_stack_pop(self->ip->stack);
    follow_momentum(self);
    funge_space_put(self->funge_space, self->ip->pos, n);
}

static void iterate(Interpreter *self) {
    funge_cell_t n = funge_stack_pop(self->ip->stack);
    funge_cell_t i;
    vector_t preserved_ip_pos = self->ip->pos;
    funge_cell_t instr;

    do {
        instr = next_instruction(self);
    } while (instr == ' ' || instr == ';');

    if (n > 0) {
        self->ip->pos = preserved_ip_pos;
        for (i = 0; i < n; i++) {
            execute_instruction(self, instr);
        }
    }
}

static void jump(Interpreter *self) {
    funge_cell_t n = funge_stack_pop(self->ip->stack);
    funge_cell_t i;

    if (n >= 0) {
        for (i = 0; i < n; i++) {
            follow_momentum(self);
        }
    } else {
        reflect(self);
        for (i = 0; i > n; i--) {
            follow_momentum(self);
        }
        reflect(self);
    }
}

static void stack_under_stack(Interpreter *self) {
    funge_cell_t n = funge_stack_pop(self->ip->stack);
    if (stack_is_empty(self->ip->stack_stack)) {
        reflect(self);
    } else {
        while (n > 0) {
            funge_stack_push(self->ip->stack, funge_stack_pop(stack_peek(
                                                  self->ip->stack_stack)));
            n--;
        }
        while (n < 0) {
            funge_stack_push(stack_peek(self->ip->stack_stack),
                             funge_stack_pop(self->ip->stack));
            n++;
        }
    }
}

static void sys_info(Interpreter *self) {
    funge_cell_t n = funge_stack_pop(self->ip->stack);
    time_t now_time = time(NULL);
    struct tm now = *localtime(&now_time);
    size_t i;
    size_t original_stack_size = funge_stack_size(self->ip->stack);
    funge_cell_t cell;
    vector_t bottom_right = funge_space_bottom_right(self->funge_space);
    vector_t top_left = funge_space_top_left(self->funge_space);

    /* TODO - environment variables */
    funge_stack_push(self->ip->stack, '\0');

    /* TODO - command-line arguments */
    funge_stack_push(self->ip->stack, '\0');
    funge_stack_push(self->ip->stack, '\0');

    /* size of stack-stack cells for size of each stack from TOSS to BOSS */
    for (i = stack_len(self->ip->stack_stack); i > 0; i--) {
        funge_stack_push(self->ip->stack,
                         stack_len(stack_get(self->ip->stack_stack, i - 1)));
    }
    funge_stack_push(self->ip->stack, original_stack_size);

    /* number of stacks currently in use */
    funge_stack_push(self->ip->stack, 1 + stack_len(self->ip->stack_stack));

    /* current (hour * 256 * 256) + (minute * 256) + (second) */
    funge_stack_push(self->ip->stack,
                     now.tm_hour * 0x10000 + now.tm_min * 0x100 + now.tm_sec);

    /* current ((year - 1900) * 256 * 256) + (month * 256) + (day of month)
     */
    funge_stack_push(self->ip->stack,
                     now.tm_year * 0x10000 + now.tm_mon * 0x100 + now.tm_mday);

    /* maximum x and y for which there exist non-space characters, relative to
     * minimums */
    funge_stack_push(self->ip->stack, bottom_right.x - top_left.x);
    funge_stack_push(self->ip->stack, bottom_right.y - top_left.y);

    /* minimum x and y for which there exist non-space characters */
    funge_stack_push(self->ip->stack, top_left.x);
    funge_stack_push(self->ip->stack, top_left.y);

    /* storage offset */
    funge_stack_push(self->ip->stack, self->ip->storage_offset.x);
    funge_stack_push(self->ip->stack, self->ip->storage_offset.y);

    /* IP delta */
    funge_stack_push(self->ip->stack, self->ip->momentum.x);
    funge_stack_push(self->ip->stack, self->ip->momentum.y);

    /* IP position */
    funge_stack_push(self->ip->stack, self->ip->pos.x);
    funge_stack_push(self->ip->stack, self->ip->pos.y);

    /* current IP team number */
    /* TODO - change NetFunge, BeGlad, etc is implemented */
    funge_stack_push(self->ip->stack, 0);

    /* current IP unique ID */
    funge_stack_push(self->ip->stack, self->ip->id);

    /* number of scalars per vector */
    /* 2 for Befunge */
    funge_stack_push(self->ip->stack, 2);

    /* path separator */
#if defined(_WIN32) || defined(_WIN64)
    funge_stack_push(self->ip->stack, '\\');
#else
    funge_stack_push(self->ip->stack, '/');
#endif

    /* operating paradigm */
    funge_stack_push(self->ip->stack, EQ_IMPLEMENTED ? OPERATING_PARADIGM : 0);

    /* version number */
    funge_stack_push(self->ip->stack, VERSION_NUMBER);

    /* handprint */
    funge_stack_push(self->ip->stack, HANDPRINT);

    /* funge cell size in bytes */
    funge_stack_push(self->ip->stack, sizeof(funge_cell_t));

    /* flags */
    funge_stack_push(self->ip->stack, UNBUFFERED_IO << 4 | EQ_IMPLEMENTED << 3
                                          | O_IMPLEMENTED << 2
                                          | I_IMPLEMENTED << 1 | T_IMPLEMENTED);
    if (n > 0) {
        cell = funge_stack_get(self->ip->stack, (size_t)(n - 1));
        while (funge_stack_size(self->ip->stack) != original_stack_size) {
            funge_stack_pop(self->ip->stack);
        }
        funge_stack_push(self->ip->stack, cell);
    }
}

typedef struct {
    fingerprint_t unwrap;
    bool is_some;
} option_fingerprint_t;

static option_fingerprint_t find_fingerprint(funge_cell_t id) {
    size_t i;
    option_fingerprint_t fingerprint;
    fingerprint.is_some = false;
    for (i = 0; i < sizeof(FINGERPRINTS) / sizeof(FINGERPRINTS[0])
                && !fingerprint.is_some;
         i++) {
        if (FINGERPRINTS[i].id == id) {
            fingerprint.unwrap = FINGERPRINTS[i];
            fingerprint.is_some = true;
        }
    }
    return fingerprint;
}

static void load_semantics(Interpreter *self) {
    funge_cell_t n = funge_stack_pop(self->ip->stack);
    funge_cell_t id = 0;
    option_fingerprint_t fingerprint;
    void *func;
    size_t i;
    while (n > 0) {
        id *= 0x100;
        id += funge_stack_pop(self->ip->stack);
        n--;
    }
    fingerprint = find_fingerprint(id);
    if (fingerprint.is_some) {
        funge_stack_push(self->ip->stack, id);
        for (i = 0; i < 26; i++) {
            if (fingerprint.unwrap.funcs[i] != NULL) {
                /* hacky trick to shut up compiler about converting function
                 * pointers */
                /* TODO - is this actually dangerous? */
                memcpy(&func, &fingerprint.unwrap.funcs[i], sizeof(func));
                stack_push(self->ip->semantics[i], func);
            }
        }
        funge_stack_push(self->ip->stack, 1);
    } else {
        reflect(self);
    }
}

static void unload_semantics(Interpreter *self) {
    funge_cell_t n = funge_stack_pop(self->ip->stack);
    funge_cell_t id = 0;
    option_fingerprint_t fingerprint;
    size_t i;
    while (n > 0) {
        id *= 0x100;
        id += funge_stack_pop(self->ip->stack);
        n--;
    }
    fingerprint = find_fingerprint(id);
    if (fingerprint.is_some) {
        for (i = 0; i < 26; i++) {
            if (fingerprint.unwrap.funcs[i] != NULL) {
                if (!stack_is_empty(self->ip->semantics[i])) {
                    stack_pop(self->ip->semantics[i]);
                }
            }
        }
    } else {
        reflect(self);
    }
}

static void end_ip(Interpreter *self) {
    instruction_pointer_destroy(self->ip);
    if (queue_is_empty(self->other_ips)) {
        self->ip = NULL;
    } else {
        self->ip = queue_dequeue(self->other_ips);
    }
}

static void quit(Interpreter *self) {
    self->return_code = funge_stack_pop(self->ip->stack);
    while (!queue_is_empty(self->other_ips)) {
        instruction_pointer_destroy(queue_dequeue(self->other_ips));
    }
    instruction_pointer_destroy(self->ip);
    self->ip = NULL;
}

static void split(Interpreter *self) {
    InstructionPointer *new = instruction_pointer_clone(self->ip);
    new->momentum.x = -self->ip->momentum.x;
    new->momentum.y = -self->ip->momentum.y;
    queue_enqueue(self->other_ips, new);
}

static void execute(Interpreter *self) {
    Keystroke *keystroke = keystroke_create();
    funge_cell_t cell = funge_stack_pop(self->ip->stack);
    while (cell != 0) {
        keystroke_append_key(keystroke, cell);
        cell = funge_stack_pop(self->ip->stack);
    }
    if (!editor_execute_keystroke(self->editor, keystroke)) {
        interpreter_error(self, "editor terminated");
    }
    keystroke_destroy(keystroke);
}

static void execute_string_mode_instruction(Interpreter *self,
                                            funge_cell_t instr) {
    if (instr == ' ') {
        if (self->ip->last_was_space) {
            while (instr == ' ') {
                instr = next_instruction(self);
            }
            self->ip->last_was_space = false;
        } else {
            self->ip->last_was_space = true;
        }
    } else {
        self->ip->last_was_space = false;
    }
    if (instr == '"') {
        self->ip->string_mode = false;
    } else {
        funge_stack_push(self->ip->stack, instr);
    }
}

static void execute_instruction(Interpreter *self, funge_cell_t instr) {
    /** ---- TODO ----
     * i - input file
     * o - output file
     *
     * Change #defines at start of this file if above are implemented
     */
    void (*func)(Interpreter *self);
    void *void_func;

    if (isdigit(instr)) {
        funge_stack_push(self->ip->stack, instr - '0');
    } else if ('a' <= instr && instr <= 'f') {
        funge_stack_push(self->ip->stack, 10 + instr - 'a');
    } else if ('A' <= instr && instr <= 'Z') {
        if (self->ip->semantics[instr - 'A']) {
            if (stack_is_empty(self->ip->semantics[instr - 'A'])) {
                reflect(self);
            } else {
                /* hacky trick to shut up compiler about converting function
                 * pointers */
                /* TODO - is this actually dangerous? */
                void_func = stack_peek(self->ip->semantics[instr - 'A']);
                memcpy(&func, &void_func, sizeof(func));
                func(self);
            }
        }
    } else {
        switch (instr) {
        case ' ':
        case 'z': break;
        case 'n': clear_stack(self); break;
        case '>': update_momentum(self, 1, 0); break;
        case 'v': update_momentum(self, 0, 1); break;
        case '<': update_momentum(self, -1, 0); break;
        case '^': update_momentum(self, 0, -1); break;
        case '[': turn_left(self); break;
        case ']': turn_right(self); break;
        case '?': go_away(self); break;
        case '#': follow_momentum(self); break;
        case 'j': jump(self); break;
        case 'k': iterate(self); break;
        case '"': string_mode(self); break;
        case '\'': fetch_character(self); break;
        case ';': comment(self); break;
        case '$': funge_stack_pop(self->ip->stack); break;
        case '!': logical_not(self); break;
        case '+': add(self); break;
        case '-': subtract(self); break;
        case '*': multiply(self); break;
        case '/': divide(self); break;
        case '%': remainder(self); break;
        case '`': greater(self); break;
        case ':': duplicate_top(self); break;
        case '_': east_west_if(self); break;
        case '|': north_south_if(self); break;
        case 'g': get(self); break;
        case 'p': put(self); break;
        case 's': store(self); break;
        case '\\': swap(self); break;
        case 'w': compare(self); break;
        case 'x': absolute_delta(self); break;
        case ',': output_character(self); break;
        case '.': output_integer(self); break;
        case '~': input_character(self); break;
        case '&': input_integer(self); break;
        case '{': begin_block(self); break;
        case '}': end_block(self); break;
        case 'u': stack_under_stack(self); break;
        case 'y': sys_info(self); break;
        case '(': load_semantics(self); break;
        case ')': unload_semantics(self); break;
        case '@': end_ip(self); break;
        case 'q': quit(self); break;
        case 't': split(self); break;
        case '=': execute(self); break;
        case 'r':
        default: reflect(self); break;
        }
    }
}
static void next_ip(Interpreter *self) {
    if (!queue_is_empty(self->other_ips)) {
        queue_enqueue(self->other_ips, self->ip);
        self->ip = queue_dequeue(self->other_ips);
    }
}

int interpreter_run(Interpreter *self) {
    funge_cell_t instr = funge_space_get(self->funge_space, self->ip->pos);
    while (!self->is_poisoned) {
        if (self->ip->string_mode) {
            execute_string_mode_instruction(self, instr);
            next_ip(self);
        } else {
            execute_instruction(self, instr);
            if (!self->ip) break;
            if (instr != ' ' && instr != ';') {
                /* spaces and comments are tickless */
                next_ip(self);
            }
        }
        instr = next_instruction(self);
    }
    return self->return_code;
}

void interpreter_spawn_macro_ip(Interpreter *self, vector_t pos) {
    size_t i;
    InstructionPointer *macro_ip = instruction_pointer_create();
    option_fingerprint_t fingerprint = find_fingerprint(FINGERPRINT_ID("BFDT"));
    void *func;
    macro_ip->pos = pos;
    if (fingerprint.is_some) {
        for (i = 0; i < 26; i++) {
            if (fingerprint.unwrap.funcs[i] != NULL) {
                /* hacky trick to shut up compiler about converting function
                 * pointers */
                /* TODO - is this actually dangerous? */
                memcpy(&func, &fingerprint.unwrap.funcs[i], sizeof(func));
                stack_push(self->ip->semantics[i], func);
            }
        }
    } else {
        report_logic_error(FILENAME
                           ": expected BFDT fingerprint to exist for macro");
    }
    if (!self->ip) {
        self->ip = macro_ip;
    } else {
        queue_enqueue(self->other_ips, macro_ip);
    }
}

#define CHUNK_SIZE 128

static InstructionPointer *instruction_pointer_clone(InstructionPointer *self) {
    InstructionPointer *new = calloc(1, sizeof(InstructionPointer));
    FungeStack *new_funge_stack, *original_funge_stack;
    size_t i, j;
    if (!new) {
        report_system_error(FILENAME ": memory allocation failure");
        goto instruction_pointer_clone_fail;
    }

    new->pos.x = self->pos.x;
    new->pos.y = self->pos.y;
    new->momentum.x = self->momentum.x;
    new->momentum.y = self->momentum.y;

    new->stack = funge_stack_clone(self->stack);
    new->stack_stack = stack_create((void (*)(void *))funge_stack_destroy);
    for (i = 0; i < stack_len(self->stack_stack); i++) {
        original_funge_stack = stack_get(self->stack_stack,
                                         stack_len(self->stack_stack) - i - 1);
        new_funge_stack = funge_stack_clone(original_funge_stack);
        stack_push(new->stack_stack, new_funge_stack);
    }

    for (i = 0; i < 26; i++) {
        new->semantics[i] = stack_create(NULL);
        if (!new->semantics[i]) goto instruction_pointer_clone_fail;
        for (j = 0; j < stack_len(self->semantics[i]); j++) {
            stack_push(new->semantics[i],
                       stack_get(self->semantics[i],
                                 stack_len(self->semantics[i]) - j - 1));
        }
    }

    new->storage_offset.x = 0;
    new->storage_offset.y = 0;

    new->id = g_next_ip_id++;

    return new;
instruction_pointer_clone_fail:
    instruction_pointer_destroy(new);
    return NULL;
}

static InstructionPointer *instruction_pointer_create(void) {
    InstructionPointer *self = calloc(1, sizeof(InstructionPointer));
    size_t i;
    if (!self) {
        report_system_error(FILENAME ": memory allocation failure");
        goto instruction_pointer_create_fail;
    }

    self->pos.x = 0;
    self->pos.y = 0;
    self->momentum.x = 1;
    self->momentum.y = 0;

    self->stack = funge_stack_create();
    if (!self->stack) goto instruction_pointer_create_fail;
    self->stack_stack = stack_create((void (*)(void *))funge_stack_destroy);
    if (!self->stack_stack) goto instruction_pointer_create_fail;

    self->storage_offset.x = 0;
    self->storage_offset.y = 0;

    for (i = 0; i < 26; i++) {
        self->semantics[i] = stack_create(NULL);
        if (!self->semantics[i]) goto instruction_pointer_create_fail;
    }

    self->id = g_next_ip_id++;

    return self;
instruction_pointer_create_fail:
    instruction_pointer_destroy(self);
    return NULL;
}

static void instruction_pointer_destroy(InstructionPointer *self) {
    size_t i;
    if (self) {
        funge_stack_destroy(self->stack);
        stack_destroy(self->stack_stack);
        for (i = 0; i < 26; i++) {
            stack_destroy(self->semantics[i]);
        }
        free(self);
    }
}

volatile bool *interpreter_is_poisoned_ref(Interpreter *self) {
    return &self->is_poisoned;
}

char *interpreter_get_output(Interpreter *self) {
    return string_builder_to_string(self->output);
}

Interpreter *interpreter_create(const char *fname, Editor *editor) {
    Interpreter *self = calloc(1, sizeof(Interpreter));
    if (!self) {
        report_system_error(FILENAME ": memory allocation failure");
        goto interpreter_create_fail;
    }

    self->editor = editor;
    self->is_poisoned = false;

    self->funge_space = funge_space_create(fname);
    if (!self->funge_space) goto interpreter_create_fail;

    self->output = string_builder_create();
    if (!self->output) goto interpreter_create_fail;

    self->other_ips
        = queue_create((void (*)(void *))instruction_pointer_destroy);
    if (!self->other_ips) goto interpreter_create_fail;

    self->ip = instruction_pointer_create();
    if (!self->ip) goto interpreter_create_fail;

    self->return_code = 0;

    return self;
interpreter_create_fail:
    interpreter_destroy(self);
    return NULL;
}

void interpreter_destroy(Interpreter *self) {
    if (self) {
        funge_space_destroy(self->funge_space);
        queue_destroy(self->other_ips);
        instruction_pointer_destroy(self->ip);
        string_builder_destroy(self->output);
        free(self);
    }
}
