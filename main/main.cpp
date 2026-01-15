#include "main.hpp"

myapp::CameraApp::CameraApp()
{
}

esp_err_t myapp::CameraApp::setup_camera()
{
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Camera Init Failed");
        return err;
    }

    err = gpio_config(&io_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "GPIO Config Failed");
    }

    return ESP_OK;
}

esp_err_t myapp::CameraApp::capture_image()
{
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb)
    {
        ESP_LOGE(TAG, "Camera Capture Failed");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Captured image: %zu bytes", fb->len);
    esp_camera_fb_return(fb);
    return ESP_OK;
}

void myapp::CameraApp::capture_task(void *pvParameters)
{
    myapp::CameraApp *app = static_cast<myapp::CameraApp *>(pvParameters);

    while (1)
    {
        app->current_tick++;
        ESP_LOGI(TAG, "Capture tick %d", app->current_tick);
        if (app->current_tick % 10 == 0)
        {
            app->set_flash(true);
        }
        else
        {
            app->set_flash(false);
        }
        esp_err_t err = app->capture_image();
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Camera capture failed with error 0x%x", err);
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // Capture an image every 2 seconds
    }
}

void myapp::CameraApp::set_flash(bool on)
{
    if (on)
    {
        gpio_set_level(CAM_PIN_FLASH, 1);
    }
    else
    {
        gpio_set_level(CAM_PIN_FLASH, 0);
    }
}

extern "C" void app_main()
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    static myapp::CameraApp camera_app;
    myapp::WifiManager wifi_manager = myapp::WifiManager("chi-ngao", "khongcopass");
    wifi_manager.setup_wifi();

    esp_err_t err = camera_app.setup_camera();
    ESP_LOGI(myapp::CameraApp::TAG, "Running with cpp");
    if (err != ESP_OK)
    {
        ESP_LOGE(myapp::CameraApp::TAG, "Camera init failed with error 0x%x", err);
        return;
    }
    xTaskCreate(myapp::CameraApp::capture_task, "camera_capture_task", 4096, &camera_app, tskIDLE_PRIORITY, NULL);
}