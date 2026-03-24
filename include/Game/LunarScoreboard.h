#pragma once
#include <stdint.h>

#define LN_MAX_SCORES 5

struct ScoreEntry {
    uint16_t score;
    uint8_t difficulty;  // 0=easy, 1=med, 2=hard
};

struct Scoreboard {
    ScoreEntry entries[LN_MAX_SCORES];
    uint8_t count;
};

void scoreboard_init(Scoreboard &sb);
// Returns rank (0-4) if score made the board, -1 if not
int scoreboard_add(Scoreboard &sb, uint16_t score, uint8_t difficulty);
void scoreboard_load(Scoreboard &sb);  // Load from SD
void scoreboard_save(const Scoreboard &sb);  // Save to SD
