#include <lvgl.h>

#include <zmk/display/status_screen.h>

#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif

#ifndef LV_ATTRIBUTE_LARGE_CONST
#define LV_ATTRIBUTE_LARGE_CONST
#endif

#ifndef LV_ATTRIBUTE_IMG_ANIME_DANCE
#define LV_ATTRIBUTE_IMG_ANIME_DANCE
#endif

#include "ansimuz_slide_oled.h"

/*
 * Dancing Girl Sprites by Ansimuz, CC0.
 * Source: https://opengameart.org/content/dancing-girl-sprites
 * Snap animation converted to upright 32x32 1-bit frames for SSD1306 OLEDs.
 */

static void set_dance_frame(void *obj, int32_t value) {
    lv_img_set_src((lv_obj_t *)obj, &anime_dance_frames[value % ANIME_FRAME_COUNT]);
}

static void start_dance(lv_obj_t *img) {
    lv_anim_t dance;

    lv_anim_init(&dance);
    lv_anim_set_var(&dance, img);
    lv_anim_set_exec_cb(&dance, set_dance_frame);
    lv_anim_set_values(&dance, 0, ANIME_FRAME_COUNT - 1);
    lv_anim_set_path_cb(&dance, lv_anim_path_step);
    lv_anim_set_time(&dance, 560);
    lv_anim_set_repeat_count(&dance, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&dance);
}

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_width(screen, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(screen, 0, LV_PART_MAIN);

    lv_obj_t *img = lv_img_create(screen);
    lv_img_set_src(img, &anime_dance_frames[0]);
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
    start_dance(img);

    return screen;
}
