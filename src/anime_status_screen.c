#include <lvgl.h>

#include <zephyr/sys/util.h>

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
 * Skip animation converted to 96x32 1-bit frames for vertically mounted SSD1306 OLEDs.
 */

static lv_obj_t *dance_img;
static lv_timer_t *dance_timer;
static uint8_t dance_frame;

static void advance_dance(lv_timer_t *timer) {
    ARG_UNUSED(timer);
    dance_frame = (dance_frame + 1) % ANIME_FRAME_COUNT;
    lv_img_set_src(dance_img, &anime_dance_frames[dance_frame]);
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

    dance_img = img;
    dance_frame = 0;
    if (dance_timer == NULL) {
        dance_timer = lv_timer_create(advance_dance, 120, NULL);
    } else {
        lv_timer_reset(dance_timer);
    }

    return screen;
}
