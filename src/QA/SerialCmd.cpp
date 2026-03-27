#include "QA/SerialCmd.h"

#ifdef FF_SERIAL_TEST

#include <Arduino.h>
#include <lvgl.h>
#include <string.h>
#include <stdarg.h>
#include "Includes.h"
#include "QA/Bling.hpp"
#include "QA/Screensaver.h"
#include "Game/LunarAudio.h"
#include "Game/LunarState.h"
#include "QA/Callsign.h"
#include "QA/Achievements.h"
#include "QA/BlePresence.h"

#define MAX_HANDLERS 24

static struct {
    const char *prefix;
    serial_cmd_handler_t handler;
    const char *help_text;
} s_handlers[MAX_HANDLERS];
static int s_handler_count = 0;

static char cmd_buf[128];
static int cmd_pos = 0;
static uint32_t s_loop_count = 0, s_loop_rate = 0, s_loop_last = 0;

void serial_cmd_register(const char *prefix, serial_cmd_handler_t handler, const char *help_text) {
    if (s_handler_count < MAX_HANDLERS) {
        s_handlers[s_handler_count] = {prefix, handler, help_text};
        s_handler_count++;
    }
}

void serial_cmd_log(const char *tag, const char *fmt, ...) {
    Serial.printf("[%s] ", tag);
    va_list args;
    va_start(args, fmt);
    char buf[256];
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    Serial.println(buf);
}

// Built-in: sys module
static void sys_handler(const char *args) {
    if (strcmp(args, "heap") == 0) {
        serial_cmd_log("SYS", "free=%d min=%d max_block=%d",
            ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getMaxAllocHeap());
    } else if (strcmp(args, "version") == 0) {
        serial_cmd_log("SYS", "firmware=%s codename=%s", BADGE_VERSION, BADGE_CODE_NAME);
    } else if (strcmp(args, "uptime") == 0) {
        serial_cmd_log("SYS", "ms=%lu", millis());
    } else if (strcmp(args, "temp") == 0) {
        serial_cmd_log("SYS", "temp_c=%.1f", temperatureRead());
    } else if (strcmp(args, "reboot") == 0) {
        serial_cmd_log("SYS", "rebooting");
        Serial.flush();
        ESP.restart();
    } else if (strcmp(args, "diag") == 0) {
        serial_cmd_log("DIAG", "heap_free=%d heap_min=%d heap_max_block=%d temp=%.1f uptime=%lu loop_rate=%d",
            ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getMaxAllocHeap(),
            temperatureRead(), millis(), s_loop_rate);
    } else {
        serial_cmd_log("SYS", "error=unknown args=%s", args);
    }
}

// Built-in: test module
static void test_handler(const char *args) {
    if (strncmp(args, "stress ", 7) == 0) {
        int cycles = atoi(args + 7);
        if (cycles <= 0) cycles = 10;
        serial_cmd_log("TEST", "stress starting cycles=%d", cycles);
        uint32_t start_heap = ESP.getFreeHeap();
        for (int i = 0; i < cycles; i++) {
            create_system_submenu();
            lv_timer_handler();
            create_main_menu(false);
            lv_timer_handler();
            serial_cmd_poll();
            if (i % 10 == 0)
                serial_cmd_log("TEST", "stress progress=%d/%d heap=%d", i, cycles, ESP.getFreeHeap());
        }
        serial_cmd_log("TEST", "stress complete cycles=%d heap_start=%d heap_end=%d delta=%d",
            cycles, start_heap, ESP.getFreeHeap(), (int)ESP.getFreeHeap() - (int)start_heap);
    } else if (strncmp(args, "idle ", 5) == 0) {
        int ms = atoi(args + 5);
        serial_cmd_log("TEST", "idle starting ms=%d", ms);
        uint32_t start = millis();
        uint32_t start_heap = ESP.getFreeHeap();
        while (millis() - start < (uint32_t)ms) {
            lv_timer_handler();
            serial_cmd_poll();
            delay(5);
        }
        serial_cmd_log("TEST", "idle complete ms=%d heap_start=%d heap_end=%d delta=%d",
            ms, start_heap, ESP.getFreeHeap(), (int)ESP.getFreeHeap() - (int)start_heap);
    } else {
        serial_cmd_log("TEST", "error=unknown args=%s", args);
    }
}

