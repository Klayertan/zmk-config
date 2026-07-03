#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <lvgl.h>
#include <zephyr/kernel.h>
#include <zmk/display.h>
#include <zmk/display/status_screen.h>

#if IS_ENABLED(CONFIG_SHIELD_CORNE_RIGHT) &&                                                  \
    (!IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL))
#include <dt-bindings/zmk/hid_usage.h>
#include <dt-bindings/zmk/hid_usage_pages.h>
#include <dt-bindings/zmk/modifiers.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/hid.h>
#endif

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

#if IS_ENABLED(CONFIG_SHIELD_CORNE_RIGHT) &&                                                  \
    (!IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL))
#define TYPED_TEXT_MAX 42

static lv_obj_t *typed_text_label;
static char typed_text[TYPED_TEXT_MAX + 1];
static bool typed_text_has_content;
static uint8_t tracked_explicit_modifiers;

struct typed_text_state {
    uint16_t usage_page;
    uint32_t keycode;
    uint8_t modifiers;
    bool pressed;
};

static struct typed_text_state typed_text_get_state(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev;

    if (eh == NULL || (ev = as_zmk_keycode_state_changed(eh)) == NULL) {
        return (struct typed_text_state){0};
    }

    return (struct typed_text_state){.usage_page = ev->usage_page,
                                    .keycode = ev->keycode,
                                    .modifiers = ev->implicit_modifiers |
                                                 ev->explicit_modifiers |
                                                 tracked_explicit_modifiers |
                                                 zmk_hid_get_explicit_mods(),
                                    .pressed = ev->state};
}

static uint8_t modifier_for_keycode(uint32_t keycode) {
    switch (keycode) {
    case HID_USAGE_KEY_KEYBOARD_LEFTCONTROL:
        return MOD_LCTL;
    case HID_USAGE_KEY_KEYBOARD_LEFTSHIFT:
        return MOD_LSFT;
    case HID_USAGE_KEY_KEYBOARD_LEFTALT:
        return MOD_LALT;
    case HID_USAGE_KEY_KEYBOARD_LEFT_GUI:
        return MOD_LGUI;
    case HID_USAGE_KEY_KEYBOARD_RIGHTCONTROL:
        return MOD_RCTL;
    case HID_USAGE_KEY_KEYBOARD_RIGHTSHIFT:
        return MOD_RSFT;
    case HID_USAGE_KEY_KEYBOARD_RIGHTALT:
        return MOD_RALT;
    case HID_USAGE_KEY_KEYBOARD_RIGHT_GUI:
        return MOD_RGUI;
    default:
        return 0;
    }
}

static bool is_shifted(uint8_t modifiers) { return (modifiers & (MOD_LSFT | MOD_RSFT)) != 0; }

static char printable_keycode(uint32_t keycode, uint8_t modifiers) {
    const bool shifted = is_shifted(modifiers);

    if (keycode >= HID_USAGE_KEY_KEYBOARD_A && keycode <= HID_USAGE_KEY_KEYBOARD_Z) {
        char letter = 'a' + (keycode - HID_USAGE_KEY_KEYBOARD_A);
        return shifted ? (letter - ('a' - 'A')) : letter;
    }

    if (keycode >= HID_USAGE_KEY_KEYBOARD_1_AND_EXCLAMATION &&
        keycode <= HID_USAGE_KEY_KEYBOARD_0_AND_RIGHT_PARENTHESIS) {
        static const char normal[] = "1234567890";
        static const char shifted_symbols[] = "!@#$%^&*()";
        uint8_t index = keycode - HID_USAGE_KEY_KEYBOARD_1_AND_EXCLAMATION;
        return shifted ? shifted_symbols[index] : normal[index];
    }

    switch (keycode) {
    case HID_USAGE_KEY_KEYBOARD_SPACEBAR:
    case HID_USAGE_KEY_KEYBOARD_TAB:
    case HID_USAGE_KEY_KEYBOARD_RETURN_ENTER:
    case HID_USAGE_KEY_KEYBOARD_RETURN:
        return ' ';
    case HID_USAGE_KEY_KEYBOARD_MINUS_AND_UNDERSCORE:
        return shifted ? '_' : '-';
    case HID_USAGE_KEY_KEYBOARD_EQUAL_AND_PLUS:
        return shifted ? '+' : '=';
    case HID_USAGE_KEY_KEYBOARD_LEFT_BRACKET_AND_LEFT_BRACE:
        return shifted ? '{' : '[';
    case HID_USAGE_KEY_KEYBOARD_RIGHT_BRACKET_AND_RIGHT_BRACE:
        return shifted ? '}' : ']';
    case HID_USAGE_KEY_KEYBOARD_BACKSLASH_AND_PIPE:
        return shifted ? '|' : '\\';
    case HID_USAGE_KEY_KEYBOARD_SEMICOLON_AND_COLON:
        return shifted ? ':' : ';';
    case HID_USAGE_KEY_KEYBOARD_APOSTROPHE_AND_QUOTE:
        return shifted ? '"' : '\'';
    case HID_USAGE_KEY_KEYBOARD_GRAVE_ACCENT_AND_TILDE:
        return shifted ? '~' : '`';
    case HID_USAGE_KEY_KEYBOARD_COMMA_AND_LESS_THAN:
        return shifted ? '<' : ',';
    case HID_USAGE_KEY_KEYBOARD_PERIOD_AND_GREATER_THAN:
        return shifted ? '>' : '.';
    case HID_USAGE_KEY_KEYBOARD_SLASH_AND_QUESTION_MARK:
        return shifted ? '?' : '/';
    default:
        return '\0';
    }
}

