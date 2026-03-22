#ifndef NATIVE_TEST

#include "Game/LunarLobby.h"
#include "Game/LunarNet.h"
#include "Game/LunarState.h"
#include <ArduinoJson.h>
#include <Arduino.h>

// Forward declaration from LunarState.cpp
extern void multiplayer_begin(lv_obj_t *screen);
extern void show_mode_select_ext();
extern void spectator_start(lv_obj_t *parent);

static lv_obj_t *s_parent = NULL;
static lv_timer_t *s_lobby_timer = NULL;
static lv_obj_t *s_player_list_label = NULL;
static lv_obj_t *s_countdown_label = NULL;

// --- Name input for joining ---
static char s_player_name[32] = "Player";
static char s_selected_room[64] = "";
static uint8_t s_create_difficulty = 0;

static void show_waiting_lobby();
static void show_create_form();

static void lobby_cleanup_timer() {
    if (s_lobby_timer) {
        lv_timer_del(s_lobby_timer);
        s_lobby_timer = NULL;
    }
}

static void lobby_back_cb(lv_event_t *e) {
    lobby_cleanup_timer();
    net_disconnect();
    show_mode_select_ext();
}

// --- Waiting lobby: polls for player_list / countdown / game_started ---
static void waiting_poll_cb(lv_timer_t *t) {
    GameState dummy;
    net_poll(dummy);  // drives ws_client.poll() and parses lobby messages

    if (net_game_started()) {
        lobby_cleanup_timer();
        multiplayer_begin(s_parent);
        return;
    }

    if (net_has_lobby_update()) {
        net_clear_lobby_flags();
        JsonDocument doc;
        if (!deserializeJson(doc, net_get_lobby_json())) {
            const char *type = doc["type"];
            if (type && strcmp(type, "player_list") == 0) {
                String names;
                for (JsonVariant p : doc["players"].as<JsonArray>()) {
                    if (names.length()) names += "\n";
                    names += p.as<const char*>();
                }
                if (s_player_list_label) lv_label_set_text(s_player_list_label, names.c_str());
            }
            if (type && strcmp(type, "countdown") == 0) {
                int secs = doc["seconds"] | 0;
                if (s_countdown_label) lv_label_set_text_fmt(s_countdown_label, "Starting in %d...", secs);
            }
        }
    }
}

