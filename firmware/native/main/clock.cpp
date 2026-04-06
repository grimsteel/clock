#include "esp_exception.hpp"
#include "esp_log.h"
#include "gpio_cxx.hpp"
#include "led.hpp"
#include "wifi.hpp"
#include "esp_event.h"
#include "nvs_flash.h"

using namespace idf;

#define TAG "clock"

extern "C" void app_main(void)
{
    try {
        ESP_LOGI(TAG, "Initializing...");
        const GPIO_Output led_1(GPIONum(4));
        const GPIO_Output led_2(GPIONum(7));
        const GPIO_Output led_onboard(GPIONum(8));
        
        // off
        led_1.set_high();
        led_2.set_high();
        
        // turn status led on until connect
        led_onboard.set_low();
        
        ESP_ERROR_CHECK(nvs_flash_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        
        wifi::connect();
    } catch (GPIOException &e) {
        ESP_LOGE(TAG, "GPIO exception: %s", esp_err_to_name(e.error));
    } catch (wifi::WifiException &e) {
        ESP_LOGE(TAG, "Wi-Fi exception: %s", esp_err_to_name(e.error));
    } catch (ESPException &e) {
        ESP_LOGE(TAG, "Miscellaneous exception: %s", esp_err_to_name(e.error));
    }
}
