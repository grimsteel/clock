#include "esp_netif_defaults.h"
#if __cpp_exceptions

#include "esp_exception.hpp"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"

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

void connect() {
    ESP_LOGI(TAG, "Initialize Wi-Fi");
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    WIFI_CHECK_THROW(esp_wifi_init(&cfg));
    
    // Create STA
    esp_netif_create_default_wifi_sta(); 
    WIFI_CHECK_THROW(esp_wifi_set_mode(WIFI_MODE_STA));
    WIFI_CHECK_THROW(esp_wifi_start());
}

void disconnect();

}

#endif