static void show_waiting_lobby() {
    lv_obj_clean(s_parent);

    lv_obj_t *title = lv_label_create(s_parent);
    lv_label_set_text(title, "WAITING FOR PLAYERS");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *room_lbl = lv_label_create(s_parent);
    lv_label_set_text_fmt(room_lbl, "Room: %s", net_get_room_id());
    lv_obj_set_style_text_color(room_lbl, lv_color_hex(0xaaaaaa), 0);
    lv_obj_align(room_lbl, LV_ALIGN_TOP_MID, 0, 30);

    s_player_list_label = lv_label_create(s_parent);
    lv_label_set_text(s_player_list_label, s_player_name);
    lv_obj_set_style_text_color(s_player_list_label, lv_color_hex(0x00ff00), 0);
    lv_obj_align(s_player_list_label, LV_ALIGN_CENTER, 0, -20);

    s_countdown_label = lv_label_create(s_parent);
    lv_label_set_text(s_countdown_label, "");
    lv_obj_set_style_text_color(s_countdown_label, lv_color_hex(0xffff00), 0);
    lv_obj_align(s_countdown_label, LV_ALIGN_CENTER, 0, 20);

    if (net_is_creator()) {
        lv_obj_t *start_btn = lv_btn_create(s_parent);
        lv_obj_set_size(start_btn, 100, 35);
        lv_obj_align(start_btn, LV_ALIGN_BOTTOM_MID, 0, -50);
        lv_obj_set_style_bg_color(start_btn, lv_color_hex(0x2e7d32), 0);
        lv_obj_add_event_cb(start_btn, [](lv_event_t *e) {
            net_start_game();
        }, LV_EVENT_CLICKED, NULL);
        lv_obj_t *sl = lv_label_create(start_btn);
        lv_label_set_text(sl, "START");
        lv_obj_center(sl);
    }

    lv_obj_t *back = lv_btn_create(s_parent);
    lv_obj_set_size(back, 80, 30);
    lv_obj_align(back, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(back, lobby_back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bl = lv_label_create(back);
    lv_label_set_text(bl, "BACK");
    lv_obj_center(bl);

    s_lobby_timer = lv_timer_create(waiting_poll_cb, 100, NULL);
}

// --- Name input prompt (used for both join and create) ---
static bool s_is_create_flow = false;

static void name_ok_cb(lv_event_t *e) {
    lv_obj_t *ta = (lv_obj_t *)lv_event_get_user_data(e);
    const char *text = lv_textarea_get_text(ta);
    if (text && strlen(text) > 0) {
        strncpy(s_player_name, text, sizeof(s_player_name) - 1);
        s_player_name[sizeof(s_player_name) - 1] = '\0';
    }

    if (!net_connect_player()) {
        lv_obj_clean(s_parent);
        lv_obj_t *err = lv_label_create(s_parent);
        lv_label_set_text(err, "Connection failed");
        lv_obj_set_style_text_color(err, lv_color_hex(0xff4444), 0);
        lv_obj_align(err, LV_ALIGN_CENTER, 0, -10);
        lv_obj_t *back = lv_btn_create(s_parent);
        lv_obj_set_size(back, 80, 30);
        lv_obj_align(back, LV_ALIGN_CENTER, 0, 30);
        lv_obj_add_event_cb(back, lobby_back_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_t *bl = lv_label_create(back);
        lv_label_set_text(bl, "BACK");
        lv_obj_center(bl);
        return;
    }

    if (s_is_create_flow) {
        net_create_room(s_player_name, s_create_difficulty);
    } else {
        net_join_room(s_selected_room, s_player_name);
    }
    show_waiting_lobby();
}

static void show_name_input() {
    lv_obj_clean(s_parent);

    lv_obj_t *lbl = lv_label_create(s_parent);
    lv_label_set_text(lbl, "Enter your name:");
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 30);

    lv_obj_t *ta = lv_textarea_create(s_parent);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_max_length(ta, 20);
    lv_textarea_set_text(ta, s_player_name);
    lv_obj_set_size(ta, 180, 40);
    lv_obj_align(ta, LV_ALIGN_CENTER, 0, -20);

    lv_obj_t *kb = lv_keyboard_create(s_parent);
    lv_keyboard_set_textarea(kb, ta);
    lv_obj_set_size(kb, LN_SCREEN_W, 120);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_obj_t *ok_btn = lv_btn_create(s_parent);
    lv_obj_set_size(ok_btn, 60, 30);
    lv_obj_align(ok_btn, LV_ALIGN_CENTER, 60, -20);
    lv_obj_set_style_bg_color(ok_btn, lv_color_hex(0x2e7d32), 0);
    lv_obj_add_event_cb(ok_btn, name_ok_cb, LV_EVENT_CLICKED, ta);
    lv_obj_t *ol = lv_label_create(ok_btn);
    lv_label_set_text(ol, "OK");
    lv_obj_center(ol);
}

// --- Create room form: difficulty select ---
static void create_diff_cb(lv_event_t *e) {
    s_create_difficulty = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    s_is_create_flow = true;
    show_name_input();
}

static void show_create_form() {
    lv_obj_clean(s_parent);

    lv_obj_t *title = lv_label_create(s_parent);
    lv_label_set_text(title, "CREATE ROOM");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    const char *labels[] = {"EASY", "MEDIUM", "HARD"};
    for (int i = 0; i < 3; i++) {
        lv_obj_t *btn = lv_btn_create(s_parent);
        lv_obj_set_size(btn, 120, 35);
        lv_obj_align(btn, LV_ALIGN_CENTER, 0, -30 + i * 45);
        lv_obj_add_event_cb(btn, create_diff_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, labels[i]);
        lv_obj_center(lbl);
    }

    lv_obj_t *back = lv_btn_create(s_parent);
    lv_obj_set_size(back, 80, 30);
    lv_obj_align(back, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(back, [](lv_event_t *e) { lobby_show_rooms(s_parent); }, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bl = lv_label_create(back);
    lv_label_set_text(bl, "BACK");
    lv_obj_center(bl);
}

// --- Room list (main lobby screen) ---
static void room_item_cb(lv_event_t *e) {
    const char *room_id = (const char *)lv_event_get_user_data(e);
    strncpy(s_selected_room, room_id, sizeof(s_selected_room) - 1);
    s_selected_room[sizeof(s_selected_room) - 1] = '\0';
    s_is_create_flow = false;
    show_name_input();
}

void lobby_show_rooms(lv_obj_t *parent) {
    s_parent = parent;
    lobby_cleanup_timer();
    lv_obj_clean(s_parent);
    lv_obj_set_style_bg_color(s_parent, lv_color_black(), 0);

    lv_obj_t *title = lv_label_create(s_parent);
    lv_label_set_text(title, "MULTIPLAYER ROOMS");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

    // Fetch rooms from server
    JsonDocument doc;
    bool ok = net_fetch_rooms(doc);

    if (!ok || doc.as<JsonArray>().size() == 0) {
        lv_obj_t *empty = lv_label_create(s_parent);
        lv_label_set_text(empty, ok ? "No rooms available" : "Server unreachable");
        lv_obj_set_style_text_color(empty, lv_color_hex(0xaaaaaa), 0);
        lv_obj_align(empty, LV_ALIGN_CENTER, 0, -20);
    } else {
        lv_obj_t *list = lv_list_create(s_parent);
        lv_obj_set_size(list, LN_SCREEN_W - 20, 140);
        lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 25);
        lv_obj_set_style_bg_color(list, lv_color_hex(0x111111), 0);

        // Store room IDs in static buffer (max 8 rooms displayed)
        static char room_ids[8][64];
        int idx = 0;
        for (JsonObject room : doc.as<JsonArray>()) {
            if (idx >= 8) break;
            const char *rid = room["id"] | "???";
            const char *name = room["name"] | rid;
            int players = room["players"] | 0;
            int diff = room["difficulty"] | 0;
            const char *diff_str = diff == 0 ? "E" : (diff == 1 ? "M" : "H");

            strncpy(room_ids[idx], rid, 63);
            room_ids[idx][63] = '\0';

            char buf[64];
            snprintf(buf, sizeof(buf), "%s  [%d] %s", name, players, diff_str);
            lv_obj_t *btn = lv_list_add_btn(list, NULL, buf);
            lv_obj_add_event_cb(btn, room_item_cb, LV_EVENT_CLICKED, room_ids[idx]);
            idx++;
        }
    }

    // CREATE button
    lv_obj_t *create_btn = lv_btn_create(s_parent);
    lv_obj_set_size(create_btn, 120, 35);
    lv_obj_align(create_btn, LV_ALIGN_BOTTOM_MID, -55, -10);
    lv_obj_set_style_bg_color(create_btn, lv_color_hex(0x1565c0), 0);
    lv_obj_add_event_cb(create_btn, [](lv_event_t *e) { show_create_form(); }, LV_EVENT_CLICKED, NULL);
    lv_obj_t *cl = lv_label_create(create_btn);
    lv_label_set_text(cl, "CREATE");
    lv_obj_center(cl);

    // BACK button
    lv_obj_t *back = lv_btn_create(s_parent);
    lv_obj_set_size(back, 80, 35);
    lv_obj_align(back, LV_ALIGN_BOTTOM_MID, 55, -10);
    lv_obj_add_event_cb(back, lobby_back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bl = lv_label_create(back);
    lv_label_set_text(bl, "BACK");
    lv_obj_center(bl);
}

// --- Spectator session browser ---
static void session_item_cb(lv_event_t *e) {
    const char *session_id = (const char *)lv_event_get_user_data(e);
    if (!session_id) return;
    if (net_spectate(session_id)) {
        spectator_start(s_parent);
    } else {
        lv_obj_t *err = lv_label_create(s_parent);
        lv_label_set_text(err, "Failed to connect");
        lv_obj_set_style_text_color(err, lv_color_make(255, 60, 60), 0);
        lv_obj_align(err, LV_ALIGN_BOTTOM_MID, 0, -45);
    }
}

void lobby_show_sessions(lv_obj_t *parent) {
    s_parent = parent;
    lobby_cleanup_timer();
    lv_obj_clean(s_parent);
    lv_obj_set_style_bg_color(s_parent, lv_color_black(), 0);

    lv_obj_t *title = lv_label_create(s_parent);
    lv_label_set_text(title, "ACTIVE GAMES");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

    JsonDocument doc;
    bool ok = net_fetch_rooms(doc);

    if (!ok || doc.as<JsonArray>().size() == 0) {
        lv_obj_t *msg = lv_label_create(s_parent);
        lv_label_set_text(msg, "No active games found");
        lv_obj_set_style_text_color(msg, lv_color_hex(0xaaaaaa), 0);
        lv_obj_align(msg, LV_ALIGN_CENTER, 0, 0);
    } else {
        lv_obj_t *list = lv_list_create(s_parent);
        lv_obj_set_size(list, 280, 150);
        lv_obj_align(list, LV_ALIGN_CENTER, 0, -10);
        lv_obj_set_style_bg_color(list, lv_color_hex(0x111111), 0);

        static char session_ids[8][64];
        int idx = 0;
        for (JsonObject room : doc.as<JsonArray>()) {
            if (idx >= 8) break;
            const char *id = room["id"] | "???";
            const char *name = room["name"] | id;
            int players = room["players"] | 1;

            strncpy(session_ids[idx], id, 63);
            session_ids[idx][63] = '\0';

            char buf[64];
            snprintf(buf, sizeof(buf), "%s  (%d player%s)", name, players, players == 1 ? "" : "s");
            lv_obj_t *btn = lv_list_add_btn(list, NULL, buf);
            lv_obj_add_event_cb(btn, session_item_cb, LV_EVENT_CLICKED, session_ids[idx]);
            idx++;
        }
    }

    // REFRESH button
    lv_obj_t *ref_btn = lv_btn_create(s_parent);
    lv_obj_set_size(ref_btn, 100, 30);
    lv_obj_align(ref_btn, LV_ALIGN_BOTTOM_MID, -60, -10);
    lv_obj_add_event_cb(ref_btn, [](lv_event_t *e) { lobby_show_sessions(s_parent); }, LV_EVENT_CLICKED, NULL);
    lv_obj_t *rl = lv_label_create(ref_btn);
    lv_label_set_text(rl, "REFRESH");
    lv_obj_center(rl);

    // BACK button
    lv_obj_t *back = lv_btn_create(s_parent);
    lv_obj_set_size(back, 80, 30);
    lv_obj_align(back, LV_ALIGN_BOTTOM_MID, 60, -10);
    lv_obj_add_event_cb(back, lobby_back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bl = lv_label_create(back);
    lv_label_set_text(bl, "BACK");
    lv_obj_center(bl);
}

#endif // NATIVE_TEST
