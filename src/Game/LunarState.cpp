#include "Game/LunarState.h"

void game_init(GameState &gs, uint8_t difficulty, uint32_t seed) {
    gs.difficulty = difficulty;
    gs.phase = PHASE_MENU;
    gs.mode = MODE_OFFLINE;
    gs.start_ms = 0;
    gs.elapsed_ms = 0;
    gs.last_tick_ms = 0;
    gs.accum_ms = 0;
    gs.score = 0;
    lander_init(gs.lander);
    terrain_generate(gs.terrain, difficulty, seed);
}

void game_tick(GameState &gs, bool thrust, int rotate_dir, uint32_t now_ms) {
    if (gs.phase == PHASE_LANDED || gs.phase == PHASE_CRASHED) return;

    if (gs.phase == PHASE_MENU && (thrust || rotate_dir != 0)) {
        gs.phase = PHASE_PLAYING;
        gs.start_ms = now_ms;
        gs.last_tick_ms = now_ms;
    }

    if (gs.phase != PHASE_PLAYING) return;

    gs.elapsed_ms = now_ms - gs.start_ms;

    // Fixed-step physics: run multiple steps if frame took longer than 1/60s
    // This ensures physics runs at real-time speed regardless of frame rate
    uint32_t dt_ms = now_ms - gs.last_tick_ms;
    if (dt_ms > 100) dt_ms = 100;  // cap at 100ms to prevent spiral of death
    gs.last_tick_ms = now_ms;

    // Accumulate time and step in fixed increments of ~16.67ms (1/60s)
    gs.accum_ms += dt_ms;
    const uint32_t step_ms = 16;  // ~60Hz
    while (gs.accum_ms >= step_ms) {
        lander_update(gs.lander, thrust, rotate_dir, LN_PHYSICS_DT);
        terrain_check_collision(gs.terrain, gs.lander);
        if (gs.lander.landed || gs.lander.crashed) break;
        gs.accum_ms -= step_ms;
    }
    if (gs.lander.landed || gs.lander.crashed) gs.accum_ms = 0;

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
#include <WiFi.h>
#include "Game/LunarRenderer.h"
#include "Game/LunarInput.h"
#include "Game/LunarAudio.h"
#include "Game/LunarLobby.h"
#include "Game/LunarNet.h"
#include "Game/LunarScoreboard.h"
#include "QA/Menu.h"
#include "QA/Bling.hpp"

static GameState gs;
static Scoreboard scoreboard;
static lv_timer_t *game_timer = NULL;
static lv_timer_t *spectate_timer = NULL;
static lv_timer_t *mp_timer = NULL;
static lv_obj_t *game_screen = NULL;
static bool was_thrusting = false;
static bool mp_was_thrusting = false;
static int mp_last_rotate = 0;

static void game_tick_cb(lv_timer_t *t);
static void start_game(uint8_t difficulty);
static void show_mode_select();
static void show_difficulty_select();
static void show_game_over();
static void spectator_tick_cb(lv_timer_t *t);
static void spectator_show_result();
static void multiplayer_tick_cb(lv_timer_t *t);
static void show_mp_game_over();

// Exposed for LunarLobby callbacks
void show_mode_select_ext() { show_mode_select(); }
void spectator_start(lv_obj_t *parent) {
    lv_obj_clean(parent);
    game_init(gs, 0, 0);
    gs.phase = PHASE_MENU;  // will become PHASE_PLAYING on first "init" message
    renderer_init(parent);
    spectate_timer = lv_timer_create(spectator_tick_cb, 16, NULL);
}

void multiplayer_begin(lv_obj_t *parent) {
    lv_obj_clean(parent);
    game_init(gs, 0, 0);
    gs.phase = PHASE_MENU;
    input_init();
    renderer_init(parent);
    mp_was_thrusting = false;
    mp_last_rotate = 0;
    mp_timer = lv_timer_create(multiplayer_tick_cb, 16, NULL);
}

static void multiplayer_tick_cb(lv_timer_t *t) {
    InputState inp = input_read();

    if (inp.back) {
        lv_timer_del(mp_timer);
        mp_timer = NULL;
        audio_thrust_stop();
        net_disconnect();
        renderer_cleanup();
        show_mode_select();
        return;
    }

    // Send input state changes to server
    if (inp.thrust && !mp_was_thrusting) net_send_input("thrust_on");
    if (!inp.thrust && mp_was_thrusting) net_send_input("thrust_off");
    if (inp.rotate_dir != mp_last_rotate) {
        if (inp.rotate_dir == -1) net_send_input("rotate_left");
        else if (inp.rotate_dir == 1) net_send_input("rotate_right");
        else net_send_input("rotate_stop");
    }
    mp_was_thrusting = inp.thrust;
    mp_last_rotate = inp.rotate_dir;

    // Poll server for authoritative state
    net_poll(gs);

    // Audio/LED feedback
    if (gs.lander.thrusting && !was_thrusting) audio_thrust_start();
    if (!gs.lander.thrusting && was_thrusting) audio_thrust_stop();
    was_thrusting = gs.lander.thrusting;
    if (gs.lander.thrusting) leds_thrust();
    else leds_idle();

    if (gs.phase == PHASE_PLAYING) {
        renderer_draw(gs);
        if (gs.lander.fuel < 100.0f && gs.lander.fuel > 0.0f) {
            static uint32_t last_beep = 0;
            if (millis() - last_beep > 2000) {
                audio_low_fuel();
                last_beep = millis();
            }
        }
    } else if (gs.phase == PHASE_LANDED || gs.phase == PHASE_CRASHED) {
        renderer_draw(gs);
        lv_timer_del(mp_timer);
        mp_timer = NULL;
        audio_thrust_stop();
        if (gs.phase == PHASE_LANDED) { audio_landed(); leds_landed(); }
        else { audio_crashed(); leds_crashed(); }
        show_mp_game_over();
    }

    // Handle disconnect
    if (net_get_mode() == NET_DISCONNECTED && gs.phase == PHASE_PLAYING) {
        lv_timer_del(mp_timer);
        mp_timer = NULL;
        audio_thrust_stop();
        renderer_cleanup();
        lv_obj_clean(game_screen);
        lv_obj_t *err = lv_label_create(game_screen);
        lv_label_set_text(err, "Disconnected from server");
        lv_obj_set_style_text_color(err, lv_color_hex(0xff4444), 0);
        lv_obj_align(err, LV_ALIGN_CENTER, 0, -10);
        lv_obj_t *back = lv_btn_create(game_screen);
        lv_obj_set_size(back, 80, 30);
        lv_obj_align(back, LV_ALIGN_CENTER, 0, 30);
        lv_obj_add_event_cb(back, [](lv_event_t *e) { show_mode_select(); }, LV_EVENT_CLICKED, NULL);
        lv_obj_t *bl = lv_label_create(back);
        lv_label_set_text(bl, "MENU");
        lv_obj_center(bl);
    }
}

static void show_mp_game_over() {
    lv_obj_t *result = lv_label_create(game_screen);
    lv_obj_set_style_text_color(result, lv_color_white(), 0);
    if (gs.phase == PHASE_LANDED) {
        lv_label_set_text_fmt(result, "LANDED!  Score: %d", gs.score);
    } else {
        lv_label_set_text(result, "CRASHED!");
    }
    lv_obj_align(result, LV_ALIGN_CENTER, 0, -20);

    // LOBBY button — return to room browser
    lv_obj_t *lobby_btn = lv_btn_create(game_screen);
    lv_obj_set_size(lobby_btn, 80, 30);
    lv_obj_align(lobby_btn, LV_ALIGN_CENTER, -50, 20);
    lv_obj_add_event_cb(lobby_btn, [](lv_event_t *e) {
        net_disconnect();
        renderer_cleanup();
        lobby_show_rooms(game_screen);
    }, LV_EVENT_CLICKED, NULL);
    lv_obj_t *ll = lv_label_create(lobby_btn);
    lv_label_set_text(ll, "AGAIN");
    lv_obj_center(ll);

    // MENU button
    lv_obj_t *menu = lv_btn_create(game_screen);
    lv_obj_set_size(menu, 80, 30);
    lv_obj_align(menu, LV_ALIGN_CENTER, 50, 20);
    lv_obj_add_event_cb(menu, [](lv_event_t *e) {
        net_disconnect();
        lunar_lander_stop();
    }, LV_EVENT_CLICKED, NULL);
    lv_obj_t *ml = lv_label_create(menu);
    lv_label_set_text(ml, "MENU");
    lv_obj_center(ml);
}

static void game_tick_cb(lv_timer_t *t) {
    InputState inp = input_read();

    if (inp.back) {
        if (gs.mode == MODE_ONLINE_SOLO) net_disconnect();
        lunar_lander_stop();
        return;
    }

    // Online solo: wait for server init
    if (gs.mode == MODE_ONLINE_SOLO && gs.phase == PHASE_WAITING) {
        net_poll(gs); // sets PHASE_PLAYING on "init" message
        renderer_draw(gs);
        return;
    }

    // Online solo: send inputs to server and poll for state
    if (gs.mode == MODE_ONLINE_SOLO && gs.phase == PHASE_PLAYING) {
        if (inp.thrust && !was_thrusting) net_send_input("thrust_on");
        if (!inp.thrust && was_thrusting) net_send_input("thrust_off");
        if (inp.rotate_dir != solo_last_rotate) {
            if (inp.rotate_dir == -1) net_send_input("rotate_left");
            else if (inp.rotate_dir == 1) net_send_input("rotate_right");
            else net_send_input("rotate_stop");
        }
        solo_last_rotate = inp.rotate_dir;
        net_poll(gs);
    } else {
        game_tick(gs, inp.thrust, inp.rotate_dir, millis());
    }

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
        if (gs.mode == MODE_ONLINE_SOLO) net_disconnect();
        show_game_over();
    } else if (gs.phase == PHASE_CRASHED) {
        lv_timer_del(game_timer);
        game_timer = NULL;
        audio_thrust_stop();
        audio_crashed();
        leds_crashed();
        if (gs.mode == MODE_ONLINE_SOLO) net_disconnect();
        show_game_over();
    }
}

