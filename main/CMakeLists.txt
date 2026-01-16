set(requires esp32-camera cat_detect esp_event esp_wifi nvs_flash nvs_flash esp_netif esp_http_server)

idf_component_register(SRCS "main.cpp" "camera_pin.h" "wifi_manager.cpp"
                    INCLUDE_DIRS ""
                    PRIV_REQUIRES ${requires})

target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
