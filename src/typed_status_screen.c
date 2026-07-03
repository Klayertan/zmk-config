#include <ctype.h>
#include <string.h>

#include <lvgl.h>

#include <zephyr/sys/util.h>

#include <dt-bindings/zmk/hid_usage.h>
#include <dt-bindings/zmk/hid_usage_pages.h>
#include <dt-bindings/zmk/modifiers.h>
#include <zmk/display.h>
#include <zmk/display/status_screen.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/keys.h>

#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif

#define PHYS_W 32
#define PHYS_H 128
#define LOGICAL_W 128
#define LOGICAL_H 32
#define IMAGE_PALETTE_BYTES 8
#define IMAGE_DATA_BYTES (LOGICAL_W * LOGICAL_H / 8)
#define WORD_MAX_CHARS 10
#define TYPED_TEXT_HAS_KEY_EVENTS                                                                 \
    (!IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL))

struct typed_text_state {
    char text[WORD_MAX_CHARS + 1];
};

static lv_obj_t *typed_img;
static char current_word[WORD_MAX_CHARS + 1] = "";
static uint8_t current_len;
static uint8_t active_mods;

static uint8_t typed_image_map[IMAGE_PALETTE_BYTES + IMAGE_DATA_BYTES] = {
    0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff,
};

static lv_img_dsc_t typed_image = {
    .header.cf = LV_IMG_CF_INDEXED_1BIT,
    .header.always_zero = 0,
    .header.reserved = 0,
    .header.w = LOGICAL_W,
    .header.h = LOGICAL_H,
    .data_size = sizeof(typed_image_map),
    .data = typed_image_map,
};

static const uint8_t glyph_blank[7] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t glyph_unknown[7] = {0x0e, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04};

static const uint8_t *glyph_for_char(char c) {
    static const uint8_t letters[26][7] = {
        {0x0e, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11}, // A
        {0x1e, 0x11, 0x11, 0x1e, 0x11, 0x11, 0x1e}, // B
        {0x0e, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0e}, // C
        {0x1e, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1e}, // D
        {0x1f, 0x10, 0x10, 0x1e, 0x10, 0x10, 0x1f}, // E
        {0x1f, 0x10, 0x10, 0x1e, 0x10, 0x10, 0x10}, // F
        {0x0e, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0f}, // G
        {0x11, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11}, // H
        {0x0e, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0e}, // I
        {0x07, 0x02, 0x02, 0x02, 0x12, 0x12, 0x0c}, // J
        {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}, // K
        {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1f}, // L
        {0x11, 0x1b, 0x15, 0x15, 0x11, 0x11, 0x11}, // M
        {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11}, // N
        {0x0e, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e}, // O
        {0x1e, 0x11, 0x11, 0x1e, 0x10, 0x10, 0x10}, // P
        {0x0e, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0d}, // Q
        {0x1e, 0x11, 0x11, 0x1e, 0x14, 0x12, 0x11}, // R
        {0x0f, 0x10, 0x10, 0x0e, 0x01, 0x01, 0x1e}, // S
        {0x1f, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}, // T
        {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e}, // U
        {0x11, 0x11, 0x11, 0x11, 0x11, 0x0a, 0x04}, // V
        {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0a}, // W
        {0x11, 0x11, 0x0a, 0x04, 0x0a, 0x11, 0x11}, // X
        {0x11, 0x11, 0x0a, 0x04, 0x04, 0x04, 0x04}, // Y
        {0x1f, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1f}, // Z
    };
    static const uint8_t numbers[10][7] = {
        {0x0e, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0e}, // 0
        {0x04, 0x0c, 0x14, 0x04, 0x04, 0x04, 0x1f}, // 1
        {0x0e, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1f}, // 2
        {0x1e, 0x01, 0x01, 0x0e, 0x01, 0x01, 0x1e}, // 3
        {0x02, 0x06, 0x0a, 0x12, 0x1f, 0x02, 0x02}, // 4
        {0x1f, 0x10, 0x10, 0x1e, 0x01, 0x01, 0x1e}, // 5
        {0x0e, 0x10, 0x10, 0x1e, 0x11, 0x11, 0x0e}, // 6
        {0x1f, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}, // 7
        {0x0e, 0x11, 0x11, 0x0e, 0x11, 0x11, 0x0e}, // 8
        {0x0e, 0x11, 0x11, 0x0f, 0x01, 0x01, 0x0e}, // 9
    };
    static const uint8_t dash[7] = {0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00};
    static const uint8_t underscore[7] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f};
    static const uint8_t dot[7] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x0c};
    static const uint8_t bang[7] = {0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04};

    c = toupper((unsigned char)c);
    if (c >= 'A' && c <= 'Z') {
        return letters[c - 'A'];
    }
    if (c >= '0' && c <= '9') {
        return numbers[c - '0'];
    }

    switch (c) {
    case ' ':
        return glyph_blank;
    case '-':
        return dash;
    case '_':
        return underscore;
    case '.':
        return dot;
    case '!':
        return bang;
    default:
        return glyph_unknown;
    }
}

