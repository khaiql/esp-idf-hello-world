idf_component_register(SRCS "main.cpp" "camera_pin.h" "wifi_manager.cpp"
                    INCLUDE_DIRS ""
                    REQUIRES esp32-camera cat_detect esp_event esp_wifi nvs_flash)

target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
