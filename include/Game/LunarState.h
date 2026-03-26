#ifndef LUNAR_STATE_H
#define LUNAR_STATE_H

#include "LunarConfig.h"
#include "LunarPhysics.h"
#include "LunarTerrain.h"
#include <stdint.h>

enum GameMode {
    MODE_OFFLINE,
    MODE_ONLINE_SOLO,
    MODE_ONLINE_MULTI,
    MODE_SPECTATE
};

enum GamePhase {
    PHASE_MENU,
    PHASE_WAITING,
    PHASE_PLAYING,
    PHASE_LANDED,
    PHASE_CRASHED
};

struct GameState {
    Lander lander;
    Terrain terrain;
    GamePhase phase;
    GameMode mode;
    uint32_t start_ms;
    uint32_t elapsed_ms;
    uint32_t last_tick_ms;  // tracks real time for fixed-step physics
    uint32_t accum_ms;     // physics time accumulator
    uint8_t difficulty;
    uint16_t score;
    LanderSkin skin;
};

void game_init(GameState &gs, uint8_t difficulty, uint32_t seed);
void game_tick(GameState &gs, bool thrust, int rotate_dir, uint32_t now_ms);
uint16_t game_calc_score(const GameState &gs);

#ifndef NATIVE_TEST
void lunar_lander_start();
void lunar_lander_stop();
const GameState* game_get_state();
#endif

#endif
