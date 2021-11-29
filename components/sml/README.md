# SML component for ESPHome

## About
This external component provides a way to retrieve data from devices using the *Smart Meter Language* (SML). SML is mainly used by electrical counters but also solar devices, gas meters, and much more.

Although the SML protocol is well defined, it gives a lot of freedom to the manufacturers how to store and identify the transmitted data.
A transmitted physical value is identified by an OBIS code (Object Identification System). If it is known which code the manufacturer assigns to the physical value, the corresponding value can be extracted. Unfortunately this differs from manufacturer to manufacturer. Also the transmitted physical values are not fixed.

As an example, many manufacturers use the OBIS code 1-0:1.8.0 for the accumulated total active energy.

## Configuration
The communication with the hardware is done using UART. Therefore you need to have an [UART bus](https://esphome.io/components/uart.html#uart) in your configuration with the `rx_pin` connected to the output of your hardware sensor component. The baud rate usually has to be set to 9600bps.

```yaml
# Example configuration entry

external_components:
  - source:
      type: local
      path: components
    components: [ sml ]

uart:
  id: uart_bus
  tx_pin: GPIO1
  rx_pin: GPIO3
  baud_rate: 9600
  data_bits: 8
  parity: NONE
  stop_bits: 1

sml:
  id: mysml
  uart_id: uart_bus

sensor:
  - platform: sml
    name: "Total energy"
    sml_id: mysml
    server_id: "0123456789abcdef"
    obis_code: "1-0:1.8.0"
    unit_of_measurement: kWh
    accuracy_decimals: 1
    device_class: energy
    state_class: total_increasing
    filters:
      - multiply: 0.0001

  - platform: sml
    name: "Active power"
    sml_id: mysml
    server_id: "0123456789abcdef"
    obis_code: "1-0:15.7.0"
    unit_of_measurement: W
    accuracy_decimals: 1
    filters:
      - multiply: 0.1

text_sensor:
  - platform: sml
    name: "Manufacturer"
    sml_id: mysml
    server_id: "0123456789abcdef"
    obis_code: "129-129:199.130.3"
    format: text

  - platform: sml
    name: "Total energy text"
    sml_id: mysml
    server_id: "0123456789abcdef"
    obis_code: "1-0:1.8.0"
```

## Configuration variables

### SML platform:
- **id** (*Optional*): Specify the ID used for code generation.
- **uart_id** (*Optional*): Manually specify the ID of the [UART Component](https://esphome.io/components/uart.html#uart).

### Sensor
- **obis_code** (*Required*, string): Specify the OBIS code you want to retrieve data for from the device. The format must be (A-B:C.D.E, e.g. 1-0:1.8.0)
- **server_id** (*Optional*, string): Specify the device's server_id to retrieve the OBIS code from. Should be specified if more then one device is connected to the same hardware sensor component.
- **sml_id** (*Optional*): Specify the ID used for code generation.
- All other options from [Sensor](https://esphome.io/components/sensor/index.html#config-sensor).

### Text Sensor
- **obis_code** (*Required*, string): Specify the OBIS code you want to retrieve data for from the device. The format must be (A-B:C.D.E, e.g. 1-0:1.8.0)
- **server_id** (*Optional*, string): Specify the device's server_id to retrieve the OBIS code from. Should be specified if more then one device is connected to the same hardware sensor component.
- **format** (*Optional*): Override the automatic interpretation of the binary data value. Possible values (`int`, `uint`, `bool`, `hex`, `text`).
- **sml_id** (*Optional*): Specify the ID used for code generation.
- All other options from [Sensor](https://esphome.io/components/sensor/index.html#config-sensor).


## Obtaining OBIS codes
The physical values in the transmitted SML telegram are identified by a *server id* and *OBIS codes*. The *server id*
identifies your smart meter. If you have only one hardware component attached to your optical sensor you usually
don't have to care about the server id and you may ommit it in your configuration.

In order to get the server id and the available OBIS codes provided by your smart meter, simply set up the
SML platform and observe the log output (the log level must be set to at least ``debug``!).

Each line in the output represents a combination of the server id (in brackets), the OBIS code and the transmitted hex value
(in square brackets).

## Precision errors
Many smart meters emit very huge numbers for certain OBIS codes (like the accumulated total active energy). This may lead to precision errors for the values reported by the sensor component to ESPHome. This shows in the fact that slightly wrong numbers may be reported to HomeAssistant. This is a result from internal limitations in ESPHome and has nothing to do with the SML component.

If you cannot live with this, you can use the `TextSensor` with an appropriate format to transmit the value as a string to HomeAssistant. On the HomeAssistant side you can define a [Template Sensor](https://www.home-assistant.io/integrations/template/) to cast the value into the appropriate format and do some scaling.

For ESPHome we have:
```yaml
# ESPHome configuration file
text_sensor:
  - platform: sml
    name: "Total energy string"
    obis_code: "1-0:1.8.0"
    format: uint
```

The `format` parameter is optional. If ommited, the SML component will try to guess the correct datatype from the received SML message.

And in HomeAssistant:
```yaml
# Home Assistant configuration.yaml
template:
  - sensor:
      - name: "Total Energy Consumption"
        unit_of_measurement: "kWh"
        state: >
          {% if states('sensor.total_energy_string') == 'unavailable' %}
            {{ states('sensor.total_energy_consumption') }}
          {% else %}
            {{ ((states('sensor.total_energy_string') | float) * 0.0001) | round(2) }}
          {% endif %}
```

Usually the template sensor's value would turn to 0 if the ESP device is unavailable. This results in problems when using the sensor in combination with the [Utility Meter](https://www.home-assistant.io/integrations/utility_meter/) integration.
The state template provided above checks for the sensor's availability and keeps the current state in case of unavailability.
