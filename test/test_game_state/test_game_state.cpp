#include <unity.h>
#include "Game/LunarState.h"

static GameState gs;

void setUp(void) { game_init(gs, 0, 42); }
void tearDown(void) {}

void test_init_values(void) {
    TEST_ASSERT_EQUAL(PHASE_MENU, gs.phase);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, LN_START_X, gs.lander.x);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, LN_START_Y, gs.lander.y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, LN_INITIAL_FUEL, gs.lander.fuel);
    TEST_ASSERT_EQUAL_UINT8(0, gs.difficulty);
}

void test_phase_to_playing(void) {
    game_tick(gs, true, 0, 1000);
    TEST_ASSERT_EQUAL(PHASE_PLAYING, gs.phase);
}

void test_no_tick_after_landed(void) {
    gs.phase = PHASE_LANDED;
    float x_before = gs.lander.x;
    game_tick(gs, true, 1, 5000);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, x_before, gs.lander.x);
}

void test_no_tick_after_crashed(void) {
    gs.phase = PHASE_CRASHED;
    float x_before = gs.lander.x;
    game_tick(gs, true, 1, 5000);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, x_before, gs.lander.x);
}

void test_difficulty_stored(void) {
    game_init(gs, 2, 42);
    TEST_ASSERT_EQUAL_UINT8(2, gs.difficulty);
}

void test_phase_waiting_exists(void) {
    gs.phase = PHASE_WAITING;
    TEST_ASSERT_EQUAL(PHASE_WAITING, gs.phase);
}

void test_game_modes_exist(void) {
    gs.mode = MODE_OFFLINE;
    TEST_ASSERT_EQUAL(MODE_OFFLINE, gs.mode);
    gs.mode = MODE_ONLINE_SOLO;
    TEST_ASSERT_EQUAL(MODE_ONLINE_SOLO, gs.mode);
    gs.mode = MODE_ONLINE_MULTI;
    TEST_ASSERT_EQUAL(MODE_ONLINE_MULTI, gs.mode);
    gs.mode = MODE_SPECTATE;
    TEST_ASSERT_EQUAL(MODE_SPECTATE, gs.mode);
}

void test_elapsed_time_advances(void) {
    gs.phase = PHASE_PLAYING;
    game_tick(gs, false, 0, 1100);
    TEST_ASSERT_GREATER_THAN(0, gs.elapsed_ms);
}

void test_fuel_decreases_with_thrust(void) {
    gs.phase = PHASE_PLAYING;
    float fuel_before = gs.lander.fuel;
    game_tick(gs, true, 0, 1016);
    TEST_ASSERT_LESS_THAN_FLOAT(fuel_before, gs.lander.fuel);
}

void test_no_fuel_no_thrust(void) {
    gs.phase = PHASE_PLAYING;
    gs.lander.fuel = 0;
    float vy_before = gs.lander.vy;
    game_tick(gs, true, 0, 1016);
    TEST_ASSERT_GREATER_THAN_FLOAT(vy_before, gs.lander.vy);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_init_values);
    RUN_TEST(test_phase_to_playing);
    RUN_TEST(test_no_tick_after_landed);
    RUN_TEST(test_no_tick_after_crashed);
    RUN_TEST(test_difficulty_stored);
    RUN_TEST(test_phase_waiting_exists);
    RUN_TEST(test_game_modes_exist);
    RUN_TEST(test_elapsed_time_advances);
    RUN_TEST(test_fuel_decreases_with_thrust);
    RUN_TEST(test_no_fuel_no_thrust);
    return UNITY_END();
}