static void set_logical_pixel(uint8_t x, uint8_t y) {
    if (x >= LOGICAL_W || y >= LOGICAL_H) {
        return;
    }

    size_t byte_index = IMAGE_PALETTE_BYTES + (y * (LOGICAL_W / 8)) + (x / 8);
    typed_image_map[byte_index] |= BIT(7 - (x % 8));
}

static void set_physical_pixel(uint8_t x, uint8_t y) {
    if (x >= PHYS_W || y >= PHYS_H) {
        return;
    }

    set_logical_pixel(LOGICAL_W - 1 - y, x);
}

static void draw_glyph_rotated(char c, uint8_t x, uint8_t y, uint8_t scale) {
    const uint8_t *glyph = glyph_for_char(c);

    for (uint8_t row = 0; row < 7; row++) {
        for (uint8_t col = 0; col < 5; col++) {
            if ((glyph[row] & BIT(4 - col)) == 0) {
                continue;
            }

            for (uint8_t dy = 0; dy < scale; dy++) {
                for (uint8_t dx = 0; dx < scale; dx++) {
                    set_physical_pixel(x + row * scale + dy, y + col * scale + dx);
                }
            }
        }
    }
}

static const char *display_text(void) { return current_len > 0 ? current_word : "TYPE"; }

static void render_text_image(const char *text) {
    memset(typed_image_map + IMAGE_PALETTE_BYTES, 0, IMAGE_DATA_BYTES);

    uint8_t len = MIN(strlen(text), WORD_MAX_CHARS);
    uint8_t scale = 2;
    uint8_t char_w = 5 * scale;
    uint8_t char_h = 7 * scale;
    uint8_t gap = 1;
    uint8_t advance = char_w + gap;
    uint8_t block_w = len * advance - gap;
    uint8_t x = (PHYS_W - char_h) / 2;
    uint8_t y = block_w < PHYS_H ? (PHYS_H - block_w) / 2 : 0;

    for (uint8_t i = 0; i < len; i++) {
        draw_glyph_rotated(text[i], x, y + i * advance, scale);
    }
}

static bool keycode_is_mod(uint32_t keycode) {
    return keycode >= HID_USAGE_KEY_KEYBOARD_LEFTCONTROL &&
           keycode <= HID_USAGE_KEY_KEYBOARD_RIGHT_GUI;
}

