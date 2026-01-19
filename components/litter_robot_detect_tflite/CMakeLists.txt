idf_component_register(SRCS "litter_robot_detect_tflite.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES esp-tflite-micro esp32-camera
                    EMBED_FILES test_image.jpg
                    )

