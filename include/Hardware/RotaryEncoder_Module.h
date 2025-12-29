#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*EncoderCallback)(long);
typedef void (*ButtonCallback)(unsigned long);

void RotaryEncoder_Module_Init();
void RotaryEncoder_Module_SetEncoderCallback(EncoderCallback cb);
void RotaryEncoder_Module_SetButtonCallback(ButtonCallback cb);
bool RotaryEncoder_Module_TurnedLeft();
bool RotaryEncoder_Module_TurnedRight();
bool RotaryEncoder_Module_ButtonPushed();

#ifdef __cplusplus
}
#endif
