#include "Game/LunarState.h"

void game_init(GameState &gs, uint8_t difficulty, uint32_t seed) {
    gs.difficulty = difficulty;
    gs.phase = PHASE_MENU;
    gs.start_ms = 0;
    gs.elapsed_ms = 0;
    gs.score = 0;
    lander_init(gs.lander);
    terrain_generate(gs.terrain, difficulty, seed);
}

void game_tick(GameState &gs, bool thrust, int rotate_dir, uint32_t now_ms) {
    if (gs.phase == PHASE_LANDED || gs.phase == PHASE_CRASHED) return;

    if (gs.phase == PHASE_MENU && (thrust || rotate_dir != 0)) {
        gs.phase = PHASE_PLAYING;
        gs.start_ms = now_ms;
    }

    if (gs.phase != PHASE_PLAYING) return;

    gs.elapsed_ms = now_ms - gs.start_ms;
    lander_update(gs.lander, thrust, rotate_dir, LN_PHYSICS_DT);
    terrain_check_collision(gs.terrain, gs.lander);

    if (gs.lander.landed) {
        gs.phase = PHASE_LANDED;
        gs.score = game_calc_score(gs);
    } else if (gs.lander.crashed) {
        gs.phase = PHASE_CRASHED;
        gs.score = 0;
    }
}

uint16_t game_calc_score(const GameState &gs) {
    if (gs.lander.crashed || !gs.lander.landed) return 0;

    float fuel_bonus = (gs.lander.fuel / LN_INITIAL_FUEL) * LN_SCORE_FUEL_MAX;
    float elapsed_s = gs.elapsed_ms / 1000.0f;
    float time_bonus = LN_SCORE_TIME_MAX - (elapsed_s - LN_SCORE_TIME_FLOOR) * LN_SCORE_TIME_DECAY;
    if (time_bonus < 0.0f) time_bonus = 0.0f;
    if (time_bonus > LN_SCORE_TIME_MAX) time_bonus = LN_SCORE_TIME_MAX;

    float raw = LN_SCORE_BASE + fuel_bonus + time_bonus;

    // Difficulty multiplier: easy=1.0, med=1.5, hard=2.0
    int mult = LN_MULT_EASY;
    if (gs.difficulty == 1) mult = LN_MULT_MED;
    else if (gs.difficulty == 2) mult = LN_MULT_HARD;

    return (uint16_t)((raw * mult) / 10);
}
