#include <unity.h>
#include "Game/LunarState.h"

static GameState gs;

void setUp(void) { game_init(gs, 0, 42); }
void tearDown(void) {}

void test_crash_zero(void) {
    gs.lander.crashed = true;
    TEST_ASSERT_EQUAL_UINT16(0, game_calc_score(gs));
}

void test_perfect_easy(void) {
    gs.lander.landed = true;
    gs.lander.fuel = LN_INITIAL_FUEL;
    gs.elapsed_ms = 15000;
    uint16_t score = game_calc_score(gs);
    uint16_t expected = (uint16_t)((LN_SCORE_BASE + LN_SCORE_FUEL_MAX + LN_SCORE_TIME_MAX) * LN_MULT_EASY / 10);
    TEST_ASSERT_EQUAL_UINT16(expected, score);
}

void test_perfect_hard(void) {
    game_init(gs, 2, 42);
    gs.lander.landed = true;
    gs.lander.fuel = LN_INITIAL_FUEL;
    gs.elapsed_ms = 15000;
    uint16_t score = game_calc_score(gs);
    uint16_t expected = (uint16_t)((LN_SCORE_BASE + LN_SCORE_FUEL_MAX + LN_SCORE_TIME_MAX) * LN_MULT_HARD / 10);
    TEST_ASSERT_EQUAL_UINT16(expected, score);
}

void test_no_fuel_bonus(void) {
    gs.lander.landed = true;
    gs.lander.fuel = 0.0f;
    gs.elapsed_ms = 15000;
    uint16_t score = game_calc_score(gs);
    uint16_t expected = (uint16_t)((LN_SCORE_BASE + 0 + LN_SCORE_TIME_MAX) * LN_MULT_EASY / 10);
    TEST_ASSERT_EQUAL_UINT16(expected, score);
}

void test_slow_time(void) {
    gs.lander.landed = true;
    gs.lander.fuel = LN_INITIAL_FUEL;
    gs.elapsed_ms = 120000;
    uint16_t score = game_calc_score(gs);
    uint16_t expected = (uint16_t)((LN_SCORE_BASE + LN_SCORE_FUEL_MAX + 0) * LN_MULT_EASY / 10);
    TEST_ASSERT_EQUAL_UINT16(expected, score);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_crash_zero);
    RUN_TEST(test_perfect_easy);
    RUN_TEST(test_perfect_hard);
    RUN_TEST(test_no_fuel_bonus);
    RUN_TEST(test_slow_time);
    return UNITY_END();
}
