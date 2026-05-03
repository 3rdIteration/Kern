#ifndef SD_CARD_H
#define SD_CARD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SD_CARD_MOUNT_POINT "/sdcard"

/**
 * @brief SD card hot-swap event type.
 */
typedef enum {
  SD_CARD_EVT_INSERTED, /**< A card was mounted successfully. */
  SD_CARD_EVT_REMOVED,  /**< A previously-mounted card was detected as removed.
                         */
} sd_card_event_t;

/**
 * @brief Callback invoked from the polling task when mount state changes.
 *
 * The callback runs in the sd_poll FreeRTOS task context, not in the LVGL
 * task.  If you need to update LVGL widgets, schedule the work with
 * lv_async_call().
 */
typedef void (*sd_card_event_cb_t)(sd_card_event_t event);

esp_err_t sd_card_init(void);
esp_err_t sd_card_deinit(void);
bool sd_card_is_mounted(void);

/**
 * @brief Register a callback for card insertion / removal events.
 *
 * Must be called before sd_card_start_hotswap_polling().  Pass NULL to
 * clear the callback.
 */
void sd_card_set_event_cb(sd_card_event_cb_t cb);

/**
 * @brief Start a background task that polls for SD card insertion and removal.
 *
 * The task checks the card state once per second.  When no card is mounted it
 * attempts sd_card_init(); when a card is mounted it sends CMD13
 * (sdmmc_get_status) and, if the card no longer responds, unmounts the
 * filesystem and clears mount state.
 *
 * Safe to call multiple times; subsequent calls are no-ops.
 */
void sd_card_start_hotswap_polling(void);

esp_err_t sd_card_write_file(const char *path, const uint8_t *data, size_t len);
esp_err_t sd_card_read_file(const char *path, uint8_t **data_out,
                            size_t *len_out);
esp_err_t sd_card_file_exists(const char *path, bool *exists);
esp_err_t sd_card_delete_file(const char *path);

esp_err_t sd_card_list_files(const char *dir_path, char ***files_out,
                             int *count_out);
void sd_card_free_file_list(char **files, int count);

#ifdef __cplusplus
}
#endif

#endif