static uint8_t mod_mask_for_keycode(uint32_t keycode) {
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

static void update_active_mods(uint32_t keycode, bool pressed) {
    uint8_t mask = mod_mask_for_keycode(keycode);

    if (mask == 0) {
        return;
    }

    if (pressed) {
        active_mods |= mask;
    } else {
        active_mods &= ~mask;
    }
}

static char shifted_digit(uint32_t keycode) {
    static const char shifted[] = {'!', '@', '#', '$', '%', '^', '&', '*', '(', ')'};

    if (keycode == HID_USAGE_KEY_KEYBOARD_0_AND_RIGHT_PARENTHESIS) {
        return shifted[9];
    }

    return shifted[keycode - HID_USAGE_KEY_KEYBOARD_1_AND_EXCLAMATION];
}

static char char_from_keycode(uint32_t keycode, bool shifted) {
    if (keycode >= HID_USAGE_KEY_KEYBOARD_A && keycode <= HID_USAGE_KEY_KEYBOARD_Z) {
        return 'a' + (keycode - HID_USAGE_KEY_KEYBOARD_A);
    }

    if (keycode >= HID_USAGE_KEY_KEYBOARD_1_AND_EXCLAMATION &&
        keycode <= HID_USAGE_KEY_KEYBOARD_0_AND_RIGHT_PARENTHESIS) {
        if (shifted) {
            return shifted_digit(keycode);
        }
        return keycode == HID_USAGE_KEY_KEYBOARD_0_AND_RIGHT_PARENTHESIS
                   ? '0'
                   : '1' + (keycode - HID_USAGE_KEY_KEYBOARD_1_AND_EXCLAMATION);
    }

    switch (keycode) {
    case HID_USAGE_KEY_KEYBOARD_MINUS_AND_UNDERSCORE:
        return shifted ? '_' : '-';
    case HID_USAGE_KEY_KEYBOARD_PERIOD_AND_GREATER_THAN:
        return '.';
    default:
        return '\0';
    }
}

static void append_char(char c) {
    if (current_len == WORD_MAX_CHARS) {
        memmove(current_word, current_word + 1, WORD_MAX_CHARS - 1);
        current_len--;
    }

    current_word[current_len++] = c;
    current_word[current_len] = '\0';
}

static void backspace_char(void) {
    if (current_len == 0) {
        return;
    }

    current_word[--current_len] = '\0';
}

static struct typed_text_state typed_text_get_state(const zmk_event_t *eh) {
#if TYPED_TEXT_HAS_KEY_EVENTS
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);

    if (ev != NULL && ev->usage_page == HID_USAGE_KEY) {
        if (keycode_is_mod(ev->keycode)) {
            update_active_mods(ev->keycode, ev->state);
        } else if (ev->state) {
            uint8_t non_text_mods = active_mods & ~(MOD_LSFT | MOD_RSFT);
            bool shifted = (active_mods & (MOD_LSFT | MOD_RSFT)) != 0 ||
                           (ev->implicit_modifiers & (MOD_LSFT | MOD_RSFT)) != 0;

            if (non_text_mods == 0) {
                switch (ev->keycode) {
                case HID_USAGE_KEY_KEYBOARD_DELETE_BACKSPACE:
                    backspace_char();
                    break;
                case HID_USAGE_KEY_KEYBOARD_SPACEBAR:
                    append_char(' ');
                    break;
                case HID_USAGE_KEY_KEYBOARD_RETURN_ENTER:
                    current_len = 0;
                    current_word[0] = '\0';
                    break;
                default: {
                    char c = char_from_keycode(ev->keycode, shifted);
                    if (c != '\0') {
                        append_char(c);
                    }
                    break;
                }
                }
            }
        }
    }
#else
    ARG_UNUSED(eh);
#endif

    struct typed_text_state state = {};
    strncpy(state.text, display_text(), sizeof(state.text) - 1);
    return state;
}

static void typed_text_update_cb(struct typed_text_state state) {
    if (typed_img == NULL) {
        return;
    }

    render_text_image(state.text);
    lv_img_set_src(typed_img, &typed_image);
    lv_obj_invalidate(typed_img);
}

ZMK_DISPLAY_WIDGET_LISTENER(typed_text_status, struct typed_text_state, typed_text_update_cb,
                            typed_text_get_state)
#if TYPED_TEXT_HAS_KEY_EVENTS
ZMK_SUBSCRIPTION(typed_text_status, zmk_keycode_state_changed);
#endif

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_width(screen, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(screen, 0, LV_PART_MAIN);

    typed_img = lv_img_create(screen);
    lv_obj_align(typed_img, LV_ALIGN_CENTER, 0, 0);

#if TYPED_TEXT_HAS_KEY_EVENTS
    typed_text_status_init();
#else
    render_text_image(display_text());
    lv_img_set_src(typed_img, &typed_image);
#endif

    return screen;
}
