#ifndef LUNAR_AUDIO_H
#define LUNAR_AUDIO_H

#include <stdint.h>

void audio_set_mute(bool muted);
bool audio_is_muted();
void audio_click();
void audio_click_back();

void audio_thrust_start();
void audio_thrust_stop();
void audio_landed();
void audio_crashed();
void audio_low_fuel();
void audio_altitude_warning(float altitude);
void audio_boot();
void audio_countdown(int seconds_left);

void leds_thrust();
void leds_landed();
void leds_crashed();
void leds_idle();
void leds_fuel_gauge(float fuel, float max_fuel);
void leds_team_color(uint8_t player_index);

void audio_achievement();
void leds_achievement();

#endif
