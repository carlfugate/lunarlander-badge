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

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_init_values);
    RUN_TEST(test_phase_to_playing);
    RUN_TEST(test_no_tick_after_landed);
    RUN_TEST(test_no_tick_after_crashed);
    return UNITY_END();
}
