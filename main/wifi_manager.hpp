#pragma once

#include "esp_err.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi_default.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include <string>

namespace myapp
{
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

    class WifiManager
    {
    public:
        explicit WifiManager(const char *ssid, const char *password);
        void setup_wifi();
        static void event_handler(void *arg, esp_event_base_t event_base,
                                  int32_t event_id, void *event_data);
        void handle_event(esp_event_base_t event_base,
                          int32_t event_id, void *event_data);

    private:
        uint8_t retry_num{0};
        std::string ssid;
        std::string password;
        esp_event_handler_instance_t instance_any_id;
        esp_event_handler_instance_t instance_got_ip;
        static constexpr const char *TAG{"wifi_manager"};
        EventGroupHandle_t wifi_event_group;
        void init_wifi();
        void configure_wifi();
        void connect_wifi();
    };
}; // namespace myapp
