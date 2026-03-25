#ifndef QA_SCREENSAVER_H
#define QA_SCREENSAVER_H

#include <stdint.h>

enum ScreensaverMode {
    SS_MODE_AD_ASTRA = 0,  // Space story (default)
    SS_MODE_MATRIX,        // Matrix rain
    SS_MODE_TERMINAL,      // Terminal boot
    SS_MODE_LAVA,          // Lava lamp
    SS_MODE_COUNT
};

void screensaver_set_mode(ScreensaverMode mode);
ScreensaverMode screensaver_get_mode();
void screensaver_reset_timer();
void screensaver_start_timer();
void screensaver_stop();

#endif
