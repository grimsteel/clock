#include "servo.hpp"
#include "driver/ledc.h"
#include "hal/ledc_types.h"

namespace servo {
// Helper macro to throw an esp_err_t as a ServoException
#define SERVO_CHECK_THROW(err) \
    do {                                                                         \
    esp_err_t result = (err);                                                  \
    if (result != ESP_OK)                                                      \
        throw ServoException(result);                                             \
    } while (0)
    
#define DUTY_CYCLE_FULL (8192 / 5) // 2^13 / 5
    
void Servo::init() {
    ledc_timer_config_t ledc_timer = {};
    ledc_timer.speed_mode = LEDC_LOW_SPEED_MODE;
    ledc_timer.duty_resolution = LEDC_TIMER_13_BIT;
    ledc_timer.timer_num = LEDC_TIMER_0;
    ledc_timer.freq_hz = SERVO_PWM_FREQUENCY;
    ledc_timer.clk_cfg = LEDC_AUTO_CLK;
    SERVO_CHECK_THROW(ledc_timer_config(&ledc_timer));
}

// static counter
ledc_channel_t Servo::current_channel_id = LEDC_CHANNEL_0;

Servo::Servo(idf::GPIONum gpio_num) {
    channel_id = Servo::current_channel_id;
    // increment
    Servo::current_channel_id = static_cast<ledc_channel_t>(static_cast<int>(Servo::current_channel_id) + 1);
    
    ledc_channel_config_t ledc_channel = {};
    ledc_channel.gpio_num       = (int) gpio_num.get_value();
    ledc_channel.speed_mode     = LEDC_LOW_SPEED_MODE;
    ledc_channel.channel        = channel_id;
    ledc_channel.timer_sel      = LEDC_TIMER_0;
    ledc_channel.duty           = DUTY_CYCLE_FULL; // 5% = 0 deg
    ledc_channel.hpoint         = 0;
    SERVO_CHECK_THROW(ledc_channel_config(&ledc_channel));
}

void Servo::set_angle(int angle) {
    if (angle < 0 || angle >= 360) return;
    // fraction of 1/5
    uint32_t duty_fraction = DUTY_CYCLE_FULL * 360 / angle;
    // need to vary from 1/5 to 2/5
    duty_fraction += DUTY_CYCLE_FULL;
    SERVO_CHECK_THROW(ledc_set_duty(LEDC_LOW_SPEED_MODE, channel_id, duty_fraction));
    // Update duty to apply the new value
    SERVO_CHECK_THROW(ledc_update_duty(LEDC_LOW_SPEED_MODE, channel_id));
}

}
