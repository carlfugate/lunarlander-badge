#include "QA/ErrorHandler.h"
#include <lvgl.h>

static lv_obj_t *alarm_overlay = NULL;

void show_1202_alarm(const char *detail) {
    lv_obj_t *scr = lv_scr_act();
    if (alarm_overlay) { lv_obj_del(alarm_overlay); alarm_overlay = NULL; }
    alarm_overlay = lv_obj_create(scr);
    lv_obj_set_size(alarm_overlay, 280, 100);
    lv_obj_align(alarm_overlay, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(alarm_overlay, lv_color_hex(0x1a0a00), 0);
    lv_obj_set_style_border_color(alarm_overlay, lv_color_hex(0xffab00), 0);
    lv_obj_set_style_border_width(alarm_overlay, 2, 0);
    lv_obj_set_style_radius(alarm_overlay, 4, 0);
    lv_obj_set_style_pad_all(alarm_overlay, 8, 0);

    lv_obj_t *title = lv_label_create(alarm_overlay);
    lv_label_set_text(title, "1202 ALARM");
    lv_obj_set_style_text_color(title, lv_color_hex(0xffab00), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t *msg = lv_label_create(alarm_overlay);
    lv_label_set_text(msg, detail);
    lv_obj_set_style_text_color(msg, lv_color_hex(0x888888), 0);
    lv_obj_set_width(msg, 260);
    lv_label_set_long_mode(msg, LV_LABEL_LONG_WRAP);
    lv_obj_align(msg, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_timer_t *t = lv_timer_create([](lv_timer_t *timer) {
        if (alarm_overlay) { lv_obj_del(alarm_overlay); alarm_overlay = NULL; }
        lv_timer_del(timer);
    }, 5000, NULL);
    lv_timer_set_repeat_count(t, 1);
}
