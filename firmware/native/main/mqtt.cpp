#include <algorithm>
#include <string>
#include <string_view>
#include <sstream>
#include "mqtt.hpp"
#include "esp_mqtt.hpp"
#include "esp_mqtt_client_config.hpp"

/// Match an event topic against a filter, but return the last wildcard in *wildcard_out
/// Modified version of idf::mqtt::Filter::match
static bool filter_match_wildcard(idf::mqtt::Filter &filter, esp_mqtt_event_handle_t const event, std::string_view *wildcard_out) {
    auto filter_begin = filter.get().begin();
    auto filter_end = filter.get().end();
    auto topic_begin = static_cast<std::string::const_iterator>(event->topic);
    auto topic_end = topic_begin + event->topic_len;
    
    for (auto mismatch = std::mismatch(filter_begin, filter_end, topic_begin);
            mismatch.first != filter_end and mismatch.second != topic_end;
            mismatch = std::mismatch(filter_begin, filter_end, topic_begin)) {
        // found a mismatch - check for wildcard
        if (*mismatch.first != '+') {
            return false;
        }
        // advance the filter and topic to next /
        filter_begin = std::find(mismatch.first, filter_end, '/');
        topic_begin = std::find(mismatch.second, topic_end, '/');
        // extract the relevant part of `topic`
        *wildcard_out = std::string_view(mismatch.second, topic_begin);
        // if we reached the end of the filter, end
        if (filter_begin == filter_end && topic_begin != topic_end) {
            return false;
        }
    }
    return true;
}

namespace mqtt {
    
// set status to off on disconnect
idf::mqtt::LastWill ClockMQTTClient::LAST_WILL = {
    .lwt_topic = MQTT_PREFIX "/status",
    .lwt_msg = "off",
    .lwt_qos = (int) idf::mqtt::QoS::AtLeastOnce,
    .lwt_retain = true,
    .lwt_msg_len = 2
};

idf::event::ESPEvent ClockMQTTClient::MQTT_EVENT_LIGHT(MQTT_EVENT_BASE, MQTT_EVENT_ID_LIGHT);
idf::event::ESPEvent ClockMQTTClient::MQTT_EVENT_PEOPLE(MQTT_EVENT_BASE, MQTT_EVENT_ID_PEOPLE);
idf::event::ESPEvent ClockMQTTClient::MQTT_EVENT_LOCATIONS(MQTT_EVENT_BASE, MQTT_EVENT_ID_LOCATIONS);
idf::event::ESPEvent ClockMQTTClient::MQTT_EVENT_LOCATION_UPDATE(MQTT_EVENT_BASE, MQTT_EVENT_ID_LOCATION_UPDATE);

ClockMQTTClient::ClockMQTTClient(
    const idf::mqtt::BrokerConfiguration &broker,
    const idf::mqtt::ClientCredentials &credentials,
    const idf::event::ESPEventLoop event_loop
) : idf::mqtt::Client(broker, credentials, idf::mqtt::Configuration { .session = { .last_will = LAST_WILL } }), event_loop(event_loop) { }
    
void ClockMQTTClient::on_connected(esp_mqtt_event_handle_t const event) {
    using idf::mqtt::QoS;
    subscribe(leds.get());
    subscribe(hass_locations.get());
    
    // send on message
    publish(std::string { MQTT_PREFIX "/status" }, idf::mqtt::StringMessage {
        .data = std::string { "on" },
        .qos = idf::mqtt::QoS::AtLeastOnce,
        .retain = idf::mqtt::Retain::Retained
    });
}

void ClockMQTTClient::on_data(esp_mqtt_event_handle_t const event) {
    std::string_view param;
    std::string payload(event->data, event->data + event->data_len);
    if (filter_match_wildcard(leds, event, &param)) {
        // set LED value
        struct LightEventPayload evt_payload;
        // wildcard is led id
        if (param == "board") {
            evt_payload.light = LightBoard;
        } else if (param == "1") {
            evt_payload.light = Light1;
        } else if (param == "2") {
            evt_payload.light = Light2;
        }
        evt_payload.on = payload == "on";
        event_loop.post_event_data(MQTT_EVENT_LIGHT, evt_payload);
    } else if (filter_match_wildcard(hass_locations, event, &param)) {
        // update location
        struct LocationUpdateEventPayload evt_payload;
        evt_payload.location = payload;
        evt_payload.person = param;
        event_loop.post_event_data(MQTT_EVENT_LOCATION_UPDATE, evt_payload);
    } else if (people.match(event->topic, event->topic_len)) {
        // update people mapping
        std::stringstream payload_stream(payload);
        struct PeopleEventPayload evt_payload;
        for (int i = 0; i < 4; i++) {
            if (!std::getline(payload_stream, evt_payload.people[i], ',')) break;
        }
        event_loop.post_event_data(MQTT_EVENT_PEOPLE, evt_payload);
    } else if (locations.match(event->topic, event->topic_len)) {
        // update zone mapping
        std::stringstream payload_stream(payload);
        struct LocationEventPayload evt_payload;
        for (int i = 0; i < 5; i++) {
            if (!std::getline(payload_stream, evt_payload.locations[i], ',')) break;
        }
        event_loop.post_event_data(MQTT_EVENT_LOCATIONS, evt_payload);
        
    }
}


}
