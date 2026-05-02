#ifndef SCAN_H
#define SCAN_H

#include <lvgl.h>
#include <stddef.h>
#include <stdint.h>

/**
 * Create the scan page — universal QR content detection
 * @param parent Parent LVGL object
 * @param return_cb Callback function to call when returning to home
 */
void scan_page_create(lv_obj_t *parent, void (*return_cb)(void));

/**
 * Create the scan page pre-loaded with a PSBT from binary data (bypasses QR
 * scanner). Used when loading a PSBT from SD card.
 * @param parent     Parent LVGL object
 * @param psbt_bytes Raw PSBT binary data
 * @param psbt_len   Length of psbt_bytes
 * @param return_cb  Callback function to call when returning to home
 */
void scan_page_create_with_psbt(lv_obj_t *parent, const uint8_t *psbt_bytes,
                                size_t psbt_len, void (*return_cb)(void));

/**
 * Show the scan page
 */
void scan_page_show(void);

/**
 * Hide the scan page
 */
void scan_page_hide(void);

/**
 * Destroy the scan page and free resources
 */
void scan_page_destroy(void);

#endif // SCAN_H
