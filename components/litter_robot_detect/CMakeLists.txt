set(embed_files test_image.jpg)
set(requires esp32-camera)
set(impl_srcs "litter_robot_detect_common.cpp")

if(CONFIG_LITTER_ROBOT_MODEL_TFLITE)
    message(STATUS "Litter Robot Detect: Using TF LITE model")

    list(APPEND impl_srcs "litter_robot_detect_tflite.cpp")
    list(APPEND requires espressif__esp-tflite-micro esp_jpeg)
    
    message(STATUS "Current requires: ${requires}")
endif()

if(CONFIG_LITTER_ROBOT_MODEL_ESP_PPQ)
    message(STATUS "Litter Robot Detect: Using ESP-DL PPQ model")

    idf_build_get_property(component_targets __COMPONENT_TARGETS)
    if ("___idf_espressif__esp-dl" IN_LIST component_targets)
        idf_component_get_property(espdl_dir espressif__esp-dl COMPONENT_DIR)
    elseif("___idf_esp-dl" IN_LIST component_targets)
        idf_component_get_property(espdl_dir esp-dl COMPONENT_DIR)
    endif()

    set(cmake_dir ${espdl_dir}/fbs_loader/cmake)
    include(${cmake_dir}/utilities.cmake)

    set(model_files lite_model_esp32s3.espdl)

    list(APPEND impl_srcs "litter_robot_detect_ppq.cpp")
    list(APPEND requires esp-dl)
endif()

idf_component_register(SRCS ${impl_srcs}
                INCLUDE_DIRS "include"
                REQUIRES ${requires}
                EMBED_FILES ${embed_files}
                )

if(CONFIG_LITTER_ROBOT_MODEL_ESP_PPQ)
    target_add_aligned_binary_data(${COMPONENT_LIB} ${model_files} BINARY)
endif()