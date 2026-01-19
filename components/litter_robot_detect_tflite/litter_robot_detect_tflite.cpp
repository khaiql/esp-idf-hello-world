#include <stdio.h>
#include "litter_robot_detect_tflite.hpp"
#include "model_data.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

litter_robot_detect_tflite::CatDetect::CatDetect()
{
}

litter_robot_detect_tflite::CatDetect::~CatDetect()
{
    if (interpreter)
    {
        delete interpreter;
        interpreter = nullptr;
    }
    if (resolver_)
    {
        delete resolver_;
        resolver_ = nullptr;
    }
    if (tensor_arena_)
    {
        heap_caps_free(tensor_arena_);
        tensor_arena_ = nullptr;
    }
}

esp_err_t litter_robot_detect_tflite::CatDetect::setup(size_t tensor_arena_size)
{
    // Load model
    model = tflite::GetModel(g_model_data);
    if (model->version() != TFLITE_SCHEMA_VERSION)
    {
        ESP_LOGE("CatDetect", "Model provided is schema version %d not equal "
                              "to supported version %d.",
                 model->version(), TFLITE_SCHEMA_VERSION);
        return ESP_FAIL;
    }

    // Create interpreter
    resolver_ = new tflite::MicroMutableOpResolver<7>();
    resolver_->AddConv2D();
    resolver_->AddDepthwiseConv2D();
    resolver_->AddQuantize();
    resolver_->AddReshape();
    resolver_->AddSoftmax();
    resolver_->AddMean();
    resolver_->AddFullyConnected();

    tensor_arena_ = (uint8_t *)heap_caps_malloc(tensor_arena_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!tensor_arena_)
    {
        ESP_LOGE(TAG, "Failed to allocate tensor arena");
        return ESP_ERR_NO_MEM;
    }

    interpreter = new tflite::MicroInterpreter(
        model, *resolver_, tensor_arena_, tensor_arena_size);
    if (!interpreter)
    {
        ESP_LOGE(TAG, "Failed to create interpreter");
        return ESP_ERR_NO_MEM;
    }

    // Allocate memory from the tensor_arena for the model's tensors.
    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk)
    {
        ESP_LOGE(TAG, "AllocateTensors() failed");
        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "setup model successfully");
    return ESP_OK;
}

litter_robot_detect_tflite::prediction_result_t litter_robot_detect_tflite::CatDetect::run_inference(const camera_fb_t *fb)
{
    prediction_result_t result;
    result.err = ESP_OK;
    TfLiteTensor *input = interpreter->input(0);

    esp_jpeg_image_cfg_t jpeg_cfg = {.indata = fb->buf,
                                     .indata_size = fb->len,
                                     .outbuf = input->data.uint8, // Decode directly to tensor input
                                     .outbuf_size = input->bytes,
                                     .out_format = JPEG_IMAGE_FORMAT_RGB888,
                                     .out_scale = JPEG_IMAGE_SCALE_0,
                                     .flags = {
                                         .swap_color_bytes = 0,
                                     }};
    esp_jpeg_image_output_t outimg;

    esp_err_t res = esp_jpeg_decode(&jpeg_cfg, &outimg);
    if (res != ESP_OK)
    {
        ESP_LOGE(TAG, "JPEG decode failed with error 0x%x", res);
        result.err = res;
        return result;
    }

    // Fix for signed int8 models: convert [0,255] to [-128,127]
    // TFLite quantization often expects int8 inputs (-128 to 127) internally.
    // If inference_input_type was not strictly enforced or if the model
    // lacks a Cast/Quantize op at the entry, the input tensor might be int8.
    // Passing raw uint8 [0,255] results in wrapping (128 becomes -128), ruining predictions.
    if (input->type == kTfLiteInt8)
    {
        ESP_LOGI(TAG, "Using int8 input tensor, adjusting [0,255] to [-128,127]");
        uint8_t *data = input->data.uint8;
        for (size_t i = 0; i < input->bytes; i++)
        {
            // xor 0x80 is equivalent to subtracting 128 for byte-sized values
            // 0 (0x00) ^ 0x80 = 128 (0x80) -> interpreted as -128
            // 255 (0xFF) ^ 0x80 = 127 (0x7F) -> interpreted as 127
            data[i] ^= 0x80;
        }
    }

    TfLiteStatus invokeStatus = this->interpreter->Invoke();
    if (invokeStatus != kTfLiteOk)
    {
        ESP_LOGE(TAG, "Invoke failed");
        result.err = ESP_FAIL;
        return result;
    }

    decode_result(result);
    return result;
}

void litter_robot_detect_tflite::CatDetect::decode_result(prediction_result_t &result)
{
    // Handle different output types
    uint8_t empty_score, nachi_score, ngao_score;
    TfLiteTensor *output = interpreter->output(0);

    if (output->type == kTfLiteUInt8)
    {
        empty_score = output->data.uint8[0];
        nachi_score = output->data.uint8[1];
        ngao_score = output->data.uint8[2];
    }
    else if (output->type == kTfLiteInt8)
    {
        // Convert int8 [-128, 127] to uint8 [0, 255]
        empty_score = output->data.int8[0] + 128;
        nachi_score = output->data.int8[1] + 128;
        ngao_score = output->data.int8[2] + 128;
    }
    else
    {
        ESP_LOGE(TAG, "Unsupported output tensor type: %d", output->type);
        result.err = ESP_ERR_NOT_SUPPORTED;
        return;
    }

    ESP_LOGD(TAG, "empty_score=%d nachi_score=%d ngao_score=%d", empty_score, nachi_score, ngao_score);

    uint8_t scores[] = {empty_score, nachi_score, ngao_score};
    int max_index = 0;
    for (int i = 1; i < 3; ++i)
    {
        if (scores[i] > scores[max_index])
        {
            max_index = i;
        }
    }

    result.empty_score = empty_score;
    result.nachi_score = nachi_score;
    result.ngao_score = ngao_score;
    result.predicted_class = CLASS_NAMES[max_index];
}

extern const uint8_t test_nachi_jpg_start[] asm("_binary_test_nachi_jpg_start");
extern const uint8_t test_nachi_jpg_end[] asm("_binary_test_nachi_jpg_end");

void litter_robot_detect_tflite::CatDetect::test_model()
{
    ESP_LOGI(TAG, "Testing model with embedded image test_nachi.jpg");

    size_t test_image_len = test_nachi_jpg_end - test_nachi_jpg_start;

    // Create a dummy camera frame buffer
    camera_fb_t fb;
    fb.buf = (uint8_t *)test_nachi_jpg_start;
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
