// Load PSBT from SD Card — browse and select .psbt files stored on the SD card

#ifndef LOAD_PSBT_STORAGE_H
#define LOAD_PSBT_STORAGE_H

#include <lvgl.h>

/**
 * Create the Load PSBT storage-browser page.
 *
 * @param parent     Parent LVGL object
 * @param return_cb  Called when the user navigates back from the browser
 *                   without selecting a file
 * @param success_cb Called when the user has finished with the scan/sign page
 *                   that was launched after selecting a PSBT file
 */
void load_psbt_storage_page_create(lv_obj_t *parent, void (*return_cb)(void),
                                   void (*success_cb)(void));

void load_psbt_storage_page_show(void);
void load_psbt_storage_page_hide(void);
void load_psbt_storage_page_destroy(void);

#endif /* LOAD_PSBT_STORAGE_H */