static void typed_text_refresh(void) {
    if (typed_text_label == NULL) {
        return;
    }

    lv_label_set_text(typed_text_label, typed_text_has_content ? typed_text : "type something...");
}

static void typed_text_clear(void) {
    typed_text[0] = '\0';
    typed_text_has_content = false;
    typed_text_refresh();
}

static void typed_text_backspace(void) {
    if (!typed_text_has_content) {
        return;
    }

    size_t len = strlen(typed_text);
    if (len > 0) {
        typed_text[len - 1] = '\0';
    }

    typed_text_has_content = typed_text[0] != '\0';
    typed_text_refresh();
}

static void typed_text_append(char c) {
    if (!typed_text_has_content) {
        typed_text[0] = '\0';
    }

    size_t len = strlen(typed_text);
    if (len >= TYPED_TEXT_MAX) {
        memmove(typed_text, typed_text + 1, TYPED_TEXT_MAX - 1);
        len = TYPED_TEXT_MAX - 1;
    }

    typed_text[len] = c;
    typed_text[len + 1] = '\0';
    typed_text_has_content = true;
    typed_text_refresh();
}

static void typed_text_update_cb(struct typed_text_state state) {
    if (state.usage_page != HID_USAGE_KEY) {
        return;
    }

    uint8_t modifier = modifier_for_keycode(state.keycode);
    if (modifier != 0) {
        if (state.pressed) {
            tracked_explicit_modifiers |= modifier;
        } else {
            tracked_explicit_modifiers &= ~modifier;
        }
        return;
    }

    if (!state.pressed) {
        return;
    }

    if (state.keycode == HID_USAGE_KEY_KEYBOARD_ESCAPE) {
        typed_text_clear();
        return;
    }

    if (state.keycode == HID_USAGE_KEY_KEYBOARD_DELETE_BACKSPACE) {
        typed_text_backspace();
        return;
    }

    char c = printable_keycode(state.keycode, state.modifiers);
    if (c != '\0') {
        typed_text_append(c);
    }
}

ZMK_DISPLAY_WIDGET_LISTENER(typed_text, struct typed_text_state, typed_text_update_cb,
                            typed_text_get_state)
ZMK_SUBSCRIPTION(typed_text, zmk_keycode_state_changed);
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

static lv_obj_t *create_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    return screen;
}

static void style_text(lv_obj_t *label) {
    lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_12, LV_PART_MAIN);
}

static lv_obj_t *create_left_dancer_screen(void) {
    lv_obj_t *screen = create_screen();

    lv_obj_t *img = lv_img_create(screen);
    lv_img_set_src(img, &anime_dance_frames[0]);
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
    anime_start_dance(img);

    return screen;
}

#if IS_ENABLED(CONFIG_SHIELD_CORNE_RIGHT) &&                                                  \
    (!IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL))
static lv_obj_t *create_right_typing_screen(void) {
    lv_obj_t *screen = create_screen();

    typed_text_label = lv_label_create(screen);
    lv_label_set_long_mode(typed_text_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(typed_text_label, 124);
    lv_obj_set_pos(typed_text_label, 2, 0);
    style_text(typed_text_label);
    typed_text_refresh();
    typed_text_init();

    return screen;
}
#endif

lv_obj_t *zmk_display_status_screen(void) {
#if IS_ENABLED(CONFIG_SHIELD_CORNE_RIGHT) &&                                                  \
    (!IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL))
    return create_right_typing_screen();
#else
    return create_left_dancer_screen();
#endif
}
