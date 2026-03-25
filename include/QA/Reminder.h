#ifndef QA_REMINDER_H
#define QA_REMINDER_H

#include <stdint.h>

void reminder_set(const char* title, uint32_t met_seconds);
void reminder_check(uint32_t current_met);

#endif
