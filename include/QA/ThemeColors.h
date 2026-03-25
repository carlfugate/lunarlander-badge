#ifndef QA_THEME_COLORS_H
#define QA_THEME_COLORS_H

#include <lvgl.h>

// NASA-style color language
#define THEME_BG        lv_color_hex(0x0a0a0f)
#define THEME_NOMINAL   lv_color_hex(0x00e5ff)  // cyan - info/nominal
#define THEME_GO        lv_color_hex(0x00c853)  // green - success/GO
#define THEME_CAUTION   lv_color_hex(0xffab00)  // amber - warning
#define THEME_CRITICAL  lv_color_hex(0xff1744)  // red - critical/failure
#define THEME_DIM       lv_color_hex(0x888888)  // gray - inactive
#define THEME_TEXT      lv_color_hex(0xcccccc)  // light gray - body text

#endif
