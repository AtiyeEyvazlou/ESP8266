# ESP8266 IoT Temperature Monitoring System

## About
This project implements an IoT temperature monitoring system using ESP8266 microcontrollers. It collects temperature data from NTC thermistors, processes it, and securely transmits it to an MQTT server for real-time monitoring. The system is designed for low power consumption, secure communication, and scalability.

## Features
- **Accurate Sensing**: NTC thermistors with lookup table for precise temperature measurements.
- **Secure Communication**: TLS/SSL encryption and MQTT with username/password authentication.
- **Power Efficiency**: Light sleep mode on publishers to reduce energy usage.
- **Real-Time Monitoring**: Data visualization via MQTT clients (e.g., MQTTx).
- **Configurable**: Web interface to set MQTT broker IP.

## System Architecture
- **Two Publishers (ESP8266)**:
  - Read temperature from NTC thermistors.
  - Publish data to MQTT server over secure TLS.
- **One Subscriber (ESP8266)**:
  - Receives temperature data.
  - Computes absolute temperature difference.
  - Publishes results to MQTT topic.
- **Communication**:
  - Wi-Fi for connectivity.
  - MQTT protocol for data exchange.
  - ESPAsyncWebServer for broker IP configuration.

## Implementation
- **Publishers**:
  - Connect to Wi-Fi in station mode.
  - Read sensor data (pin A0), apply moving average filter, convert to Celsius via lookup table.
  - Publish data every ~7s using light sleep mode.
  - Return 99/-99 for out-of-range values.
- **Subscriber**:
  - Connects to Wi-Fi and MQTT broker (e.g., broker.emqx.io:8883) with TLS/SSL (fingerprint-based).
  - Stores broker IP in EEPROM, configurable via web interface (http://[IP]/).
  - Computes and publishes temperature differences.
  - Uses 1.5s watchdog timer to prevent hangs.
- **Security**:
  - TLS/SSL encryption.
  - Username/password authentication (e.g., "emqx"/"public").
- **Notes**:
  - Subscriber avoids sleep mode for continuous subscriptions.
  - Wi-Fi/MQTT reconnection ensures reliability.
  - Fingerprint-based TLS due to memory limits.

## Installation
1. **Clone Repository**:
   ```bash
   git clone https://github.com/AtiyeEyvazlou/ESP8266.git