static void spectator_tick_cb(lv_timer_t *t) {
    InputState inp = input_read();
    if (inp.back) {
        lv_timer_del(spectate_timer);
        spectate_timer = NULL;
        net_disconnect();
        renderer_cleanup();
        show_mode_select();
        return;
    }

    net_poll(gs);
    if (gs.phase == PHASE_PLAYING || gs.phase == PHASE_MENU) {
        renderer_draw(gs);
    } else if (gs.phase == PHASE_LANDED || gs.phase == PHASE_CRASHED) {
        renderer_draw(gs);
        lv_timer_del(spectate_timer);
        spectate_timer = NULL;
        net_disconnect();
        spectator_show_result();
    }
}

static void spectator_show_result() {
    lv_obj_t *result = lv_label_create(game_screen);
    lv_obj_set_style_text_color(result, lv_color_white(), 0);
    if (gs.phase == PHASE_LANDED) {
        lv_label_set_text_fmt(result, "LANDED!  Score: %d", gs.score);
    } else {
        lv_label_set_text(result, "CRASHED!");
    }
    lv_obj_align(result, LV_ALIGN_CENTER, 0, -20);

    lv_obj_t *back = lv_btn_create(game_screen);
    lv_obj_set_size(back, 80, 30);
    lv_obj_align(back, LV_ALIGN_CENTER, 0, 20);
    lv_obj_add_event_cb(back, [](lv_event_t *e) {
        renderer_cleanup();
        show_mode_select();
    }, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bl = lv_label_create(back);
    lv_label_set_text(bl, "BACK");
    lv_obj_center(bl);
}

static GameMode pending_mode = MODE_OFFLINE;
static lv_obj_t *mode_label = NULL;

static void diff_btn_cb(lv_event_t *e) {
    uint8_t diff = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    gs.mode = pending_mode;
    start_game(diff);
}

static int solo_last_rotate = 0;

static void start_game(uint8_t difficulty) {
    lv_obj_clean(game_screen);
    if (gs.mode == MODE_ONLINE_SOLO) {
        net_init();
        if (net_connect_player()) {
            net_send_start(difficulty);
            game_init(gs, difficulty, (uint32_t)millis());
            gs.mode = MODE_ONLINE_SOLO;
            gs.phase = PHASE_WAITING;
        } else {
            gs.mode = MODE_OFFLINE;
            game_init(gs, difficulty, (uint32_t)millis());
        }
    } else {
        game_init(gs, difficulty, (uint32_t)millis());
    }
    input_init();
    renderer_init(game_screen);
    was_thrusting = false;
    solo_last_rotate = 0;
    game_timer = lv_timer_create(game_tick_cb, 16, NULL);
}

static void show_mode_select() {
    lv_obj_clean(game_screen);
    lv_obj_set_style_bg_color(game_screen, lv_color_black(), 0);

    lv_obj_t *title = lv_label_create(game_screen);
    lv_label_set_text(title, "LUNAR LANDER");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    bool wifi = (WiFi.status() == WL_CONNECTED);

    // PLAY button — always enabled
    lv_obj_t *play_btn = lv_btn_create(game_screen);
    lv_obj_set_size(play_btn, 120, 32);
    lv_obj_align(play_btn, LV_ALIGN_TOP_MID, 0, 35);
    lv_obj_set_style_bg_color(play_btn, lv_color_hex(0x2e7d32), 0);
    lv_obj_add_event_cb(play_btn, [](lv_event_t *e) { show_difficulty_select(); }, LV_EVENT_CLICKED, NULL);
    lv_obj_t *pl = lv_label_create(play_btn);
    lv_label_set_text(pl, "PLAY");
    lv_obj_center(pl);

    // SPECTATE button
    lv_obj_t *spec_btn = lv_btn_create(game_screen);
    lv_obj_set_size(spec_btn, 120, 32);
    lv_obj_align(spec_btn, LV_ALIGN_TOP_MID, 0, 75);
    if (!wifi) {
        lv_obj_add_state(spec_btn, LV_STATE_DISABLED);
        lv_obj_set_style_bg_color(spec_btn, lv_color_hex(0x555555), LV_STATE_DISABLED);
        lv_obj_set_style_text_color(spec_btn, lv_color_hex(0x888888), LV_STATE_DISABLED);
    }
    lv_obj_add_event_cb(spec_btn, [](lv_event_t *e) {
        lv_obj_clean(game_screen);
        lv_obj_set_style_bg_color(game_screen, lv_color_black(), 0);
        lobby_show_sessions(game_screen);
    }, LV_EVENT_CLICKED, NULL);
    lv_obj_t *sl = lv_label_create(spec_btn);
    lv_label_set_text(sl, "SPECTATE");
    lv_obj_center(sl);

    // MULTIPLAYER button
    lv_obj_t *mp_btn = lv_btn_create(game_screen);
    lv_obj_set_size(mp_btn, 120, 32);
    lv_obj_align(mp_btn, LV_ALIGN_TOP_MID, 0, 115);
    if (!wifi) {
        lv_obj_add_state(mp_btn, LV_STATE_DISABLED);
        lv_obj_set_style_bg_color(mp_btn, lv_color_hex(0x555555), LV_STATE_DISABLED);
        lv_obj_set_style_text_color(mp_btn, lv_color_hex(0x888888), LV_STATE_DISABLED);
    }
    lv_obj_add_event_cb(mp_btn, [](lv_event_t *e) {
        lv_obj_clean(game_screen);
        lv_obj_set_style_bg_color(game_screen, lv_color_black(), 0);
        lobby_show_rooms(game_screen);
    }, LV_EVENT_CLICKED, NULL);
    lv_obj_t *ml = lv_label_create(mp_btn);
    lv_label_set_text(ml, "MULTIPLAYER");
    lv_obj_center(ml);

    // BACK button
    lv_obj_t *back = lv_btn_create(game_screen);
    lv_obj_set_size(back, 80, 28);
    lv_obj_align(back, LV_ALIGN_TOP_MID, 0, 160);
    lv_obj_add_event_cb(back, [](lv_event_t *e) { lunar_lander_stop(); }, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bl = lv_label_create(back);
    lv_label_set_text(bl, "BACK");
    lv_obj_center(bl);
}

static void show_difficulty_select() {
    lv_obj_clean(game_screen);
    lv_obj_set_style_bg_color(game_screen, lv_color_black(), 0);

    bool wifi = (WiFi.status() == WL_CONNECTED);
    pending_mode = wifi ? MODE_ONLINE_SOLO : MODE_OFFLINE;

    lv_obj_t *title = lv_label_create(game_screen);
    lv_label_set_text(title, "SELECT DIFFICULTY");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    const char *labels[] = {"EASY", "MEDIUM", "HARD"};
    for (int i = 0; i < 3; i++) {
        lv_obj_t *btn = lv_btn_create(game_screen);
        lv_obj_set_size(btn, 120, 32);
        lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 35 + i * 40);
        lv_obj_add_event_cb(btn, diff_btn_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, labels[i]);
        lv_obj_center(lbl);
    }

    // Mode toggle label
    mode_label = lv_label_create(game_screen);
    if (wifi) {
        lv_label_set_text(mode_label, "Playing Online");
        lv_obj_set_style_text_color(mode_label, lv_color_hex(0x00c853), 0);
        lv_obj_add_flag(mode_label, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(mode_label, [](lv_event_t *e) {
            if (pending_mode == MODE_ONLINE_SOLO) {
                pending_mode = MODE_OFFLINE;
                lv_label_set_text(mode_label, "Playing Offline");
                lv_obj_set_style_text_color(mode_label, lv_color_hex(0x888888), 0);
            } else {
                pending_mode = MODE_ONLINE_SOLO;
                lv_label_set_text(mode_label, "Playing Online");
                lv_obj_set_style_text_color(mode_label, lv_color_hex(0x00c853), 0);
            }
        }, LV_EVENT_CLICKED, NULL);
    } else {
        lv_label_set_text(mode_label, "Playing Offline");
        lv_obj_set_style_text_color(mode_label, lv_color_hex(0x888888), 0);
    }
    lv_obj_align(mode_label, LV_ALIGN_TOP_MID, 0, 160);

    lv_obj_t *back = lv_btn_create(game_screen);
    lv_obj_set_size(back, 80, 28);
    lv_obj_align(back, LV_ALIGN_TOP_MID, 0, 185);
    lv_obj_add_event_cb(back, [](lv_event_t *e) { show_mode_select(); }, LV_EVENT_CLICKED, NULL);
    lv_obj_t *blbl = lv_label_create(back);
    lv_label_set_text(blbl, "BACK");
    lv_obj_center(blbl);
}

static void show_game_over() {
    int rank = -1;
    if (gs.phase == PHASE_LANDED && gs.score > 0) {
        rank = scoreboard_add(scoreboard, gs.score, gs.difficulty);
        scoreboard_save(scoreboard);
    }

    lv_obj_t *result = lv_label_create(game_screen);
    lv_obj_set_style_text_color(result, lv_color_white(), 0);
    if (gs.phase == PHASE_LANDED) {
        lv_label_set_text_fmt(result, "LANDED!  Score: %d", gs.score);
    } else {
        lv_label_set_text(result, "CRASHED!");
    }
    lv_obj_align(result, LV_ALIGN_TOP_MID, 0, 10);

    // Scoreboard
    if (scoreboard.count > 0) {
        lv_obj_t *hdr = lv_label_create(game_screen);
        lv_label_set_text(hdr, "HIGH SCORES");
        lv_obj_set_style_text_color(hdr, lv_color_hex(0x00e5ff), 0);
        lv_obj_align(hdr, LV_ALIGN_TOP_MID, 0, 35);

        static const char *diff_names[] = {"E", "M", "H"};
        for (int i = 0; i < scoreboard.count; i++) {
            lv_obj_t *row = lv_label_create(game_screen);
            const char *d = (scoreboard.entries[i].difficulty < 3) ? diff_names[scoreboard.entries[i].difficulty] : "?";
            lv_label_set_text_fmt(row, "%d. %5d [%s]", i + 1, scoreboard.entries[i].score, d);
            bool is_current = (rank == i);
            lv_obj_set_style_text_color(row, is_current ? lv_color_hex(0x00c853) : lv_color_hex(0xcccccc), 0);
            lv_obj_align(row, LV_ALIGN_TOP_MID, 0, 52 + i * 16);
        }
    }

    // Buttons
    lv_obj_t *again = lv_btn_create(game_screen);
    lv_obj_set_size(again, 80, 28);
    lv_obj_align(again, LV_ALIGN_BOTTOM_MID, -50, -10);
    lv_obj_add_event_cb(again, [](lv_event_t *e) {
        start_game(gs.difficulty);
    }, LV_EVENT_CLICKED, NULL);
    lv_obj_t *al = lv_label_create(again);
    lv_label_set_text(al, "AGAIN");
    lv_obj_center(al);

    lv_obj_t *menu = lv_btn_create(game_screen);
    lv_obj_set_size(menu, 80, 28);
    lv_obj_align(menu, LV_ALIGN_BOTTOM_MID, 50, -10);
    lv_obj_add_event_cb(menu, [](lv_event_t *e) { show_mode_select(); }, LV_EVENT_CLICKED, NULL);
    lv_obj_t *ml = lv_label_create(menu);
    lv_label_set_text(ml, "MENU");
    lv_obj_center(ml);
}

void lunar_lander_start() {
    bling_stop_animation();
    game_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(game_screen, lv_color_black(), 0);
    load_screen_and_delete_old(game_screen);
    leds_idle();
    scoreboard_load(scoreboard);
    show_mode_select();
}

void lunar_lander_stop() {
    if (game_timer) {
        lv_timer_del(game_timer);
        game_timer = NULL;
        if (gs.mode == MODE_ONLINE_SOLO) net_disconnect();
    }
    if (spectate_timer) {
        lv_timer_del(spectate_timer);
        spectate_timer = NULL;
        net_disconnect();
    }
    if (mp_timer) {
        lv_timer_del(mp_timer);
        mp_timer = NULL;
        net_disconnect();
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
