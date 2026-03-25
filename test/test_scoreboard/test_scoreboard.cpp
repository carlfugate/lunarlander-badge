#include <unity.h>
#include "Game/LunarScoreboard.h"

static Scoreboard sb;

void setUp(void) { sb.count = 0; }
void tearDown(void) {}

void test_add_first_score(void) {
    int rank = scoreboard_add(sb, 1000, 0);
    TEST_ASSERT_EQUAL(0, rank);
    TEST_ASSERT_EQUAL(1, sb.count);
    TEST_ASSERT_EQUAL_UINT16(1000, sb.entries[0].score);
}

void test_scores_sorted_descending(void) {
    scoreboard_add(sb, 500, 0);
    scoreboard_add(sb, 1000, 1);
    scoreboard_add(sb, 750, 2);
    TEST_ASSERT_EQUAL_UINT16(1000, sb.entries[0].score);
    TEST_ASSERT_EQUAL_UINT16(750, sb.entries[1].score);
    TEST_ASSERT_EQUAL_UINT16(500, sb.entries[2].score);
}

void test_max_five_entries(void) {
    for (int i = 0; i < 7; i++) scoreboard_add(sb, (i + 1) * 100, 0);
    TEST_ASSERT_EQUAL(5, sb.count);
    TEST_ASSERT_EQUAL_UINT16(700, sb.entries[0].score);
}

void test_low_score_rejected(void) {
    for (int i = 0; i < 5; i++) scoreboard_add(sb, 1000 + i * 100, 0);
    int rank = scoreboard_add(sb, 50, 0);
    TEST_ASSERT_EQUAL(-1, rank);
    TEST_ASSERT_EQUAL(5, sb.count);
}

void test_difficulty_preserved(void) {
    scoreboard_add(sb, 1000, 2);
    TEST_ASSERT_EQUAL_UINT8(2, sb.entries[0].difficulty);
}

void test_zero_score_rejected(void) {
    int rank = scoreboard_add(sb, 0, 0);
    TEST_ASSERT_EQUAL(-1, rank);
    TEST_ASSERT_EQUAL(0, sb.count);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_add_first_score);
    RUN_TEST(test_scores_sorted_descending);
    RUN_TEST(test_max_five_entries);
    RUN_TEST(test_low_score_rejected);
    RUN_TEST(test_difficulty_preserved);
    RUN_TEST(test_zero_score_rejected);
    return UNITY_END();
}
