// About Page

#include "about.h"
#include "../../ui/assets/kern_logo_lvgl.h"
#include "../../ui/theme.h"
#include <esp_app_desc.h>
#include <lvgl.h>
#include <string.h>

static lv_obj_t *about_screen = NULL;
static void (*return_callback)(void) = NULL;

static void about_screen_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if ((code == LV_EVENT_CLICKED || code == LV_EVENT_PRESSED) && return_callback)
    return_callback();
}

void about_page_create(lv_obj_t *parent, void (*return_cb)(void)) {
  if (!parent)
    return;

  return_callback = return_cb;

  int32_t pad = theme_get_default_padding();
  int32_t scr_w = theme_get_screen_width();
  int32_t scr_h = theme_get_screen_height();
  int32_t font_h = lv_font_get_line_height(theme_font_small());
  bool landscape = scr_w > scr_h;

  about_screen = theme_create_page_container(parent);

  // Full-screen touch layer for "tap to return"
  lv_obj_t *touch = lv_obj_create(about_screen);
  lv_obj_remove_style_all(touch);
  lv_obj_set_size(touch, LV_PCT(100), LV_PCT(100));
  lv_obj_add_flag(touch, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(touch, about_screen_event_cb, LV_EVENT_CLICKED, NULL);

  // Title pinned at top
  theme_create_page_title(about_screen, "About");

  // Footer pinned at bottom
  lv_obj_t *footer = theme_create_label(about_screen, "Tap to return", true);
  lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -pad);

  // Pre-compute shared content
  const esp_app_desc_t *app_desc = esp_app_get_description();
  char ver_text[48];
  snprintf(ver_text, sizeof(ver_text), "Version: %s", app_desc->version);
  const char *qr_data = "https://github.com/odudex/Kern";

  // Body region between title and footer
  int32_t body_h = scr_h - 2 * (pad + font_h + pad);
  lv_obj_t *body = lv_obj_create(about_screen);
  lv_obj_remove_style_all(body);
  lv_obj_set_size(body, LV_PCT(100), body_h);
  lv_obj_align(body, LV_ALIGN_CENTER, 0, 0);
  lv_obj_clear_flag(body, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

  if (landscape) {
    // Side-by-side layout for landscape screens: logo+version on left, QR on
    // right
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(body, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    // Left column: logo + version text
    lv_obj_t *left_col = lv_obj_create(body);
    lv_obj_remove_style_all(left_col);
    lv_obj_clear_flag(left_col, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(left_col, LV_PCT(50), body_h);
    lv_obj_set_flex_flow(left_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(left_col, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    kern_logo_with_text_inline(left_col);
    theme_create_label(left_col, ver_text, true);

    // Right column: QR code sized to fit available height
    lv_obj_t *right_col = lv_obj_create(body);
    lv_obj_remove_style_all(right_col);
    lv_obj_clear_flag(right_col, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(right_col, LV_PCT(50), body_h);
    lv_obj_set_flex_flow(right_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(right_col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    int32_t qr_size = body_h * 80 / 100;
    lv_obj_t *qr = lv_qrcode_create(right_col);
    lv_qrcode_set_size(qr, qr_size);
    lv_qrcode_update(qr, qr_data, strlen(qr_data));
    lv_obj_set_style_border_color(qr, lv_color_white(), 0);
    lv_obj_set_style_border_width(qr, 10, 0);
  } else {
    // Vertical stacked layout for portrait / square screens
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(body, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    kern_logo_with_text_inline(body);
    theme_create_label(body, ver_text, true);

    int32_t min_dim = LV_MIN(scr_w, scr_h);
    lv_obj_t *qr = lv_qrcode_create(body);
    lv_qrcode_set_size(qr, min_dim * 25 / 72); // 250 @ 720
    lv_qrcode_update(qr, qr_data, strlen(qr_data));
    lv_obj_set_style_border_color(qr, lv_color_white(), 0);
    lv_obj_set_style_border_width(qr, 10, 0);
  }
}

void about_page_show(void) {
  if (about_screen)
    lv_obj_clear_flag(about_screen, LV_OBJ_FLAG_HIDDEN);
}

void about_page_hide(void) {
  if (about_screen)
    lv_obj_add_flag(about_screen, LV_OBJ_FLAG_HIDDEN);
}

void about_page_destroy(void) {
  if (about_screen) {
    lv_obj_del(about_screen);
    about_screen = NULL;
  }
  return_callback = NULL;
}
