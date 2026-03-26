#ifndef QA_BLING_HPP
#define QA_BLING_HPP

#include <lvgl.h>

// Create the Bling window (RGB/LED effects menu)
void create_bling_window();

// Stop bling animation and clear LEDs (safe to call from any context)
void bling_stop_animation();

void bling_set_mode(int mode);
int bling_get_mode();

#endif // QA_BLING_HPP
