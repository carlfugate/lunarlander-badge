#include "Hardware/RotaryEncoder_Module.h"
#include <ESP32RotaryEncoder.h>
#include "pins.h"

// Rotary encoder instance and state flags
typedef void (*EncoderCallback)(long);
typedef void (*ButtonCallback)(unsigned long);

static RotaryEncoder rotaryEncoder(ENCODER_PIN_A, ENCODER_PIN_B, ENCODER_BUTTON_PIN);
volatile bool rotaryEncoderTurnedLeftFlag = false;
volatile bool rotaryEncoderTurnedRightFlag = false;
volatile bool rotaryEncoderButtonPushedFlag = false;

static EncoderCallback userEncoderCallback = nullptr;
static ButtonCallback userButtonCallback = nullptr;

void RotaryEncoder_Module_Init() {
    rotaryEncoder.setEncoderType(EncoderType::FLOATING);
    rotaryEncoder.setBoundaries(-1, 1, false);
    rotaryEncoder.onTurned([](long value) {
        if (userEncoderCallback) userEncoderCallback(value);
        switch (value) {
            case 1:
                rotaryEncoderTurnedRightFlag = true;
                break;
            case -1:
                rotaryEncoderTurnedLeftFlag = true;
                break;
        }
        rotaryEncoder.setEncoderValue(0);
    });
    rotaryEncoder.onPressed([](unsigned long v) {
        rotaryEncoderButtonPushedFlag = true;
        if (userButtonCallback) userButtonCallback(v);
    });
    rotaryEncoder.begin();
}

void RotaryEncoder_Module_SetEncoderCallback(EncoderCallback cb) {
    userEncoderCallback = cb;
}

void RotaryEncoder_Module_SetButtonCallback(ButtonCallback cb) {
    userButtonCallback = cb;
}

bool RotaryEncoder_Module_TurnedLeft() {
    bool flag = rotaryEncoderTurnedLeftFlag;
    rotaryEncoderTurnedLeftFlag = false;
    return flag;
}

bool RotaryEncoder_Module_TurnedRight() {
    bool flag = rotaryEncoderTurnedRightFlag;
    rotaryEncoderTurnedRightFlag = false;
    return flag;
}

bool RotaryEncoder_Module_ButtonPushed() {
    bool flag = rotaryEncoderButtonPushedFlag;
    rotaryEncoderButtonPushedFlag = false;
    return flag;
}
