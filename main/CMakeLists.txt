# Base requirements
set(requires esp32-camera esp_event esp_wifi nvs_flash esp_netif esp_http_server litter_robot_detect)

if(CONFIG_TARGET_ESP32S3)
    list(APPEND requires cat_detect)
else()
    message(STATUS "Skipping cat_detect model for non-ESP32S3 target")
endif()

idf_component_register(SRCS "main.cpp" "camera_pin.h" "wifi_manager.cpp"
                    INCLUDE_DIRS ""
                    PRIV_REQUIRES ${requires})

target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
