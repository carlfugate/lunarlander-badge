#ifndef LUNAR_INPUT_H
#define LUNAR_INPUT_H

#include <stdbool.h>

struct InputState {
    bool thrust;
    int rotate_dir;  // -1, 0, +1
    bool back;
};

void input_init();
InputState input_read();

#endif
