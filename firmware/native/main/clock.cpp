#include "esp_event_cxx.hpp"
#include "esp_exception.hpp"
#include "esp_log.h"
#include "esp_mqtt_client_config.hpp"
#include "freertos/idf_additions.h"
#include "gpio_cxx.hpp"
#include "portmacro.h"
#include "sdkconfig.h"
#include "wifi.hpp"
#include "mqtt.hpp"
#include "servo.cpp"
#include "esp_event.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "nvs_handle.hpp"
#include <vector>

#define TAG "clock"

// lets encrypt
extern const char isrgrootx1_pem_start[]   asm("_binary_isrgrootx1_pem_start");
extern const char isrgrootx1_pem_end[]   asm("_binary_isrgrootx1_pem_end");

class ClockManager {
public:
    ClockManager(idf::event::ESPEventLoop &loop) : event_loop(loop) {
        // Initialize NVS
        esp_err_t err = nvs_flash_init();
        if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            // NVS partition was truncated and needs to be erased
            // Retry nvs_flash_init
            ESP_ERROR_CHECK(nvs_flash_erase());
            err = nvs_flash_init();
        }
        ESP_ERROR_CHECK( err );
        nvs_handle = nvs::open_nvs_handle("storage", NVS_READWRITE, &err);
        ESP_ERROR_CHECK(err);
        
        // Fetch from nvs
        char zone_str[7] = "zone_0";
        for (int i = 0; i < 5; i++) {
            // build actual string
            zone_str[5] = '0' + i;
            zones[i] = nvs_get_string(zone_str);
        }
        char people_str[9] = "person_0";
        for (int i = 0; i < 4; i++) {
            // build actual string
            people_str[7] = '0' + i;
            people[i] = nvs_get_string(people_str);
        }
        
        // Init servos
        servo::Servo::init();
        servos.reserve(4);
        for (int i = 0; i < 4; i++) {
            servos.push_back(servo::Servo(servo_gpios[i]));
        }
    }
    
    void init_callbacks() {
        event_loop.register_event(mqtt::ClockMQTTClient::MQTT_EVENT_LOCATIONS, [this](const idf::event::ESPEvent& event, void* data) {
            mqtt::LocationEventPayload* payload = static_cast<mqtt::LocationEventPayload*>(data);
            char zone_str[7] = "zone_0";
            for (int i = 0; i < 5; i++) {
                // build actual string
                zone_str[5] = '0' + i;
                zones[i] = move(payload->locations[i]);
                nvs_handle->set_string(zone_str, zones[i].c_str());
                nvs_handle->commit();
            }
        });
        event_loop.register_event(mqtt::ClockMQTTClient::MQTT_EVENT_PEOPLE, [this](const idf::event::ESPEvent& event, void* data) {
            mqtt::PeopleEventPayload* payload = static_cast<mqtt::PeopleEventPayload*>(data);
            char people_str[9] = "person_0";
            for (int i = 0; i < 4; i++) {
                // build actual string
                people_str[7] = '0' + i;
                people[i] = move(payload->people[i]);
                nvs_handle->set_string(people_str, people[i].c_str());
                nvs_handle->commit();
            }
        });
        event_loop.register_event(mqtt::ClockMQTTClient::MQTT_EVENT_LOCATION_UPDATE, [this](const idf::event::ESPEvent& event, void* data) {
            mqtt::LocationUpdateEventPayload* payload = static_cast<mqtt::LocationUpdateEventPayload*>(data);
            int person_idx = -1, location_idx = -1;
            // Find index
            for (int i = 0; i < 5; i++) {
                if (zones[i] == payload->location) {
                    location_idx = i;
                    break;
                }
            }
            for (int i = 0; i < 4; i++) {
                if (people[i] == payload->person) {
                    person_idx = i;
                    break;
                }
            }
            if (person_idx == -1 || location_idx == -1) return;
            // scale to full 360, then move by person, then move to center
            int angle = location_idx * 360 / 5 + person_idx * 360 / (5 * 4) + 360 / (5 * 4 * 2);
            servos[person_idx].set_angle(angle);
        });
    }
private:
    std::string nvs_get_string(const char* key) {
        size_t str_size;
        esp_err_t err = nvs_handle->get_item_size(nvs::ItemType::SZ, key, str_size);
        std::string str;
        if (err != ESP_ERR_NVS_NOT_FOUND) {
            str.resize(str_size); // size includes null term
            nvs_handle->get_string(key, str.data(), str_size);
        }
        
        return str;
    }
    std::unique_ptr<nvs::NVSHandle> nvs_handle;
    idf::event::ESPEventLoop &event_loop;
    std::string zones[5];
    std::string people[4];
    std::vector<servo::Servo> servos;
    idf::GPIONum servo_gpios[4] = { idf::GPIONum(10), idf::GPIONum(3), idf::GPIONum(0), idf::GPIONum(1) };
};

extern "C" void app_main(void)
{
    try {
        ESP_LOGI(TAG, "Initializing...");
        
        const idf::GPIO_Output led_1(idf::GPIONum(4));
        const idf::GPIO_Output led_2(idf::GPIONum(7));
        const idf::GPIO_Output led_onboard(idf::GPIONum(8));
        
        static idf::event::ESPEventLoop loop;
        
        // off
        led_1.set_high();
        led_2.set_high();
        
        // turn status led on until connect
        led_onboard.set_low();
        
        static ClockManager manager(loop);
        
        ESP_LOGI(TAG, "Initialized peripherals");
        
        wifi::connect();
        
        ESP_LOGI(TAG, "Connected to Wi-Fi");
        
        // init MQTT
        idf::mqtt::BrokerConfiguration broker {
            .address = {idf::mqtt::URI{std::string{CONFIG_CLOCK_MQTT_BROKER}}},
            .security = idf::mqtt::CryptographicInformation{idf::mqtt::PEM{isrgrootx1_pem_start}}
        };
        idf::mqtt::ClientCredentials credentials {
            .username = std::string { CONFIG_CLOCK_MQTT_USERNAME },
            .authentication = idf::mqtt::Password { std::string { CONFIG_CLOCK_MQTT_PASSWORD } }
        };

        mqtt::ClockMQTTClient mqtt_client(broker, credentials, loop);
        
        ESP_LOGI(TAG, "Connected to MQTT broker");
        
        // events are handled in the event loop
        manager.init_callbacks();
        while (true) vTaskDelay(portMAX_DELAY);
    } catch (idf::GPIOException &e) {
        ESP_LOGE(TAG, "GPIO exception: %s", esp_err_to_name(e.error));
    } catch (wifi::WifiException &e) {
        ESP_LOGE(TAG, "Wi-Fi exception: %s", esp_err_to_name(e.error));
    } catch (idf::ESPException &e) {
        ESP_LOGE(TAG, "Miscellaneous exception: %s", esp_err_to_name(e.error));
    }
}
