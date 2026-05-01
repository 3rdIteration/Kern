#pragma once

#include <stdbool.h>

/**
 * @brief Probe for camera hardware at boot.
 *
 * Tries to detect whether a camera is physically present by attempting
 * to initialise the video subsystem and open the device.  Must be called
 * once, before any camera-dependent UI page is created.
 */
void camera_detect(void);

/**
 * @brief Returns true if a camera was detected at boot.
 *
 * Returns the result cached by the most recent call to camera_detect().
 * Returns false if camera_detect() has not been called yet.
 */
bool camera_is_available(void);
