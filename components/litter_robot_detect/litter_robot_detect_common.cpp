#include "litter_robot_detect.hpp"

extern "C"
{
    extern const uint8_t _binary_test_image_jpg_start[] asm("_binary_test_image_jpg_start");
    extern const uint8_t _binary_test_image_jpg_end[] asm("_binary_test_image_jpg_end");
}

void litter_robot_detect::CatDetect::test_model()
{
    ESP_LOGI(TAG, "Testing model with embedded image");

    size_t test_image_len = _binary_test_image_jpg_end - _binary_test_image_jpg_start;

    // Create a dummy camera frame buffer
    camera_fb_t fb;
    fb.buf = (uint8_t *)_binary_test_image_jpg_start;
    fb.len = test_image_len;
    fb.width = 0;  // Decoder might determine this, or set if known/needed
    fb.height = 0; // Decoder might determine this, or set if known/needed
    fb.format = PIXFORMAT_JPEG;

    prediction_result_t result = run_inference(&fb);

    if (result.err == ESP_OK)
    {
        ESP_LOGI(TAG, "Inference successful!");
        ESP_LOGI(TAG, "Predicted Class: %s", result.predicted_class.c_str());
        ESP_LOGI(TAG, "Scores - Empty: %d, Nachi: %d, Ngao: %d",
                 result.empty_score, result.nachi_score, result.ngao_score);
    }
    else
    {
        ESP_LOGE(TAG, "Inference failed with error %d", result.err);
    }
}