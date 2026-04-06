#include "esp_log.h"
#include "gpio_cxx.hpp"
#include "led.hpp"

using namespace idf;

#define TAG "clock"

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Initializing...");
    const GPIO_Output led_1(GPIONum(4));
    const GPIO_Output led_2(GPIONum(7));
    const GPIO_Output led_onboard(GPIONum(8));
    
    // off
    led_1.set_high();
    led_2.set_high();
    
    // turn status led on until connect
    led_onboard.set_low();
}
