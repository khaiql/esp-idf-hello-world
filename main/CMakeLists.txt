idf_component_register(SRCS "main.cpp" "camera_pin.h" "wifi_manager.cpp"
                    INCLUDE_DIRS "")

target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