// Built-in: help
static void help_handler(const char *args) {
    if (args[0] == '\0') {
        serial_cmd_log("HELP", "modules=%d", s_handler_count);
        for (int i = 0; i < s_handler_count; i++)
            Serial.printf("  %s -- %s\n", s_handlers[i].prefix, s_handlers[i].help_text);
    } else {
        for (int i = 0; i < s_handler_count; i++) {
            if (strcmp(args, s_handlers[i].prefix) == 0) {
                serial_cmd_log("HELP", "module=%s commands=%s", s_handlers[i].prefix, s_handlers[i].help_text);
                return;
            }
        }
        serial_cmd_log("HELP", "error=unknown_module module=%s", args);
    }
}

static void process_command(char *cmd) {
    while (*cmd == ' ') cmd++;
    char *end = cmd + strlen(cmd) - 1;
    while (end > cmd && (*end == ' ' || *end == '\r' || *end == '\n')) *end-- = '\0';
    if (strlen(cmd) == 0) return;

    for (int i = 0; i < s_handler_count; i++) {
        int plen = strlen(s_handlers[i].prefix);
        if (strncmp(cmd, s_handlers[i].prefix, plen) == 0) {
            if (cmd[plen] == '\0') {
                s_handlers[i].handler("");
                return;
            } else if (cmd[plen] == '.' || cmd[plen] == ' ') {
                s_handlers[i].handler(cmd + plen + 1);
                return;
            }
        }
    }
    serial_cmd_log("ERROR", "unknown_command cmd=%s", cmd);
}

// --- nav ---
static void nav_handler(const char *args) {
    screensaver_stop();   // stop timer before screen delete to avoid dangling ss_canvas
    struct { const char *name; void (*fn)(); } screens[] = {
        {"main", []() { create_main_menu(false); }},
        {"system", []() { create_system_submenu(); }},
        {"bling", []() { create_bling_window(); }},
        {"wifi", []() { create_wifi_window(); }},
        {"callsign", []() { create_callsign_window(); }},
        {"achievements", []() { create_achievements_window(); }},
        {"crew", []() { create_crew_log_window(); }},
        {"comms", []() { create_comms_window(); }},
        {"game", []() { lunar_lander_start(); }},
        {"schedule", []() { extern void displaySchedule(); displaySchedule(); }},
        {"battery", []() { create_battery_window(); }},
        {"buzzer", []() { create_buzzer_window(); }},
        {"sd", []() { create_sd_card_window(); }},
        {"info", []() { create_system_info_window(); }},
        {"ota", []() { create_ota_window(); }},
        {"credits", []() { create_credits_window(); }},
        {"card", []() { create_badge_card_window(); }},
        {"checkin", []() { create_checkin_window(); }},
    };
    for (auto &s : screens) {
        if (strcmp(args, s.name) == 0) { s.fn(); serial_cmd_log("NAV", "screen=%s heap=%d", args, ESP.getFreeHeap()); return; }
    }
    serial_cmd_log("NAV", "error=unknown screen=%s", args);
}

// --- bling ---
static void bling_handler(const char *args) {
    if (strncmp(args, "set ", 4) == 0) { int m = atoi(args+4); bling_set_mode(m); serial_cmd_log("BLING", "mode=%d", m); }
    else if (strcmp(args, "off") == 0) { bling_set_mode(0); serial_cmd_log("BLING", "mode=0"); }
    else if (strcmp(args, "status") == 0) { serial_cmd_log("BLING", "mode=%d", bling_get_mode()); }
    else if (strcmp(args, "list") == 0) { serial_cmd_log("BLING", "0=off 1=rainbow 2=police 3=blink 4=chase 5=random 6=breathe 7=aurora 8=morse 9=comet 10=fire 11=sparkle"); }
    else serial_cmd_log("BLING", "error=unknown args=%s", args);
}

// --- audio ---
static void audio_handler(const char *args) {
    if (strcmp(args, "mute") == 0) { audio_set_mute(true); serial_cmd_log("AUDIO", "muted=1"); }
    else if (strcmp(args, "unmute") == 0) { audio_set_mute(false); serial_cmd_log("AUDIO", "muted=0"); }
    else if (strcmp(args, "status") == 0) { serial_cmd_log("AUDIO", "muted=%d", audio_is_muted()?1:0); }
    else serial_cmd_log("AUDIO", "error=unknown args=%s", args);
}

// --- callsign ---
static void callsign_handler(const char *args) {
    if (strcmp(args, "get") == 0) { serial_cmd_log("CALLSIGN", "name=%s", callsign_get()); }
    else if (strncmp(args, "set ", 4) == 0) { callsign_set(args+4); serial_cmd_log("CALLSIGN", "name=%s", callsign_get()); }
    else serial_cmd_log("CALLSIGN", "error=unknown args=%s", args);
}

