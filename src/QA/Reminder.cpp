#include "QA/Reminder.h"
#include "Game/LunarAudio.h"
#include <string.h>

static char s_title[40] = "";
static uint32_t s_remind_at = 0;
static bool s_active = false;
static bool s_fired = false;

void reminder_set(const char* title, uint32_t met_seconds) {
    strncpy(s_title, title, sizeof(s_title) - 1);
    s_title[sizeof(s_title) - 1] = '\0';
    s_remind_at = met_seconds;
    s_active = true;
    s_fired = false;
}

void reminder_check(uint32_t current_met) {
    if (!s_active || s_fired) return;
    if (current_met >= s_remind_at) {
        s_fired = true;
        audio_countdown(0);
    }
}
