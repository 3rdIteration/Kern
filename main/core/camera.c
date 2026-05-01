#include "camera.h"
#include "../components/video/video.h"
#include <bsp/esp-bsp.h>

void camera_detect(void) { app_video_detect(bsp_i2c_get_handle()); }

bool camera_is_available(void) { return app_video_is_available(); }
