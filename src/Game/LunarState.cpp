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

#ifndef NATIVE_TEST
#include <lvgl.h>
#include <Arduino.h>
#include "Game/LunarRenderer.h"
#include "Game/LunarInput.h"
#include "Game/LunarAudio.h"
#include "QA/Menu.h"

static GameState gs;
static lv_timer_t *game_timer = NULL;
static lv_obj_t *game_screen = NULL;
static bool was_thrusting = false;

static void game_tick_cb(lv_timer_t *t);
static void start_game(uint8_t difficulty);
static void show_difficulty_select();
static void show_game_over();

static void game_tick_cb(lv_timer_t *t) {
    InputState inp = input_read();

    if (inp.back) {
        lunar_lander_stop();
        return;
    }

    game_tick(gs, inp.thrust, inp.rotate_dir, millis());

    if (gs.lander.thrusting && !was_thrusting) audio_thrust_start();
    if (!gs.lander.thrusting && was_thrusting) audio_thrust_stop();
    was_thrusting = gs.lander.thrusting;

    if (gs.lander.thrusting) leds_thrust();
    else leds_idle();

    if (gs.phase == PHASE_PLAYING && gs.lander.fuel < 100.0f && gs.lander.fuel > 0.0f) {
        static uint32_t last_beep = 0;
        if (millis() - last_beep > 2000) {
            audio_low_fuel();
            last_beep = millis();
        }
    }

    renderer_draw(gs);

    if (gs.phase == PHASE_LANDED) {
        lv_timer_del(game_timer);
        game_timer = NULL;
        audio_thrust_stop();
        audio_landed();
        leds_landed();
        show_game_over();
    } else if (gs.phase == PHASE_CRASHED) {
        lv_timer_del(game_timer);
        game_timer = NULL;
        audio_thrust_stop();
        audio_crashed();
        leds_crashed();
        show_game_over();
    }
}

static void diff_btn_cb(lv_event_t *e) {
    uint8_t diff = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    start_game(diff);
}

static void start_game(uint8_t difficulty) {
    lv_obj_clean(game_screen);
    game_init(gs, difficulty, (uint32_t)millis());
    input_init();
    renderer_init(game_screen);
    was_thrusting = false;
    game_timer = lv_timer_create(game_tick_cb, 16, NULL);
}

static void show_difficulty_select() {
    lv_obj_clean(game_screen);
    lv_obj_set_style_bg_color(game_screen, lv_color_black(), 0);

    lv_obj_t *title = lv_label_create(game_screen);
    lv_label_set_text(title, "LUNAR LANDER");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);

    const char *labels[] = {"EASY", "MEDIUM", "HARD"};
    for (int i = 0; i < 3; i++) {
        lv_obj_t *btn = lv_btn_create(game_screen);
        lv_obj_set_size(btn, 120, 40);
        lv_obj_align(btn, LV_ALIGN_CENTER, 0, -30 + i * 50);
        lv_obj_add_event_cb(btn, diff_btn_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, labels[i]);
        lv_obj_center(lbl);
    }

    lv_obj_t *back = lv_btn_create(game_screen);
    lv_obj_set_size(back, 80, 30);
    lv_obj_align(back, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(back, [](lv_event_t *e) { lunar_lander_stop(); }, LV_EVENT_CLICKED, NULL);
    lv_obj_t *blbl = lv_label_create(back);
    lv_label_set_text(blbl, "BACK");
    lv_obj_center(blbl);
}

static void show_game_over() {
    lv_obj_t *result = lv_label_create(game_screen);
    lv_obj_set_style_text_color(result, lv_color_white(), 0);

    if (gs.phase == PHASE_LANDED) {
        lv_label_set_text_fmt(result, "LANDED!  Score: %d", gs.score);
    } else {
        lv_label_set_text(result, "CRASHED!");
    }
    lv_obj_align(result, LV_ALIGN_CENTER, 0, -20);

    lv_obj_t *again = lv_btn_create(game_screen);
    lv_obj_set_size(again, 80, 30);
    lv_obj_align(again, LV_ALIGN_CENTER, -50, 20);
    lv_obj_add_event_cb(again, [](lv_event_t *e) {
        start_game(gs.difficulty);
    }, LV_EVENT_CLICKED, NULL);
    lv_obj_t *al = lv_label_create(again);
    lv_label_set_text(al, "AGAIN");
    lv_obj_center(al);

    lv_obj_t *menu = lv_btn_create(game_screen);
    lv_obj_set_size(menu, 80, 30);
    lv_obj_align(menu, LV_ALIGN_CENTER, 50, 20);
    lv_obj_add_event_cb(menu, [](lv_event_t *e) { lunar_lander_stop(); }, LV_EVENT_CLICKED, NULL);
    lv_obj_t *ml = lv_label_create(menu);
    lv_label_set_text(ml, "MENU");
    lv_obj_center(ml);
}

void lunar_lander_start() {
    game_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(game_screen, lv_color_black(), 0);
    lv_scr_load(game_screen);
    leds_idle();
    show_difficulty_select();
}

void lunar_lander_stop() {
    if (game_timer) {
        lv_timer_del(game_timer);
        game_timer = NULL;
    }
    audio_thrust_stop();
    leds_idle();
    renderer_cleanup();
    if (game_screen) {
        lv_obj_del(game_screen);
        game_screen = NULL;
    }
    was_thrusting = false;
    create_main_menu(false);
}
#endif