// --- screensaver ---
static void screensaver_handler(const char *args) {
    if (strncmp(args, "mode ", 5) == 0) { screensaver_set_mode((ScreensaverMode)atoi(args+5)); serial_cmd_log("SS", "mode=%d", (int)screensaver_get_mode()); }
    else if (strcmp(args, "status") == 0) { serial_cmd_log("SS", "mode=%d", (int)screensaver_get_mode()); }
    else if (strcmp(args, "list") == 0) { serial_cmd_log("SS", "0=ad_astra 1=matrix 2=terminal 3=lava"); }
    else serial_cmd_log("SS", "error=unknown args=%s", args);
}

// --- achievements ---
static void ach_handler(const char *args) {
    if (strcmp(args, "status") == 0) { serial_cmd_log("ACH", "total=%d/%d", achievements_total(), ACH_COUNT); }
    else if (strcmp(args, "list") == 0) {
        serial_cmd_log("ACH", "total=%d/%d", achievements_total(), ACH_COUNT);
        for (int i = 0; i < ACH_COUNT; i++)
            Serial.printf("  %d: %s\n", i, achievement_unlocked(i) ? "UNLOCKED" : "locked");
    }
    else if (strncmp(args, "unlock ", 7) == 0) { achievement_unlock(atoi(args+7)); serial_cmd_log("ACH", "done"); }
    else serial_cmd_log("ACH", "error=unknown args=%s", args);
}

// --- ble ---
static void ble_handler(const char *args) {
    if (strcmp(args, "status") == 0) { serial_cmd_log("BLE", "nearby=%d total=%d", ble_presence_nearby_count(), ble_presence_total_count()); }
    else serial_cmd_log("BLE", "error=unknown args=%s", args);
}

// --- game ---
static void game_handler(const char *args) {
    if (strcmp(args, "state") == 0) {
        const GameState *g = game_get_state();
        if (g) {
            serial_cmd_log("GAME", "phase=%d mode=%d diff=%d score=%d fuel=%d x=%d y=%d",
                g->phase, g->mode, g->difficulty, g->score,
                (int)g->lander.fuel, (int)g->lander.x, (int)g->lander.y);
        } else {
            serial_cmd_log("GAME", "not_active");
        }
    }
    else if (strncmp(args, "start", 5) == 0) {
        int diff = 0;
        if (args[5] == ' ') diff = atoi(args + 6);
        if (diff < 0 || diff > 2) diff = 0;
        screensaver_stop();
        game_start_at_difficulty((uint8_t)diff);
        serial_cmd_log("GAME", "started diff=%d phase=%d heap=%d", diff, game_get_state()->phase, ESP.getFreeHeap());
    }
    else if (strcmp(args, "stop") == 0) { lunar_lander_stop(); serial_cmd_log("GAME", "stopped"); }
    else serial_cmd_log("GAME", "error=unknown args=%s", args);
}

void serial_cmd_init() {
    cmd_pos = 0;
    serial_cmd_register("sys", sys_handler, "heap, version, uptime, temp, diag, reboot");
    serial_cmd_register("test", test_handler, "stress <n>, idle <ms>");
    serial_cmd_register("help", help_handler, "[module] - list commands");
    serial_cmd_register("nav", nav_handler, "main,system,bling,wifi,callsign,achievements,crew,comms,game,schedule,battery,buzzer,sd,info,ota,credits,card,checkin");
    serial_cmd_register("bling", bling_handler, "set <n>, off, status, list");
    serial_cmd_register("audio", audio_handler, "mute, unmute, status");
    serial_cmd_register("callsign", callsign_handler, "get, set <name>");
    serial_cmd_register("screensaver", screensaver_handler, "mode <n>, status, list");
    serial_cmd_register("achievements", ach_handler, "status, list, unlock <id>");
    serial_cmd_register("ble", ble_handler, "status");
    serial_cmd_register("game", game_handler, "state, start [0-2], stop");
    serial_cmd_log("INIT", "bstp_v1 ready firmware=%s handlers=%d", BADGE_VERSION, s_handler_count);
}

void serial_cmd_poll() {
    s_loop_count++;
    uint32_t now = millis();
    if (now - s_loop_last >= 1000) {
        s_loop_rate = s_loop_count;
        s_loop_count = 0;
        s_loop_last = now;
    }
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            if (cmd_pos > 0) {
                cmd_buf[cmd_pos] = '\0';
                process_command(cmd_buf);
                cmd_pos = 0;
            }
        } else if (cmd_pos < (int)sizeof(cmd_buf) - 1) {
            cmd_buf[cmd_pos++] = c;
        }
    }
}

#endif // FF_SERIAL_TEST
