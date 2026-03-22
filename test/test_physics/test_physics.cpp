#include <unity.h>
#include "Game/LunarPhysics.h"
#include <math.h>

static Lander l;

void setUp(void) { lander_init(l); }
void tearDown(void) {}

void test_init_position(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.01f, LN_START_X, l.x);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, LN_START_Y, l.y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, l.vx);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, l.vy);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, LN_INITIAL_FUEL, l.fuel);
    TEST_ASSERT_FALSE(l.crashed);
    TEST_ASSERT_FALSE(l.landed);
}

void test_gravity_only(void) {
    lander_update(l, false, 0, LN_PHYSICS_DT);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, l.vx);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, LN_GRAVITY * LN_PHYSICS_DT, l.vy);
}

void test_thrust_up(void) {
    lander_update(l, true, 0, LN_PHYSICS_DT);
    float expected_vy = -LN_THRUST_POWER * LN_PHYSICS_DT + LN_GRAVITY * LN_PHYSICS_DT;
    TEST_ASSERT_FLOAT_WITHIN(0.001f, expected_vy, l.vy);
    TEST_ASSERT_TRUE(l.thrusting);
}

void test_thrust_angled(void) {
    l.rotation = M_PI / 4.0f;
    lander_update(l, true, 0, LN_PHYSICS_DT);
    float expected_vx = sinf(M_PI / 4.0f) * LN_THRUST_POWER * LN_PHYSICS_DT;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, expected_vx, l.vx);
}

void test_rotation_rate(void) {
    lander_update(l, false, 1, LN_PHYSICS_DT);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, LN_ROTATION_SPEED * LN_PHYSICS_DT, l.rotation);
}

void test_rotation_clamp(void) {
    l.rotation = M_PI - 0.01f;
    lander_update(l, false, 1, 1.0f);
    TEST_ASSERT_TRUE(l.rotation >= -M_PI && l.rotation <= M_PI);
}

void test_fuel_depletion(void) {
    lander_update(l, true, 0, LN_PHYSICS_DT);
    float expected = LN_INITIAL_FUEL - LN_FUEL_CONSUMPTION * LN_PHYSICS_DT;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, expected, l.fuel);
}

void test_no_thrust_empty(void) {
    l.fuel = 0.0f;
    float vy_before = l.vy;
    lander_update(l, true, 0, LN_PHYSICS_DT);
    TEST_ASSERT_FALSE(l.thrusting);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, vy_before + LN_GRAVITY * LN_PHYSICS_DT, l.vy);
}

void test_position_update(void) {
    l.vx = 10.0f;
    l.vy = 5.0f;
    float x0 = l.x, y0 = l.y;
    lander_update(l, false, 0, LN_PHYSICS_DT);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, x0 + 10.0f * LN_PHYSICS_DT, l.x);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_init_position);
    RUN_TEST(test_gravity_only);
    RUN_TEST(test_thrust_up);
    RUN_TEST(test_thrust_angled);
    RUN_TEST(test_rotation_rate);
    RUN_TEST(test_rotation_clamp);
    RUN_TEST(test_fuel_depletion);
    RUN_TEST(test_no_thrust_empty);
    RUN_TEST(test_position_update);
    return UNITY_END();
}
