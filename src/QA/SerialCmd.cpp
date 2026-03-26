#include "QA/SerialCmd.h"

#ifdef FF_SERIAL_TEST

#include <Arduino.h>
#include <lvgl.h>
#include <string.h>
#include <stdarg.h>
#include "Includes.h"

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

void serial_cmd_init() {
    cmd_pos = 0;
    serial_cmd_register("sys", sys_handler, "heap, version, uptime, temp, diag, reboot");
    serial_cmd_register("test", test_handler, "stress <n>, idle <ms>");
    serial_cmd_register("help", help_handler, "[module] - list commands");
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
