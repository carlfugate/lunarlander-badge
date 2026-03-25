#include "QA/Callsign.h"
#include "QA/Menu.h"
#include <lvgl.h>
#include <SD.h>
#include <string.h>

static char s_callsign[MAX_CALLSIGN_LEN + 1] = "PILOT";

static const char* preset_callsigns[] = {
    "EAGLE", "COLUMBIA", "ODYSSEY", "AQUARIUS",
    "ENDEAVOUR", "DISCOVERY", "ATLANTIS", "PHOENIX",
    "ORION", "GEMINI", "MERCURY", "VOSTOK",
};
static const int NUM_PRESETS = sizeof(preset_callsigns) / sizeof(preset_callsigns[0]);

void callsign_init() {
    File f = SD.open("/callsign.txt", FILE_READ);
    if (f) {
        size_t n = f.readBytesUntil('\n', s_callsign, MAX_CALLSIGN_LEN);
        s_callsign[n] = '\0';
        f.close();
    }
}

const char* callsign_get() { return s_callsign; }

void callsign_set(const char* name) {
    strncpy(s_callsign, name, MAX_CALLSIGN_LEN);
    s_callsign[MAX_CALLSIGN_LEN] = '\0';
    File f = SD.open("/callsign.txt", FILE_WRITE);
    if (f) { f.println(s_callsign); f.close(); }
}

static void preset_cb(lv_event_t *e) {
    const char* name = (const char*)lv_event_get_user_data(e);
    callsign_set(name);
    create_system_submenu();
}

void create_callsign_window() {
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0a0a0f), 0);
    load_screen_and_delete_old(scr);

    // HUD title
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "CREW IDENTIFICATION");
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

    // Current callsign
    lv_obj_t *current = lv_label_create(scr);
    lv_label_set_text_fmt(current, "CURRENT: %s", s_callsign);
    lv_obj_set_style_text_color(current, lv_color_hex(0x00c853), 0);
    lv_obj_set_pos(current, 8, 26);

    // Custom entry
    lv_obj_t *ta = lv_textarea_create(scr);
    lv_textarea_set_max_length(ta, MAX_CALLSIGN_LEN);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_text(ta, s_callsign);
    lv_obj_set_size(ta, 160, 30);
    lv_obj_set_pos(ta, 4, 38);
    lv_obj_set_style_bg_color(ta, lv_color_hex(0x1a1a2e), 0);
    lv_obj_set_style_text_color(ta, lv_color_hex(0x00e5ff), 0);
    lv_obj_set_style_border_color(ta, lv_color_hex(0x00e5ff), 0);

    // Set button
    lv_obj_t *set_btn = lv_btn_create(scr);
    lv_obj_set_size(set_btn, 60, 30);
    lv_obj_set_pos(set_btn, 168, 38);
    lv_obj_set_style_bg_color(set_btn, lv_color_hex(0x00c853), 0);
    lv_obj_add_event_cb(set_btn, [](lv_event_t *e) {
        lv_obj_t *textarea = (lv_obj_t*)lv_event_get_user_data(e);
        const char *text = lv_textarea_get_text(textarea);
        if (text && text[0] != '\0') {
            callsign_set(text);
            create_callsign_window();
        }
    }, LV_EVENT_CLICKED, ta);
    lv_obj_t *set_lbl = lv_label_create(set_btn);
    lv_label_set_text(set_lbl, "SET");
    lv_obj_set_style_text_color(set_lbl, lv_color_hex(0x000000), 0);
    lv_obj_center(set_lbl);

    // On-screen keyboard
    lv_obj_t *kb = lv_keyboard_create(scr);
    lv_keyboard_set_textarea(kb, ta);
    lv_obj_set_size(kb, 320, 120);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);

    lv_obj_add_event_cb(ta, [](lv_event_t *e) {
        lv_obj_t *keyboard = (lv_obj_t*)lv_event_get_user_data(e);
        lv_obj_clear_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    }, LV_EVENT_FOCUSED, kb);

    lv_obj_add_event_cb(ta, [](lv_event_t *e) {
        lv_obj_t *keyboard = (lv_obj_t*)lv_event_get_user_data(e);
        lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    }, LV_EVENT_DEFOCUSED, kb);

    lv_obj_add_event_cb(kb, [](lv_event_t *e) {
        lv_keyboard_t *keyboard = (lv_keyboard_t*)lv_event_get_target(e);
        lv_obj_add_flag((lv_obj_t*)keyboard, LV_OBJ_FLAG_HIDDEN);
    }, LV_EVENT_READY, NULL);
    lv_obj_add_event_cb(kb, [](lv_event_t *e) {
        lv_keyboard_t *keyboard = (lv_keyboard_t*)lv_event_get_target(e);
        lv_obj_add_flag((lv_obj_t*)keyboard, LV_OBJ_FLAG_HIDDEN);
    }, LV_EVENT_CANCEL, NULL);

    for (int i = 0; i < NUM_PRESETS; i++) {
        lv_obj_t *btn = lv_btn_create(scr);
        lv_obj_set_size(btn, 96, 28);
        int col = i % 3;
        int row = i / 3;
        lv_obj_set_pos(btn, 8 + col * 104, 74 + row * 34);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x1a1a2e), 0);
        lv_obj_set_style_radius(btn, 6, 0);
        lv_obj_add_event_cb(btn, preset_cb, LV_EVENT_CLICKED, (void*)preset_callsigns[i]);

        // Highlight active callsign with green border
        if (strcmp(preset_callsigns[i], s_callsign) == 0) {
            lv_obj_set_style_border_color(btn, lv_color_hex(0x00c853), 0);
            lv_obj_set_style_border_width(btn, 2, 0);
        }

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, preset_callsigns[i]);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xcccccc), 0);
        lv_obj_center(lbl);
    }

    lv_obj_t *back = lv_btn_create(scr);
    lv_obj_set_size(back, 80, 28);
    lv_obj_align(back, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_style_bg_color(back, lv_color_hex(0x222222), 0);
    lv_obj_add_event_cb(back, [](lv_event_t *e) { create_system_submenu(); }, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bl = lv_label_create(back);
    lv_label_set_text(bl, LV_SYMBOL_LEFT " BACK");
    lv_obj_set_style_text_color(bl, lv_color_hex(0x888888), 0);
    lv_obj_center(bl);
}

#ifdef FF_SERIAL_TEST
#include "QA/SerialCmd.h"
static void callsign_serial_handler(const char *args) {
    if (strcmp(args, "get") == 0) {
        serial_cmd_log("CALLSIGN", "name=%s", callsign_get());
    } else if (strncmp(args, "set ", 4) == 0) {
        callsign_set(args + 4);
        serial_cmd_log("CALLSIGN", "name=%s", callsign_get());
    } else {
        serial_cmd_log("CALLSIGN", "error=unknown args=%s", args);
    }
}
void serial_register_callsign() {
    serial_cmd_register("callsign", callsign_serial_handler, "get, set <name>");
}
#else
void serial_register_callsign() {}
#endif
