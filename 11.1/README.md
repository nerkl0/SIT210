# SmartBath

SmartBath is an automated bath-filling system controlled from your phone. Set a target temperature and bath size in the web app and start the fill: the system draws water, blends hot and cold to reach the target temperature, and tracks the fill level to stop at the set volume. A live progress readout shows how far along the fill is, and temperature problems or faults are signalled on the OLED display, via status LEDs, and with an audible buzzer.

## Contents

- [Hardware](#hardware)
- [Features](#features)
- [System Architecture](#system-architecture)
  - [Firmware (Arduino / FreeRTOS)](#firmware-arduino--freertos)
  - [Drivers / Header Files](#drivers--header-files)
  - [Web Application](#web-application)
- [MQTT Topics](#mqtt-topics)
- [Bath State Machine](#bath-state-machine)
- [Getting Started](#getting-started)
  - [Requirements](#requirements)
  - [Firmware](#firmware)
  - [Web App](#web-app)
- [Known Issues / Limitations](#known-issues--limitations)
- [Safety Notice](#safety-notice)


## Hardware

| Hardware | Model | Total | Notes |
|----------|-------|-------|-------|
| Microcontroller | Arduino Nano 33 IoT (SAMD21 w/ WiFiNINA) | 1 | |
| Water pump | 12V | 2 | Hot: ENA, IN/OUT 1-2. Cold: ENB, IN/OUT 3-4 |
| Motor driver | Dual H-bridge L298N | 1 | Can use a relay module or MOSFET if not using PWM |
| Temperature sensor | DS18B20 (1-Wire) | 1 | |
| OLED display | SSD1306 (I²C) | 1 | |
| Red LED | | 1 | Hard Warning / Sensor Fault |
| Yellow LED | | 1 | Soft Warning / Lost Connection |
| Buzzer | Piezo | 1 | |

Pin assignments are defined in `Config.h` and the controller source files.

## Features

- **Remote start/stop** of bath filling via a mobile-friendly web app.
- **Target temperature control** with live adjustment while the bath is filling.
- **Configurable bath size** converted to a fill duration from a known pump flow rate.
- **Add more water** for topping up without a fixed target volume.
- **Live status reporting** of bath state, current water temperature and fill progress in the web app.
- **Safety alerts** visual (LEDs + OLED), audible (buzzer) and in-app for high temperature, sensor faults and lost connection.
- **Silenceable warnings** to silence the buzzer on a sensor fault or hard warning
- **Resilient connectivity** with automatic Wi-Fi and MQTT reconnection, plus a Last-Will-and-Testament (LWT) so the app knows when the bath drops offline.

## System Architecture

### Firmware (Arduino / FreeRTOS)

Runs on an Arduino SAMD21-based board, using FreeRTOS for concurrency across three tasks:

| Task | Priority | Timing | Responsibility |
|------|----------|--------|----------------|
| `highPriorityTask` | High | 250 ms | Reads the temperature sensor (every 750 ms), evaluates bath state, drives pumps and updates the LEDs and buzzer. |
| `normalPriorityTask` | Normal | 250 ms | Refreshes the OLED display and publishes status to MQTT (every 2000 ms). |
| `generalTask` | Normal | 100 ms | Services the MQTT client loop and handles reconnection. |

### Drivers / Header Files

- **`BathController`** the state machine. Determines the bath state (`IDLE`, `FILLING`, `SOFT_WARNING`, `HARD_WARNING`, `LOST_CONNECTION`, `SENSOR_FAULT`) and decides the resulting pump actions.
- **`PumpController`** low-level control of the hot and cold pumps via an H-bridge.
- **`TempSensor`** reads a DS18B20 1-Wire sensor and tracks its health, reporting when the sensor is faulty.
- **`Display`** drives the SSD1306 OLED over I²C.
- **`AppManager`** owns the Wi-Fi and MQTT logic: connection, subscriptions, message handling, and status publishing.
- **`Config`** pin assignments and buzzer settings.

### Web Application

A simple, intuitive interface:

- **`index.html` / `style.css`** Home and Notifications pages, with controls for temperature, bath size, start/stop, add more water and warning silencing.
- **`server.js`** MQTT.js client logic; connects to the broker over secure WebSockets.
- **`app.js`** application logic; maintains UI state, parses incoming status messages and renders notifications.

## MQTT Topics

All topics live under the `smartbath/` namespace.

### Published by the firmware

| Topic | Payload | Notes |
|-------|---------|-------|
| `smartbath/status` | JSON `{ "state", "temp", "progress" }` | All bath status published in one push |
| `smartbath/status/connected` | `"True"` / `"False"` | Retained by the MQTT LWT |

### Sent by the app (commands)

| Topic | Payload | Action |
|-------|---------|--------|
| `smartbath/command/start` | target temperature | Begin filling |
| `smartbath/command/stop` | `{}` | Stop filling |
| `smartbath/command/temp` | target temperature | Adjust target mid-fill |
| `smartbath/command/size` | litres | Set bath volume |
| `smartbath/command/silence` | `{}` | Silence the warning buzzer |

---

## Bath State Machine

The controller evaluates a new state on every high-priority cycle:

- **`IDLE`** default state; both pumps off.
- **`FILLING`** within the target temperature band; both pumps running.
- **`SOFT_WARNING`** temperature drifting outside the band; the controller corrects by running only the hot or cold pump as needed.
- **`HARD_WARNING`** temperature too high or far out of range; red LED and buzzer fire.
- **`SENSOR_FAULT`** the temperature sensor has detected a fault; filling halts.
- **`LOST_CONNECTION`** connectivity with the broker has been lost; filling halts.

Filling also ends automatically once the fill duration has elapses.

## Getting Started

### MQTT Broker

Both the firmware and the web app connect to an MQTT broker, which relays all commands and status between them. You'll need a broker (e.g. [Mosquitto](https://mosquitto.org/), [EMQX](https://www.emqx.io/), or a hosted service such as HiveMQ Cloud) configured with two listeners:

- **TLS (port 8883)** used by the firmware via `PubSubClient`.
- **Secure WebSockets / WSS (port 8884)** used by the web app via MQTT.js in the browser.

Create an MQTT user and password for the system to authenticate with and use those credentials in the secrets files below. The ports above match the defaults in this project; adjust them to your broker if needed.

### Requirements

- `FreeRTOS_SAMD21`
- `WiFiNINA`
- `PubSubClient`
- `OneWire` and `DallasTemperature`
- `U8g2`
- `MQTT.js`

### Firmware

1. Install the Arduino IDE (or `arduino-cli`) with SAMD board support and the libraries listed above.
2. Create an `arduino_secrets.h` file alongside the firmware defining your credentials:
   ```cpp
   #define WIFI_SSID "your-ssid"
   #define WIFI_PASS "your-password"
   #define MQTT_BROKER "broker.example.com"
   #define MQTT_PORT 8883
   #define MQTT_CLIENT "smartbath"
   #define MQTT_USER "user"
   #define MQTT_PASS "pass"
   ```
3. Open `src.ino`, select your board and upload.

### Web App

1. Provide an `env_secrets.js` and export your broker details
   ```js
   export const HOST = "broker.example.com";
   export const PORT = 8884;
   export const USER = "user";
   export const PASS = "password";
   export const CLIENTID = "smartbath-web";
   ```
2. Install dependencies
   ```
    npm init -y && npm install
   ```
3. Open `index.html` in a browser.

## Known Issues / Limitations

**Notification spam**
<br>While the bath is filling, adjusting the temperature sends one notification per degree increment. Fine for 1–2 degrees, but excessive beyond that.

**Warning state while idle**
<br>Temperature and sensor warnings before filling starts are not yet handled. The state is tracked during and after filling, but not before.

## Safety Notice

This system controls running water and heating. It is a prototype project and is not a certified safety device. Always supervise filling, verify the temperature independently, and never rely solely on the software to prevent scalding or overflow.