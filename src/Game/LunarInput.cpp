#include "Game/LunarInput.h"

#ifndef NATIVE_TEST

#include <Arduino.h>
#include <lvgl.h>
#include "pins.h"
#include "Game/LunarConfig.h"

void input_init() {
    pinMode(BUTTON_ENTER_PIN, INPUT_PULLUP);
    pinMode(BUTTON_BACK_PIN, INPUT_PULLUP);
}

InputState input_read() {
    InputState s = {false, 0, false};

    // Touch input via LVGL pointer indev
    lv_indev_t *indev = lv_indev_get_next(NULL);
    while (indev) {
        if (lv_indev_get_type(indev) == LV_INDEV_TYPE_POINTER) {
            lv_point_t point;
            lv_indev_get_point(indev, &point);
            lv_indev_state_t state = lv_indev_get_state(indev);
            if (state == LV_INDEV_STATE_PRESSED) {
                if (point.x < LN_SCREEN_W / 2) {
                    s.thrust = true;
                } else if (point.y < LN_SCREEN_H / 2) {
                    s.rotate_dir = -1;
                } else {
                    s.rotate_dir = 1;
                }
            }
            break;
        }
        indev = lv_indev_get_next(indev);
    }

    // Physical buttons (active LOW)
    if (digitalRead(BUTTON_ENTER_PIN) == LOW) s.thrust = true;
    if (digitalRead(BUTTON_BACK_PIN) == LOW) s.back = true;

    return s;
}

#else

void input_init() {}

InputState input_read() {
    return {false, 0, false};
}

#endif
