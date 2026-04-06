#if __cpp_exceptions

#include "esp_exception.hpp"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif_defaults.h"
#include "freertos/idf_additions.h"
#include "portmacro.h"

#include "wifi.hpp"

namespace wifi {

#define TAG "wifi"

// Helper macro to throw an esp_err_t as a WifiException
#define WIFI_CHECK_THROW(err) \
 do {                                                                         \
   esp_err_t result = (err);                                                  \
   if (result != ESP_OK)                                                      \
     throw WifiException(result);                                             \
 } while (0)
 
static int reconnect_count = 0;
static SemaphoreHandle_t semaphore_got_ip = NULL;

#define MAX_RECONNECT 6
 
static void handler_wifi_disconnect(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (++reconnect_count > MAX_RECONNECT) {
        ESP_LOGE(TAG, "Wi-Fi connect failed %d times, stopping", MAX_RECONNECT);
        disconnect();
        return;
    }
    
    auto evt = static_cast<wifi_event_sta_disconnected_t*>(event_data);
    // ignore
    if (evt->reason == WIFI_REASON_ROAMING) return;
    
    // attempt reconnect
    ESP_LOGW(TAG, "Wi-Fi connected failed %d times, retrying", reconnect_count);
    esp_wifi_connect();
}

static void handler_got_ip(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    // reset for future reconnects
    reconnect_count = 0;
    auto evt = static_cast<ip_event_got_ip_t*>(event_data);
    ESP_LOGI(TAG, "Connected. Got IP address:  " IPSTR, IP2STR(&evt->ip_info.ip));
    
    xSemaphoreGive(semaphore_got_ip);
}

void connect() {
    ESP_LOGI(TAG, "Initialize Wi-Fi");
    WIFI_CHECK_THROW(esp_netif_init());
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    WIFI_CHECK_THROW(esp_wifi_init(&cfg));
    
    // Create STA
    esp_netif_create_default_wifi_sta(); 
    WIFI_CHECK_THROW(esp_wifi_set_mode(WIFI_MODE_STA));
    WIFI_CHECK_THROW(esp_wifi_start());
    
    // Apply auth config
    wifi_config_t wifi_cfg = {
        .sta = {
            .ssid = CONFIG_CLOCK_WIFI_SSID,
            .password = CONFIG_CLOCK_WIFI_PASSWORD,
            .scan_method = WIFI_FAST_SCAN,
            .threshold = {
                .authmode = WIFI_AUTH_WPA2_PSK,
            },
        },
    };
    
    semaphore_got_ip = xSemaphoreCreateBinary();
    
    WIFI_CHECK_THROW(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &handler_wifi_disconnect, NULL));
    WIFI_CHECK_THROW(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &handler_got_ip, NULL));
    WIFI_CHECK_THROW(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
    // start connection
    ESP_LOGI(TAG, "Connecting to %s...", wifi_cfg.sta.ssid);
    WIFI_CHECK_THROW(esp_wifi_connect());
    
    // wait for ip
    xSemaphoreTake(semaphore_got_ip, portMAX_DELAY);
}

void disconnect() {
    // unregister event handlers
    WIFI_CHECK_THROW(esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &handler_wifi_disconnect));
    WIFI_CHECK_THROW(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &handler_got_ip));
    vSemaphoreDelete(semaphore_got_ip);
    WIFI_CHECK_THROW(esp_wifi_disconnect());
}

#endif
