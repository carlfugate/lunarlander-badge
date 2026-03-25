#include "QA/SerialCmd.h"

#ifdef FF_SERIAL_TEST

#include <Arduino.h>
#include <lvgl.h>
#include <stdarg.h>
#include <string.h>
#include "Hardware/BadgeVersion.h"
#include "QA/Menu.h"
#include "QA/Bling.hpp"
#include "QA/Callsign.h"
#include "QA/Achievements.h"
#include "QA/BlePresence.h"
#include "Game/LunarState.h"

static char cmd_buf[128];
static int cmd_pos = 0;

void serial_cmd_log(const char *tag, const char *fmt, ...) {
    Serial.printf("[%s] ", tag);
    va_list args;
    va_start(args, fmt);
    char buf[256];
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    Serial.println(buf);
}

static void cmd_heap() {
    serial_cmd_log("HEAP", "free=%d min=%d",
        ESP.getFreeHeap(), ESP.getMinFreeHeap());
}

static void cmd_nav(const char *screen) {
    if (strcmp(screen, "main") == 0) { create_main_menu(false); }
    else if (strcmp(screen, "system") == 0) { create_system_submenu(); }
    else if (strcmp(screen, "bling") == 0) { create_bling_window(); }
    else if (strcmp(screen, "callsign") == 0) { create_callsign_window(); }
    else if (strcmp(screen, "achievements") == 0) { create_achievements_window(); }
    else if (strcmp(screen, "crew") == 0) { create_crew_log_window(); }
    else if (strcmp(screen, "comms") == 0) { create_comms_window(); }
    else if (strcmp(screen, "game") == 0) { lunar_lander_start(); }
    else { serial_cmd_log("NAV", "error=unknown_screen screen=%s", screen); return; }
    serial_cmd_log("NAV", "screen=%s heap=%d", screen, ESP.getFreeHeap());
}

static void cmd_tap(int x, int y) {
    lv_obj_t *scr = lv_scr_act();
    if (scr) {
        lv_obj_send_event(scr, LV_EVENT_PRESSED, NULL);
        lv_obj_send_event(scr, LV_EVENT_CLICKED, NULL);
    }
    serial_cmd_log("TAP", "x=%d y=%d", x, y);
}

static void cmd_state() {
    serial_cmd_log("STATE", "heap=%d min_heap=%d uptime=%lu",
        ESP.getFreeHeap(), ESP.getMinFreeHeap(), millis());
}

static void cmd_stress(int cycles) {
    serial_cmd_log("STRESS", "starting cycles=%d", cycles);
    uint32_t start_heap = ESP.getFreeHeap();
    for (int i = 0; i < cycles; i++) {
        create_main_menu(false);
        lv_timer_handler();
        create_system_submenu();
        lv_timer_handler();
        create_main_menu(false);
        lv_timer_handler();
        if (i % 10 == 0) {
            serial_cmd_log("STRESS", "progress=%d/%d heap=%d", i, cycles, ESP.getFreeHeap());
        }
    }
    serial_cmd_log("STRESS", "complete cycles=%d heap_start=%d heap_end=%d delta=%d",
        cycles, start_heap, ESP.getFreeHeap(), (int)ESP.getFreeHeap() - (int)start_heap);
}

static void cmd_idle(int ms) {
    serial_cmd_log("IDLE", "starting ms=%d", ms);
    uint32_t start = millis();
    uint32_t start_heap = ESP.getFreeHeap();
    while (millis() - start < (uint32_t)ms) {
        lv_timer_handler();
        delay(5);  // OK here — this IS the main loop context
    }
    serial_cmd_log("IDLE", "complete ms=%d heap_start=%d heap_end=%d delta=%d",
        ms, start_heap, ESP.getFreeHeap(), (int)ESP.getFreeHeap() - (int)start_heap);
}

static void cmd_version() {
    serial_cmd_log("VERSION", "firmware=%s codename=%s", BADGE_VERSION, BADGE_CODE_NAME);
}

static void cmd_reboot() {
    serial_cmd_log("REBOOT", "rebooting");
    Serial.flush();
    ESP.restart();
}

static void process_command(char *cmd) {
    while (*cmd == ' ') cmd++;
    char *end = cmd + strlen(cmd) - 1;
    while (end > cmd && (*end == ' ' || *end == '\r' || *end == '\n')) *end-- = '\0';

    if (strlen(cmd) == 0) return;

    if (strcmp(cmd, "heap") == 0) { cmd_heap(); }
    else if (strcmp(cmd, "state") == 0) { cmd_state(); }
    else if (strcmp(cmd, "version") == 0) { cmd_version(); }
    else if (strcmp(cmd, "reboot") == 0) { cmd_reboot(); }
    else if (strncmp(cmd, "nav ", 4) == 0) { cmd_nav(cmd + 4); }
    else if (strncmp(cmd, "tap ", 4) == 0) {
        int x, y;
        if (sscanf(cmd + 4, "%d %d", &x, &y) == 2) cmd_tap(x, y);
        else serial_cmd_log("ERROR", "usage: tap <x> <y>");
    }
    else if (strncmp(cmd, "stress ", 7) == 0) { cmd_stress(atoi(cmd + 7)); }
    else if (strncmp(cmd, "idle ", 5) == 0) { cmd_idle(atoi(cmd + 5)); }
    else if (strcmp(cmd, "back") == 0) { create_main_menu(false); serial_cmd_log("NAV", "screen=main heap=%d", ESP.getFreeHeap()); }
    else if (strcmp(cmd, "help") == 0) {
        Serial.println("[HELP] Commands: heap, state, version, reboot, nav <screen>, tap <x> <y>, stress <n>, idle <ms>, back, help");
        Serial.println("[HELP] Screens: main, system, bling, callsign, achievements, crew, comms, game");
    }
    else { serial_cmd_log("ERROR", "unknown command: %s", cmd); }
}

void serial_cmd_init() {
    cmd_pos = 0;
    serial_cmd_log("INIT", "serial_test_harness ready firmware=%s", BADGE_VERSION);
}

void serial_cmd_poll() {
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
