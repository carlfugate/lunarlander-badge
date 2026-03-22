#ifndef LUNAR_STATE_H
#define LUNAR_STATE_H

#include "LunarConfig.h"
#include "LunarPhysics.h"
#include "LunarTerrain.h"
#include <stdint.h>

enum GamePhase {
    PHASE_MENU,
    PHASE_PLAYING,
    PHASE_LANDED,
    PHASE_CRASHED
};

struct GameState {
    Lander lander;
    Terrain terrain;
    GamePhase phase;
    uint32_t start_ms;
    uint32_t elapsed_ms;
    uint8_t difficulty;
    uint16_t score;
};

void game_init(GameState &gs, uint8_t difficulty, uint32_t seed);
void game_tick(GameState &gs, bool thrust, int rotate_dir, uint32_t now_ms);
uint16_t game_calc_score(const GameState &gs);

#ifndef NATIVE_TEST
void lunar_lander_start();
void lunar_lander_stop();
#endif

#endif
