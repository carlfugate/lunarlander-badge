#include <unity.h>
#include "Game/LunarTerrain.h"
#include "Game/LunarPhysics.h"

static Terrain t;
static Lander l;

void setUp(void) {
    terrain_generate(t, 0, 42);
    lander_init(l);
}
void tearDown(void) {}

void test_above_terrain(void) {
    l.y = 50.0f;
    terrain_check_collision(t, l);
    TEST_ASSERT_FALSE(l.crashed);
    TEST_ASSERT_FALSE(l.landed);
}

void test_safe_landing(void) {
    float mid = (t.zone_x1 + t.zone_x2) / 2.0f;
    l.x = mid;
    l.y = terrain_height_at(t, mid);
    l.vx = 0.0f;
    l.vy = 1.0f;
    l.rotation = 0.0f;
    terrain_check_collision(t, l);
    TEST_ASSERT_TRUE(l.landed);
    TEST_ASSERT_FALSE(l.crashed);
}

void test_too_fast(void) {
    float mid = (t.zone_x1 + t.zone_x2) / 2.0f;
    l.x = mid;
    l.y = terrain_height_at(t, mid);
    l.vy = 10.0f;
    l.rotation = 0.0f;
    terrain_check_collision(t, l);
    TEST_ASSERT_TRUE(l.crashed);
}

void test_bad_angle(void) {
    float mid = (t.zone_x1 + t.zone_x2) / 2.0f;
    l.x = mid;
    l.y = terrain_height_at(t, mid);
    l.vy = 1.0f;
    l.rotation = 1.0f;
    terrain_check_collision(t, l);
    TEST_ASSERT_TRUE(l.crashed);
}

void test_off_zone(void) {
    l.x = 50.0f;
    l.y = terrain_height_at(t, 50.0f);
    l.vy = 1.0f;
    l.rotation = 0.0f;
    terrain_check_collision(t, l);
    TEST_ASSERT_TRUE(l.crashed);
}

void test_out_of_bounds(void) {
    l.x = -10.0f;
    terrain_check_collision(t, l);
    TEST_ASSERT_TRUE(l.crashed);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_above_terrain);
    RUN_TEST(test_safe_landing);
    RUN_TEST(test_too_fast);
    RUN_TEST(test_bad_angle);
    RUN_TEST(test_off_zone);
    RUN_TEST(test_out_of_bounds);
    return UNITY_END();
}
