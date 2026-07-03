#include <stdint.h>
#include <stdio.h>

#include <lvgl.h>
#include <zephyr/kernel.h>
#include <zmk/display.h>
#include <zmk/display/status_screen.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/position_state_changed.h>

#include <zmk/display/widgets/layer_status.h>
#include <zmk/display/widgets/output_status.h>
#include <zmk/display/widgets/peripheral_status.h>

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

#define KEY_GRID_COUNT 42

#if IS_ENABLED(CONFIG_SHIELD_CORNE_RIGHT)

#if IS_ENABLED(CONFIG_ZMK_WIDGET_OUTPUT_STATUS)
static struct zmk_widget_output_status output_status_widget;
#endif

#if IS_ENABLED(CONFIG_ZMK_WIDGET_PERIPHERAL_STATUS)
static struct zmk_widget_peripheral_status peripheral_status_widget;
#endif

#if IS_ENABLED(CONFIG_ZMK_WIDGET_LAYER_STATUS)
static struct zmk_widget_layer_status layer_status_widget;
#endif

static lv_obj_t *position_label;
static lv_obj_t *keycode_label;
static lv_obj_t *key_grid[KEY_GRID_COUNT];
static int32_t active_position = -1;

struct keypress_status_state {
    uint8_t source;
    uint32_t position;
    bool pressed;
};

struct keycode_status_state {
    uint16_t usage_page;
    uint32_t keycode;
    bool pressed;
};

static struct keypress_status_state keypress_status_get_state(const zmk_event_t *eh) {
    const struct zmk_position_state_changed *ev;

    if (eh == NULL || (ev = as_zmk_position_state_changed(eh)) == NULL) {
        return (struct keypress_status_state){.position = UINT32_MAX};
    }

    return (struct keypress_status_state){
        .source = ev->source, .position = ev->position, .pressed = ev->state};
}

static const char *position_source_label(uint8_t source) {
    return source == ZMK_POSITION_STATE_CHANGE_SOURCE_LOCAL ? "R" : "L";
}

static void set_grid_active(uint32_t position, bool active) {
    if (position >= KEY_GRID_COUNT || key_grid[position] == NULL) {
        return;
    }

    lv_obj_set_style_bg_color(key_grid[position], active ? lv_color_white() : lv_color_black(),
                              LV_PART_MAIN);
}

static void keypress_status_update_cb(struct keypress_status_state state) {
    if (position_label == NULL) {
        return;
    }

    if (state.position == UINT32_MAX) {
        lv_label_set_text(position_label, "POS --");
        return;
    }

    if (active_position >= 0 && active_position != (int32_t)state.position) {
        set_grid_active(active_position, false);
    }

    if (state.pressed) {
        active_position = state.position;
        set_grid_active(state.position, true);
    } else if (active_position == (int32_t)state.position) {
        set_grid_active(state.position, false);
        active_position = -1;
    }

    char text[16];
    snprintf(text, sizeof(text), "%s%02u %s", position_source_label(state.source),
             (uint8_t)state.position, state.pressed ? "DN" : "UP");
    lv_label_set_text(position_label, text);
}

ZMK_DISPLAY_WIDGET_LISTENER(keypress_status, struct keypress_status_state,
                            keypress_status_update_cb, keypress_status_get_state)
ZMK_SUBSCRIPTION(keypress_status, zmk_position_state_changed);

static struct keycode_status_state keycode_status_get_state(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev;

    if (eh == NULL || (ev = as_zmk_keycode_state_changed(eh)) == NULL) {
        return (struct keycode_status_state){0};
    }

    return (struct keycode_status_state){
        .usage_page = ev->usage_page, .keycode = ev->keycode, .pressed = ev->state};
}

static const char *keycode_name(uint16_t usage_page, uint32_t keycode) {
    if (usage_page != 0x07) {
        return "MEDIA";
    }

    if (keycode >= 4 && keycode <= 29) {
        static char letter[2];
        letter[0] = 'A' + (keycode - 4);
        letter[1] = '\0';
        return letter;
    }

    static char code[8];

    switch (keycode) {
    case 30:
        return "1";
    case 31:
        return "2";
    case 32:
        return "3";
    case 33:
        return "4";
    case 34:
        return "5";
    case 35:
        return "6";
    case 36:
        return "7";
    case 37:
        return "8";
    case 38:
        return "9";
    case 39:
        return "0";
    case 40:
        return "ENTER";
    case 41:
        return "ESC";
    case 42:
        return "BSPC";
    case 43:
        return "TAB";
    case 44:
        return "SPACE";
    case 45:
        return "-";
    case 46:
        return "=";
    case 47:
        return "[";
    case 48:
        return "]";
    case 49:
        return "\\";
    case 51:
        return ";";
    case 52:
        return "'";
    case 53:
        return "`";
    case 54:
        return ",";
    case 55:
        return ".";
    case 56:
        return "/";
    default:
        snprintf(code, sizeof(code), "%u", (uint8_t)keycode);
        return code;
    }
}

static void keycode_status_update_cb(struct keycode_status_state state) {
    if (keycode_label == NULL || !state.pressed) {
        return;
    }

    char text[16];
    snprintf(text, sizeof(text), "KEY %s", keycode_name(state.usage_page, state.keycode));
    lv_label_set_text(keycode_label, text);
}

ZMK_DISPLAY_WIDGET_LISTENER(keycode_status, struct keycode_status_state, keycode_status_update_cb,
                            keycode_status_get_state)
