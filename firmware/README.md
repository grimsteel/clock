# Clock Firmware

There are two options for the firmware for the clock: ESPHome and a native ESP-IDF firmware.

Both of these options require location zones to be set up in Home Assistant for the relevant locations. This results in the `person.person_1` sensors having a string value representing the zone.

## Usage (ESPHome)

The ESPHome firmware pulls location info for each person through the ESPHome Home Assistant API, using a Home Assistant text sensor.

In order to use it, you'll need to:
1. (Boilerplate) Update the ESPHome encryption key, OTA password, and fallback AP password
2. Update the entity IDs for the `person` text sensors. Person 1 corresponds to servo 1, and so on. The servo numbers match the PCB designators for the connectors (so, they're placed from left to right on the board.)
3. Update the mappings of location to section ID in `location_mappings.h`. The `default` case statement can be used for a catchall.

## Usage (Native)

The native firmware only communicates with an MQTT broker, so you'll need to configure Home Assistant to publish person location state over MQTT. This can be configured with [MQTT Statestream](https://www.home-assistant.io/integrations/mqtt_statestream/):

`configuration.yaml`:
```yaml
mqtt_statestream:
  base_topic: homeassistant
  # If you need to publish attributes for other things, I would recommend making template entities that just have the location zone info to avoid publishing location coordinates on MQTT.
  publish_attributes: false
  publish_timestamps: false
  include:
    entities:
      - person.person_1
      - person.person_2
      - person.person_3
      - person.person_4
```


The native firmware is likely more lightweight than the ESPHome firmware, and will be used if I make a battery-powered version in the future.

### Build instructions

In `firmware/native`:

1. Run `menuconfig` to configure the Wi-Fi and MQTT connection information:
   * `idf.py menuconfig`
   * These parameters are in the "Clock Connection Configuration" section
   * WPA2 and username/password MQTT can be configured here; for more advanced setups you'll need to edit `wifi.cpp`
2. Flash the firmware: `idf.py flash`
3. Upon running, the onboard LED will blink until the Wi-Fi and MQTT connections have been established.
4. A Home Assistant discovery payload will be published, so the MQTT items below can be controlled through Home Assistant:

### MQTT Protocol Specification

Topic prefix: `/devices/clock`

#### LEDs

The three LED IDs are `led_1`, `led_2`, and `led_onboard`. State values are either `on` or `off`.

State is published at `/leds/{led_id}/state`. They can be controlled at `/leds/{led_id}/control`. All LEDs are initially set to off upon connect.

#### Person Configuration

The four Home Assistant person IDs can be configured by writing to `/config/people`.

The value should be a comma separated list of person IDs, without the `person.` prefix:

`person_1,person_2,person_3,person_4`.

This value will be remembered in flash, so it only needs to be published once.

#### Location Configuration

The five location zone IDs can be configured by writing to `/config/locations`, in the same format as above.

This value will also be remembered. 

The actual sectors which each location corresponds to depends on the servo orientation
