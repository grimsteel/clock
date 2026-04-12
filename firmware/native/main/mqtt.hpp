#pragma once

#include "esp_err.h"
#include "esp_event_cxx.hpp"
#include "esp_mqtt_client_config.hpp"
#include "mqtt.hpp"
#include "esp_mqtt.hpp"

#define MQTT_PREFIX "devices/clock"

namespace mqtt {

// event loop event definitions
ESP_EVENT_DEFINE_BASE(MQTT_EVENT_BASE);
const idf::event::ESPEventID MQTT_EVENT_ID_LIGHT(0);
const idf::event::ESPEventID MQTT_EVENT_ID_PEOPLE(1);
const idf::event::ESPEventID MQTT_EVENT_ID_LOCATIONS(2);
const idf::event::ESPEventID MQTT_EVENT_ID_LOCATION_UPDATE(3);

// event payloads
enum LightId {
    Light1,
    Light2,
    LightBoard
};
struct LightEventPayload {
    bool on;
    enum LightId light;
};

struct PeopleEventPayload {
    std::string people[4];
};
struct LocationEventPayload {
    std::string locations[5];
};
struct LocationUpdateEventPayload {
    std::string person;
    std::string location;
};
    
class ClockMQTTClient final : public idf::mqtt::Client {
public:
    ClockMQTTClient(
        const idf::mqtt::BrokerConfiguration &broker,
        const idf::mqtt::ClientCredentials &credentials,
        const idf::event::ESPEventLoop event_loop
    );
    static idf::event::ESPEvent MQTT_EVENT_LIGHT;
    static idf::event::ESPEvent MQTT_EVENT_PEOPLE;
    static idf::event::ESPEvent MQTT_EVENT_LOCATIONS;
    static idf::event::ESPEvent MQTT_EVENT_LOCATION_UPDATE;
private:
    void on_connected(esp_mqtt_event_handle_t const event) override;
    void on_data(esp_mqtt_event_handle_t const event) override;
    idf::mqtt::Filter leds { std::string { MQTT_PREFIX "/leds/+/control" } };
    idf::mqtt::Filter people { std::string { MQTT_PREFIX "/config/people" } };
    idf::mqtt::Filter locations { std::string { MQTT_PREFIX "/config/locations" } };
    idf::mqtt::Filter hass_locations { std::string { "/homeassistant/person/+/state_json" } };
    idf::event::ESPEventLoop event_loop;
    
    static idf::mqtt::LastWill LAST_WILL;
};

}
