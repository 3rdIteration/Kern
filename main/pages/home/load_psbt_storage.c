// Load PSBT from SD Card — list .psbt files, let user select one, then hand
// off to the scan page for review and signing.

#include "load_psbt_storage.h"
#include "../../core/storage.h"
#include "../../ui/dialog.h"
#include "../scan/scan.h"
#include "../shared/storage_browser.h"
#include <lvgl.h>
#include <stdlib.h>
#include <string.h>

static void (*load_psbt_success_cb)(void) = NULL;

/* ---------- Load selected PSBT ---------- */

static void load_selected(int idx, const char *filename) {
  (void)idx;

  uint8_t *data = NULL;
  size_t len = 0;

  esp_err_t ret = storage_load_psbt(filename, &data, &len);
  if (ret == ESP_ERR_INVALID_SIZE) {
    dialog_show_error("PSBT file too large", NULL, 0);
    return;
  }
  if (ret != ESP_OK) {
    dialog_show_error("Failed to load PSBT file", NULL, 0);
    return;
  }

  storage_browser_hide();

  scan_page_create_with_psbt(lv_screen_active(), data, len,
                             load_psbt_success_cb);
  scan_page_show();
  free(data);
}

/* ---------- Display name ---------- */

static char *get_display_name(storage_location_t loc, const char *filename) {
  (void)loc;
  /* Strip the .psbt extension for a cleaner label */
  size_t len = strlen(filename);
  const char *ext = STORAGE_PSBT_EXT;
  size_t ext_len = strlen(ext);
  if (len > ext_len && strcmp(filename + len - ext_len, ext) == 0) {
    char *name = strndup(filename, len - ext_len);
    return name ? name : strdup(filename);
  }
  return strdup(filename);
}

/* ---------- Page lifecycle ---------- */

void load_psbt_storage_page_create(lv_obj_t *parent, void (*return_cb)(void),
                                   void (*success_cb)(void)) {
  if (!parent)
    return;

  load_psbt_success_cb = success_cb;

  storage_browser_config_t config = {
      .item_type_name = "PSBT",
      .location = STORAGE_SD,
      .list_files = storage_list_psbts,
      .delete_file = storage_delete_psbt,
      .get_display_name = get_display_name,
      .load_selected = load_selected,
      .return_cb = return_cb,
  };

  storage_browser_create(parent, &config);
}

void load_psbt_storage_page_show(void) { storage_browser_show(); }

void load_psbt_storage_page_hide(void) { storage_browser_hide(); }

void load_psbt_storage_page_destroy(void) {
  storage_browser_destroy();
  load_psbt_success_cb = NULL;
}
