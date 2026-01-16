#pragma once

#include "esp_camera.h"
#include "esp_log.h"
#include "esp_err.h"
#include "camera_pin.h"
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "wifi_manager.hpp"
#include "nvs_flash.h"
#include "cat_detect.hpp"

namespace myapp
{
    class CameraApp
    {
    public:
        CameraApp();
        ~CameraApp();
        esp_err_t setup_camera();
        esp_err_t capture_image();
        static void capture_task(void *pvParameters);
        static constexpr const char *TAG = "camera_app";
        void set_flash(bool on);
        void run_inference(const camera_fb_t *fb);

    private:
        CatDetect *detect;
        uint8_t current_tick = 0;
        static constexpr gpio_config_t io_config = gpio_config_t{
            .pin_bit_mask = (1ULL << CAM_PIN_FLASH),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };

        static constexpr camera_config_t camera_config = camera_config_t{
            .pin_pwdn = CAM_PIN_PWDN,
            .pin_reset = CAM_PIN_RESET,
            .pin_xclk = CAM_PIN_XCLK,
            .pin_sccb_sda = CAM_PIN_SIOD,
            .pin_sccb_scl = CAM_PIN_SIOC,
            .pin_d7 = CAM_PIN_D7,
            .pin_d6 = CAM_PIN_D6,
            .pin_d5 = CAM_PIN_D5,
            .pin_d4 = CAM_PIN_D4,
            .pin_d3 = CAM_PIN_D3,
            .pin_d2 = CAM_PIN_D2,
            .pin_d1 = CAM_PIN_D1,
            .pin_d0 = CAM_PIN_D0,
            .pin_vsync = CAM_PIN_VSYNC,
            .pin_href = CAM_PIN_HREF,
            .pin_pclk = CAM_PIN_PCLK,
            .xclk_freq_hz = 20000000,
            .ledc_timer = LEDC_TIMER_0,
            .ledc_channel = LEDC_CHANNEL_0,
            .pixel_format = PIXFORMAT_JPEG,
            .frame_size = FRAMESIZE_VGA,
            .jpeg_quality = 12,
            .fb_count = 1,
            .fb_location = CAMERA_FB_IN_PSRAM,
            .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
            .sccb_i2c_port = 0};
    };
} // namespace camera_app
