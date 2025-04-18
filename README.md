# ESP8266 IoT Temperature Monitor

## About
IoT system using ESP8266 to monitor temperature with NTC thermistors, securely sending data to an MQTT server. Focuses on low power, security, and scalability.

## Features
- Precise temperature sensing via lookup table.
- Secure MQTT with TLS/SSL and authentication.
- Power-saving light sleep on publishers.
- Real-time monitoring with MQTTx.
- Web interface for MQTT broker setup.

## Architecture
- **Two Publishers**: Read thermistor data, publish to MQTT via TLS.
- **One Subscriber**: Computes temperature difference, publishes to MQTT.
- **Communication**: Wi-Fi, MQTT, ESPAsyncWebServer.

## Implementation
- **Publishers**: Wi-Fi connection, sensor data (pin A0) with moving average, publish every ~7s in light sleep. Errors: 99/-99.
- **Subscriber**: Wi-Fi/MQTT (broker.emqx.io:8883) with TLS, EEPROM for broker IP, web config, 1.5s watchdog.
- **Notes**: Subscriber avoids sleep; uses fingerprints for TLS due to memory limits.

## Testing
- **Accuracy**: Lookup table ensures precision.
- **Reliability**: Consistent MQTT data, verified by MQTTx.
- **Power**: Light sleep reduces publisher energy use.
- **Errors**: Flags 99/-99 for sensor issues.

## Limitations
- Wi-Fi takes ~3-4s, delaying publishes.
- Subscriber power higher (no sleep).
- Fingerprint TLS due to memory.

## Contact
Atiye Eyvazlou: AtiyeEyvazlou@gmail.com
