#ifndef BATTERY_H
#define BATTERY_H

#include <lvgl.h>

/**
 * Create a battery percentage label with auto-refresh timer.
 * Color and symbol reflect charge state: orange ⚡ when charging, green ✓
 * when fully charged, green when discharging above 20%, red at or below 20%.
 * Returns NULL if PMIC is unavailable. The refresh timer is automatically
 * deleted when the label is destroyed.
 *
 * @param parent Parent LVGL object
 * @return Label object, or NULL if battery info is unavailable
 */
lv_obj_t *ui_battery_create(lv_obj_t *parent);

#endif
