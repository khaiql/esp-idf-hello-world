#include "wifi_manager.hpp"

myapp::WifiManager::WifiManager(const char *ssid, const char *password) : ssid(ssid), password(password)
{
    this->wifi_event_group = xEventGroupCreate();
}

void myapp::WifiManager::setup_wifi()
{
    ESP_LOGI(TAG, "Setting up wifi");

    this->init_wifi();
    this->configure_wifi();
    EventBits_t eventBit = xEventGroupWaitBits(this->wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    if (eventBit & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "WiFi connected successfully");
    }
    else if (eventBit & WIFI_FAIL_BIT)
    {
        ESP_LOGE(TAG, "Failed to connect to WiFi");
    }
}

void myapp::WifiManager::event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    static_cast<myapp::WifiManager *>(arg)->handle_event(event_base, event_id, event_data);
}

void myapp::WifiManager::handle_event(esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        this->connect_wifi();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGI(TAG, "WiFi disconnected, reconnecting...");
        if (this->retry_num < 5)
        {
            this->retry_num++;
            ESP_LOGI(TAG, "Retrying to connect to the AP (Attempt %d)", this->retry_num);
            this->connect_wifi();
        }
        else
        {
            ESP_LOGW(TAG, "Maximum retry attempts reached, giving up");
            xEventGroupSetBits(this->wifi_event_group, WIFI_FAIL_BIT);
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = static_cast<ip_event_got_ip_t *>(event_data);
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(this->wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void myapp::WifiManager::init_wifi()
{
    ESP_LOGI(TAG, "Initializing WiFi");
    esp_err_t err = esp_netif_init();
    ESP_ERROR_CHECK(err);
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();
    wifi_init_config_t wifi_conf = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_conf));

    ESP_LOGI(TAG, "Registering WiFi Event handlers");
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, WifiManager::event_handler, this, &this->instance_any_id);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, WifiManager::event_handler, this, &this->instance_got_ip);
}

void myapp::WifiManager::configure_wifi()
{
    wifi_config_t wifi_config = {
        .sta = {
            .threshold = {
                .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            },
            .pmf_cfg = {.capable = true, .required = false},
        },
    };
    strlcpy((char *)wifi_config.sta.ssid, this->ssid.c_str(), sizeof(wifi_config.sta.ssid));
    strlcpy((char *)wifi_config.sta.password, this->password.c_str(), sizeof(wifi_config.sta.password));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi configured and started");
}

void myapp::WifiManager::connect_wifi()
{
    ESP_ERROR_CHECK(esp_wifi_connect());
}
