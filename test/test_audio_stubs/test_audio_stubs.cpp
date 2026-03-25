#include <unity.h>
#include "Game/LunarAudio.h"

void setUp(void) { audio_set_mute(false); }
void tearDown(void) {}

void test_mute_default_off(void) {
    TEST_ASSERT_FALSE(audio_is_muted());
}

void test_mute_toggle(void) {
    audio_set_mute(true);
    TEST_ASSERT_TRUE(audio_is_muted());
    audio_set_mute(false);
    TEST_ASSERT_FALSE(audio_is_muted());
}

void test_audio_functions_dont_crash(void) {
    audio_thrust_start();
    audio_thrust_stop();
    audio_landed();
    audio_crashed();
    audio_low_fuel();
    audio_click();
    audio_click_back();
    audio_altitude_warning(50.0f);
    audio_countdown(5);
    audio_achievement();
    TEST_ASSERT_TRUE(true);
}

void test_led_functions_dont_crash(void) {
    leds_thrust();
    leds_landed();
    leds_crashed();
    leds_idle();
    leds_fuel_gauge(50.0f, 100.0f);
    leds_team_color(0);
    leds_achievement();
    TEST_ASSERT_TRUE(true);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_mute_default_off);
    RUN_TEST(test_mute_toggle);
    RUN_TEST(test_audio_functions_dont_crash);
    RUN_TEST(test_led_functions_dont_crash);
    return UNITY_END();
}
