#include "Game/LunarScoreboard.h"
#include <string.h>

void scoreboard_init(Scoreboard &sb) {
    memset(&sb, 0, sizeof(sb));
}

int scoreboard_add(Scoreboard &sb, uint16_t score, uint8_t difficulty) {
    if (score == 0) return -1;

    // Find insertion point (sorted descending)
    int pos = sb.count;
    for (int i = 0; i < sb.count; i++) {
        if (score > sb.entries[i].score) {
            pos = i;
            break;
        }
    }
    if (pos >= LN_MAX_SCORES) return -1;

    // Shift entries down
    int end = sb.count < LN_MAX_SCORES ? sb.count : LN_MAX_SCORES - 1;
    for (int i = end; i > pos; i--) {
        sb.entries[i] = sb.entries[i - 1];
    }
    sb.entries[pos] = {score, difficulty};
    if (sb.count < LN_MAX_SCORES) sb.count++;
    return pos;
}

#ifndef NATIVE_TEST
#include <SD.h>

static const char *SCORE_FILE = "/lander_scores.dat";

void scoreboard_load(Scoreboard &sb) {
    scoreboard_init(sb);
    File f = SD.open(SCORE_FILE, FILE_READ);
    if (!f) return;
    f.read((uint8_t *)&sb, sizeof(sb));
    f.close();
    // Validate
    if (sb.count > LN_MAX_SCORES) scoreboard_init(sb);
}

void scoreboard_save(const Scoreboard &sb) {
    File f = SD.open(SCORE_FILE, FILE_WRITE);
    if (!f) return;
    f.write((const uint8_t *)&sb, sizeof(sb));
    f.close();
}

#else
void scoreboard_load(Scoreboard &sb) { scoreboard_init(sb); }
void scoreboard_save(const Scoreboard &sb) { (void)sb; }
#endif
