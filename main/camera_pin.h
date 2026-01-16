#ifdef CONFIG_IDF_TARGET_ESP32
#define CAM_PIN_PWDN 32  // power down is not used
#define CAM_PIN_RESET -1 // software reset will be performed
#define CAM_PIN_XCLK 0
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27

#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 21
#define CAM_PIN_D2 19
#define CAM_PIN_D1 18
#define CAM_PIN_D0 5
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22

#define CAM_PIN_FLASH GPIO_NUM_4
#endif

#ifdef CONFIG_IDF_TARGET_ESP32S3

#define CAM_PIN_PWDN -1
#define CAM_PIN_RESET -1
#define CAM_PIN_XCLK 5
#define CAM_PIN_SIOD 8
#define CAM_PIN_SIOC 9

#define CAM_PIN_D7 4
#define CAM_PIN_D6 6
#define CAM_PIN_D5 7
#define CAM_PIN_D4 14
#define CAM_PIN_D3 17
#define CAM_PIN_D2 21
#define CAM_PIN_D1 18
#define CAM_PIN_D0 16
#define CAM_PIN_VSYNC 1
#define CAM_PIN_HREF 2
#define CAM_PIN_PCLK 15

#define CAM_PIN_FLASH GPIO_NUM_3
#endif
