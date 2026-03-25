#ifndef QA_ACHIEVEMENTS_H
#define QA_ACHIEVEMENTS_H

#include <stdint.h>

#define ACH_FIRST_LANDING   0
#define ACH_EAGLE_SCOUT     1  // score > 5000
#define ACH_APOLLO_13       2  // land with < 1% fuel
#define ACH_MARATHON        3  // 10 games played
#define ACH_SPEEDRUNNER     4  // land in < 30s
#define ACH_PERFECT         5  // velocity < 0.5 at landing
#define ACH_HARD_MODE       6  // land on hard
#define ACH_KONAMI          7  // enter konami code
#define ACH_COUNT           8

void achievements_init();
void achievements_save();
bool achievement_unlocked(uint8_t id);
void achievement_unlock(uint8_t id);
int achievements_total();
int achievements_games_played();
void achievements_increment_games();

#ifndef NATIVE_TEST
void create_achievements_window();
#endif

#endif
