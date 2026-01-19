#pragma once

#include <tensorflow/lite/core/c/common.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/micro/micro_mutable_op_resolver.h>
#include <esp_camera.h>

namespace litter_robot_detect_tflite
{
    typedef struct
    {
        uint8_t empty_score;
        uint8_t nachi_score;
        uint8_t ngao_score;
        std::string predicted_class;
        esp_err_t err;
    } prediction_result_t;

    static const std::string CLASS_NAMES[] = {"empty", "nachi", "ngao"};

    class CatDetect
    {
    public:
        CatDetect();
        ~CatDetect();

        esp_err_t setup(size_t tensor_arena_size);
        prediction_result_t run_inference(const camera_fb_t *fb);
        void test_model();

    private:
        static constexpr const char *TAG{"litter_robot_detect_tflite::CatDetect"};
        uint8_t *tensor_arena_{nullptr};
        const tflite::Model *model{nullptr};
        tflite::MicroInterpreter *interpreter{nullptr};
        tflite::MicroMutableOpResolver<7> *resolver_{nullptr};
        void decode_result(prediction_result_t &result);
    };
} // namespace litter_robot_detect_tflite