ZMK_SUBSCRIPTION(keycode_status, zmk_keycode_state_changed);

#endif

static void anime_set_frame(void *obj, int32_t value) {
    lv_img_set_src((lv_obj_t *)obj, &anime_dance_frames[value % ANIME_FRAME_COUNT]);
}

static void anime_set_translate_x(void *obj, int32_t value) {
    lv_obj_set_style_translate_x((lv_obj_t *)obj, value, LV_PART_MAIN);
}

static void anime_set_translate_y(void *obj, int32_t value) {
    lv_obj_set_style_translate_y((lv_obj_t *)obj, value, LV_PART_MAIN);
}

static void anime_start_dance(lv_obj_t *img) {
    lv_anim_t frame_anim;
    lv_anim_init(&frame_anim);
    lv_anim_set_var(&frame_anim, img);
    lv_anim_set_exec_cb(&frame_anim, anime_set_frame);
    lv_anim_set_values(&frame_anim, 0, ANIME_FRAME_COUNT - 1);
    lv_anim_set_path_cb(&frame_anim, lv_anim_path_step);
    lv_anim_set_time(&frame_anim, 720);
    lv_anim_set_repeat_count(&frame_anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&frame_anim);

    lv_anim_t sway;
    lv_anim_init(&sway);
    lv_anim_set_var(&sway, img);
    lv_anim_set_exec_cb(&sway, anime_set_translate_x);
    lv_anim_set_values(&sway, -1, 1);
    lv_anim_set_time(&sway, 360);
    lv_anim_set_playback_time(&sway, 360);
    lv_anim_set_repeat_count(&sway, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&sway);

    lv_anim_t bounce;
    lv_anim_init(&bounce);
    lv_anim_set_var(&bounce, img);
    lv_anim_set_exec_cb(&bounce, anime_set_translate_y);
    lv_anim_set_values(&bounce, -1, 1);
    lv_anim_set_time(&bounce, 240);
    lv_anim_set_playback_time(&bounce, 240);
    lv_anim_set_repeat_count(&bounce, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_delay(&bounce, 80);
    lv_anim_start(&bounce);
}

static lv_obj_t *create_label(lv_obj_t *parent, const char *text, lv_coord_t x, lv_coord_t y) {
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_12, LV_PART_MAIN);
    return label;
}

static lv_obj_t *create_left_dancer_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);

    lv_obj_t *img = lv_img_create(screen);
    lv_img_set_src(img, &anime_dance_frames[0]);
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
    anime_start_dance(img);

    return screen;
}

#if IS_ENABLED(CONFIG_SHIELD_CORNE_RIGHT)

static void create_key_grid_cell(lv_obj_t *screen, uint8_t index, lv_coord_t x, lv_coord_t y) {
    key_grid[index] = lv_obj_create(screen);
    lv_obj_clear_flag(key_grid[index], LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(key_grid[index], 4, 4);
    lv_obj_set_pos(key_grid[index], x, y);
    lv_obj_set_style_radius(key_grid[index], 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(key_grid[index], lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_color(key_grid[index], lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_width(key_grid[index], 1, LV_PART_MAIN);
}

static void create_key_grid(lv_obj_t *screen) {
    for (uint8_t row = 0; row < 3; row++) {
        for (uint8_t col = 0; col < 12; col++) {
            uint8_t index = (row * 12) + col;
            create_key_grid_cell(screen, index, 64 + (col * 5), 1 + (row * 6));
        }
    }

    for (uint8_t thumb = 0; thumb < 6; thumb++) {
        create_key_grid_cell(screen, 36 + thumb, 79 + (thumb * 5), 23);
    }
}

static void style_status_widget(lv_obj_t *obj) {
    lv_obj_set_style_text_color(obj, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN);
}

static lv_obj_t *create_right_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);

#if IS_ENABLED(CONFIG_ZMK_WIDGET_OUTPUT_STATUS)
    zmk_widget_output_status_init(&output_status_widget, screen);
    lv_obj_t *output_status = zmk_widget_output_status_obj(&output_status_widget);
    lv_obj_align(output_status, LV_ALIGN_TOP_LEFT, 0, 0);
    style_status_widget(output_status);
#endif

#if IS_ENABLED(CONFIG_ZMK_WIDGET_PERIPHERAL_STATUS)
    zmk_widget_peripheral_status_init(&peripheral_status_widget, screen);
    lv_obj_t *peripheral_status = zmk_widget_peripheral_status_obj(&peripheral_status_widget);
    lv_obj_align(peripheral_status, LV_ALIGN_TOP_LEFT, 0, 0);
    style_status_widget(peripheral_status);
#endif

#if IS_ENABLED(CONFIG_ZMK_WIDGET_LAYER_STATUS)
    zmk_widget_layer_status_init(&layer_status_widget, screen);
    lv_obj_t *layer_status = zmk_widget_layer_status_obj(&layer_status_widget);
    lv_obj_align(layer_status, LV_ALIGN_TOP_LEFT, 40, 0);
    style_status_widget(layer_status);
#endif

    position_label = create_label(screen, "POS --", 0, 12);
    keycode_label = create_label(screen, "KEY --", 0, 22);
    create_key_grid(screen);

    keypress_status_init();
    keycode_status_init();

    return screen;
}

#endif

lv_obj_t *zmk_display_status_screen(void) {
#if IS_ENABLED(CONFIG_SHIELD_CORNE_RIGHT)
    return create_right_status_screen();
#else
    return create_left_dancer_screen();
#endif
}
