#pragma once

#include "esp_camera.h"
#include "esp_log.h"
#include "esp_err.h"
#include "camera_pin.h"
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "wifi_manager.hpp"
#include "nvs_flash.h"
#ifdef CONFIG_DETECTION_CAT_DETECT
#include "cat_detect.hpp"
#include "dl_image_jpeg.hpp"
#elif defined(CONFIG_DETECTION_LITTER_ROBOT_TFLITE)
#include "litter_robot_detect.hpp"
#endif
#include "esp_http_server.h"

namespace myapp
{
    static esp_err_t stream_handler(httpd_req_t *req);
#define PART_BOUNDARY "123456789000000000000987654321"
    static const char *_STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
    static const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
    static const char *_STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

    class CameraApp
    {
    public:
        CameraApp();
        ~CameraApp();
        esp_err_t setup_camera();
        esp_err_t setup_model();
        httpd_handle_t start_http_server_task();
        static void run_inference_task(void *pvParameters);
        static constexpr const char *TAG = "camera_app";
        void run_inference(const camera_fb_t *fb);
        TaskHandle_t ai_task_handler;
        camera_fb_t *inference_fb;

    private:
#ifdef CONFIG_DETECTION_CAT_DETECT
        CatDetect *detect;
#elif defined(CONFIG_DETECTION_LITTER_ROBOT_TFLITE)
        litter_robot_detect::CatDetect *detect;
#endif
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
            .fb_count = 2,
            .fb_location = CAMERA_FB_IN_PSRAM,
            .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
            .sccb_i2c_port = 0};
    };
} // namespace camera_app
