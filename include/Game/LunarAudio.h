#ifndef LUNAR_AUDIO_H
#define LUNAR_AUDIO_H

void audio_thrust_start();
void audio_thrust_stop();
void audio_landed();
void audio_crashed();
void audio_low_fuel();

void leds_thrust();
void leds_landed();
void leds_crashed();
void leds_idle();

#endif
