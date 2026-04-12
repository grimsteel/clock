#pragma once

#if __cpp_exceptions

#include "esp_err.h"
#include "esp_exception.hpp"
#include "driver/ledc.h"
#include "gpio_cxx.hpp"

#define SERVO_PWM_FREQUENCY          (50) // Hz

namespace servo {

struct ServoException : public idf::ESPException {
    ServoException(esp_err_t err) : idf::ESPException(err) {}
};

class Servo {
public:
    /// Init PWM timer
    static void init();
    Servo(idf::GPIONum gpio_num);
    /// between 0 and 360
    void set_angle(int angle);
private:
    ledc_channel_t channel_id;
    static ledc_channel_t current_channel_id;
};

}

#endif
