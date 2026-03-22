#include <unity.h>
#include "Game/LunarTerrain.h"

static Terrain t;

void setUp(void) { terrain_generate(t, 0, 42); }
void tearDown(void) {}

void test_point_count(void) {
    TEST_ASSERT_TRUE(t.num_points > 0);
    TEST_ASSERT_TRUE(t.num_points <= LN_MAX_TERRAIN_POINTS);
    TEST_ASSERT_EQUAL_INT16(0, t.points[0][0]);
    TEST_ASSERT_TRUE(t.points[t.num_points - 1][0] >= LN_WORLD_W - LN_DIFF_EASY_STEP);
}

void test_zone_exists(void) {
    TEST_ASSERT_TRUE(t.zone_x1 >= 400);
    TEST_ASSERT_TRUE(t.zone_x2 <= 700 + LN_DIFF_EASY_ZONE);
    TEST_ASSERT_TRUE(t.zone_x2 > t.zone_x1);
}

void test_zone_width_easy(void) {
    TEST_ASSERT_EQUAL_INT16(LN_DIFF_EASY_ZONE, t.zone_x2 - t.zone_x1);
}

void test_zone_width_hard(void) {
    Terrain th;
    terrain_generate(th, 2, 42);
    TEST_ASSERT_EQUAL_INT16(LN_DIFF_HARD_ZONE, th.zone_x2 - th.zone_x1);
}

void test_height_interpolation(void) {
    float h0 = terrain_height_at(t, (float)t.points[0][0]);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, (float)t.points[0][1], h0);
    float mid_x = (t.points[0][0] + t.points[1][0]) / 2.0f;
    float mid_h = terrain_height_at(t, mid_x);
    float expected = (t.points[0][1] + t.points[1][1]) / 2.0f;
    TEST_ASSERT_FLOAT_WITHIN(1.0f, expected, mid_h);
}

void test_zone_detection(void) {
    float mid = (t.zone_x1 + t.zone_x2) / 2.0f;
    TEST_ASSERT_TRUE(terrain_is_landing_zone(t, mid));
    TEST_ASSERT_FALSE(terrain_is_landing_zone(t, 0.0f));
}

void test_y_bounds(void) {
    for (uint16_t i = 0; i < t.num_points; i++) {
        TEST_ASSERT_TRUE(t.points[i][1] >= 0);
        TEST_ASSERT_TRUE(t.points[i][1] <= LN_WORLD_H);
    }
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_point_count);
    RUN_TEST(test_zone_exists);
    RUN_TEST(test_zone_width_easy);
    RUN_TEST(test_zone_width_hard);
    RUN_TEST(test_height_interpolation);
    RUN_TEST(test_zone_detection);
    RUN_TEST(test_y_bounds);
    return UNITY_END();
}
