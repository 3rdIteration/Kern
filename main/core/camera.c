#include "camera.h"
#include "../components/video/video.h"
#include "../ui/dialog.h"
#include <bsp/esp-bsp.h>
#include <esp_log.h>

static const char *TAG = "camera";

void camera_detect(void) {
  bool detected = app_video_detect(bsp_i2c_get_handle());
  if (!detected) {
    ESP_LOGW(TAG, "No camera detected at boot");
  }
}

bool camera_is_available(void) { return app_video_is_available(); }

void camera_show_no_camera_dialog(void) {
  dialog_show_message("No Camera Detected", "This feature requires a camera.");
}
