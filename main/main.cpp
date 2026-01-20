#include "main.hpp"

myapp::CameraApp::CameraApp()
{
}

myapp::CameraApp::~CameraApp()
{
#if defined(CONFIG_DETECTION_CAT_DETECT) || defined(CONFIG_DETECTION_LITTER_ROBOT_TFLITE)
    delete detect;
#endif
}

esp_err_t myapp::CameraApp::setup_camera()
{
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Camera Init Failed");
        return err;
    }

    sensor_t *s = esp_camera_sensor_get();
    if (s->id.PID == OV2640_PID)
    {
        ESP_LOGI(TAG, "OV2640 detected");
    }
    else if (s->id.PID == OV3660_PID)
    {
        ESP_LOGI(TAG, "OV3660 detected");
        s->set_vflip(s, 1); // Flip vertically
    }
    else
    {
        ESP_LOGW(TAG, "Unknown sensor detected: PID=0x%02X", s->id.PID);
    }
    s->set_brightness(s, 1);
    s->set_saturation(s, -2);
    s->set_awb_gain(s, 1); // Enabled AWB gain control
    s->set_wb_mode(s, 1);  // Cloudy mode
    err = gpio_config(&io_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "GPIO Config Failed");
    }

    return ESP_OK;
}

esp_err_t myapp::CameraApp::setup_model()
{
#ifdef CONFIG_DETECTION_CAT_DETECT
    detect = new CatDetect();
#elif defined(CONFIG_DETECTION_LITTER_ROBOT_TFLITE)
    detect = new litter_robot_detect::CatDetect();
    ESP_ERROR_CHECK(detect->setup(1024 * 1024));
    detect->test_model();

#endif
    return ESP_OK;
}

httpd_handle_t myapp::CameraApp::start_http_server_task()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.ctrl_port = 32768;

    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK)
    {
        // Stream Endpoint
        httpd_uri_t stream_uri = {
            .uri = "/stream",
            .method = HTTP_GET,
            .handler = stream_handler,
            .user_ctx = this};
        httpd_register_uri_handler(server, &stream_uri);
    }
    return server;
}

void myapp::CameraApp::run_inference_task(void *pvParameters)
{
    auto app = static_cast<myapp::CameraApp *>(pvParameters);
    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        if (app->inference_fb)
        {
            app->run_inference(app->inference_fb);

            // AI is finally done, NOW we return the buffer to the camera driver
            esp_camera_fb_return(app->inference_fb);
            app->inference_fb = NULL;
        }
    }
}

void myapp::CameraApp::run_inference(const camera_fb_t *fb)
{
#ifdef CONFIG_DETECTION_CAT_DETECT
    uint32_t start_decode = esp_log_timestamp();
    dl::image::jpeg_img_t jpeg_img = {
        .data = fb->buf,
        .data_len = fb->len,
    };
    auto img = dl::image::sw_decode_jpeg(jpeg_img, dl::image::DL_IMAGE_PIX_TYPE_RGB888);
    uint32_t end_decode = esp_log_timestamp();
    ESP_LOGI(TAG, "JPEG decode took %lu ms", end_decode - start_decode);

    uint32_t start_infer = esp_log_timestamp();
    auto &detect_results = detect->run(img);
    uint32_t end_infer = esp_log_timestamp();
    ESP_LOGI(TAG, "Inference took %lu ms", end_infer - start_infer);

    for (const auto &res : detect_results)
    {
        ESP_LOGI(TAG,
                 "[category: %d, score: %f, x1: %d, y1: %d, x2: %d, y2: %d]",
                 res.category,
                 res.score,
                 res.box[0],
                 res.box[1],
                 res.box[2],
                 res.box[3]);
    }
    heap_caps_free(img.data);
#elifdef CONFIG_DETECTION_LITTER_ROBOT_TFLITE
    uint32_t start_infer = esp_log_timestamp();
    auto result = detect->run_inference(fb);
    uint32_t end_infer = esp_log_timestamp();
    ESP_LOGI(TAG, "Inference took %lu ms", end_infer - start_infer);
    if (result.err != ESP_OK)
    {
        ESP_LOGE(TAG, "Inference error: 0x%x", result.err);
        return;
    }
    else
    {
        ESP_LOGI(TAG, "Predicted class: %s", result.predicted_class.c_str());
    }
#endif
}

static esp_err_t myapp::stream_handler(httpd_req_t *req)
{
    camera_fb_t *fb = NULL;
    esp_err_t res = ESP_OK;
    char *part_buf[64];

    httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    auto app = static_cast<myapp::CameraApp *>(req->user_ctx);

    while (true)
    {
        fb = esp_camera_fb_get();
        if (!fb)
        {
            ESP_LOGE("HTTP", "Camera capture failed");
            res = ESP_FAIL;
            break;
        }

        // 1. Send the boundary
        res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));

        // 2. Send the header (JPEG length)
        size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, fb->len);
        res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);

        // 3. Send the actual JPEG data
        res = httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);

        // --- HANDOVER LOGIC FOR AI ---
        // If AI is idle, hand it this buffer.
        // If AI is busy, we MUST return it now so the camera can reuse it.
        if (app->inference_fb == NULL)
        {
            app->inference_fb = fb;
            xTaskNotifyGive(app->ai_task_handler);
            // We do NOT return fb here; inference_task will do it.
        }
        else
        {
            esp_camera_fb_return(fb);
        }

        if (res != ESP_OK)
            break;
    }
    return res;
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
    camera_app.setup_model();
    ESP_LOGI(myapp::CameraApp::TAG, "Running with cpp");
    if (err != ESP_OK)
    {
        ESP_LOGE(myapp::CameraApp::TAG, "Camera init failed with error 0x%x", err);
        return;
    }

    xTaskCreatePinnedToCore(myapp::CameraApp::run_inference_task, "ai_task", 16384, &camera_app, tskIDLE_PRIORITY, &camera_app.ai_task_handler, 1);
    camera_app.start_http_server_task();
}