#include "QA/Achievements.h"
#include <string.h>

static uint8_t s_unlocked[ACH_COUNT] = {0};
static int s_total = 0;
static int s_games_played = 0;

static const char* ach_names[] = {
    "First Landing", "Eagle Scout", "Apollo 13",
    "Marathon", "Speedrunner", "Perfect Landing",
    "Hard Mode", "Konami Code"
};

#ifndef NATIVE_TEST

#include "QA/Menu.h"
#include "Game/LunarAudio.h"
#include <lvgl.h>
#include <SD.h>

void achievements_init() {
    memset(s_unlocked, 0, sizeof(s_unlocked));
    s_total = 0;
    s_games_played = 0;
    File f = SD.open("/achievements.dat", FILE_READ);
    if (f) {
        f.read(s_unlocked, ACH_COUNT);
        f.read((uint8_t *)&s_games_played, sizeof(s_games_played));
        f.close();
        for (int i = 0; i < ACH_COUNT; i++)
            if (s_unlocked[i]) s_total++;
    }
}

void achievements_save() {
    File f = SD.open("/achievements.dat", FILE_WRITE);
    if (f) {
        f.write(s_unlocked, ACH_COUNT);
        f.write((uint8_t *)&s_games_played, sizeof(s_games_played));
        f.close();
    }
}

bool achievement_unlocked(uint8_t id) {
    return id < ACH_COUNT && s_unlocked[id];
}

void achievement_unlock(uint8_t id) {
    if (id >= ACH_COUNT || s_unlocked[id]) return;
    s_unlocked[id] = 1;
    s_total++;
    achievements_save();
    audio_achievement();
    leds_achievement();
}

int achievements_total() { return s_total; }
int achievements_games_played() { return s_games_played; }

void achievements_increment_games() {
    s_games_played++;
    achievements_save();
    if (s_games_played >= 10) achievement_unlock(ACH_MARATHON);
}

void create_achievements_window() {
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0a0a0f), 0);
    load_screen_and_delete_old(scr);

    // HUD title
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text_fmt(title, "MISSION PATCHES  %d/%d", s_total, ACH_COUNT);
    lv_obj_set_style_text_color(title, lv_color_hex(0x00e5ff), 0);
    lv_obj_set_style_text_font(title, &lv_font_unscii_8, 0);
    lv_obj_set_pos(title, 8, 8);

    // Accent line
    lv_obj_t *line = lv_obj_create(scr);
    lv_obj_set_size(line, 312, 1);
    lv_obj_set_pos(line, 4, 22);
    lv_obj_set_style_bg_color(line, lv_color_hex(0x00e5ff), 0);
    lv_obj_set_style_border_width(line, 0, 0);
    lv_obj_set_style_radius(line, 0, 0);
    lv_obj_set_style_pad_all(line, 0, 0);
    lv_obj_clear_flag(line, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < ACH_COUNT; i++) {
        lv_obj_t *row = lv_label_create(scr);
        if (s_unlocked[i]) {
            lv_label_set_text_fmt(row, LV_SYMBOL_OK " %s", ach_names[i]);
            lv_obj_set_style_text_color(row, lv_color_hex(0x00c853), 0);
        } else {
            lv_label_set_text_fmt(row, LV_SYMBOL_CLOSE " %s", ach_names[i]);
            lv_obj_set_style_text_color(row, lv_color_hex(0x444444), 0);
        }
        lv_obj_set_pos(row, 20, 28 + i * 22);
    }

    lv_obj_t *back = lv_btn_create(scr);
    lv_obj_set_size(back, 80, 28);
    lv_obj_align(back, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_style_bg_color(back, lv_color_hex(0x222222), 0);
    lv_obj_add_event_cb(back, [](lv_event_t *e) { create_main_menu(false); }, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bl = lv_label_create(back);
    lv_label_set_text(bl, LV_SYMBOL_LEFT " BACK");
    lv_obj_set_style_text_color(bl, lv_color_hex(0x888888), 0);
    lv_obj_center(bl);
}

#else // NATIVE_TEST stubs

void achievements_init() {
    memset(s_unlocked, 0, sizeof(s_unlocked));
    s_total = 0;
    s_games_played = 0;
}
void achievements_save() {}
bool achievement_unlocked(uint8_t id) { return id < ACH_COUNT && s_unlocked[id]; }
void achievement_unlock(uint8_t id) {
    if (id >= ACH_COUNT || s_unlocked[id]) return;
    s_unlocked[id] = 1;
    s_total++;
}
int achievements_total() { return s_total; }
int achievements_games_played() { return s_games_played; }
void achievements_increment_games() {
    s_games_played++;
    if (s_games_played >= 10) achievement_unlock(ACH_MARATHON);
}

#endif
