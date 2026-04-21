# Modbus RTU to MQTT Bridge with Real-Time Monitoring Dashboard

This project implements an industrial data acquisition pipeline that reads sensor data over Modbus RTU and publishes it to an MQTT broker for real-time visualization.

The system demonstrates a practical integration of embedded communication, a data processing and communication layer, and live monitoring through a web-based dashboard powered by Node.js.

---

## Overview

The application reads temperature and humidity values from a Modbus RTU device via RS485 and publishes them as JSON messages to an MQTT broker. These messages are consumed by a Node.js server and visualized in real time in a browser-based dashboard.

---

## System Architecture

STM32 Sensor Device
→ Modbus RTU (RS485)
→ JavaFX Application (Bridge)
→ MQTT Broker (Mosquitto)
→ Node.js Dashboard Server
→ Web Dashboard (Browser)

---

## Features

* Modbus RTU communication using Function Code 0x04
* Bulk Modbus register reading with targeted data extraction
* MQTT publishing with structured JSON payload
* JavaFX-based graphical user interface
* Node.js-based real-time dashboard integration
* Real-time data updates via MQTT and WebSocket (Socket.IO)
* Manual and automatic data publishing modes
* System logging and debugging interface

---

## Node.js Dashboard

A lightweight Node.js-based web server is used to visualize MQTT data in real time.

The dashboard subscribes to MQTT topics and displays incoming sensor data such as temperature and humidity.

### Features

* Real-time data updates via MQTT and WebSocket (Socket.IO)
* JSON payload parsing and visualization
* Simple and responsive UI

### How it works

1. JavaFX application publishes sensor data to MQTT
2. Node.js server subscribes to the same topic
3. Incoming data is pushed to the browser via Socket.IO
4. Dashboard updates instantly

---

## Data Flow

1. Sensor data is read via Modbus RTU (RS485)
2. JavaFX application processes the data
3. Data is published to MQTT broker
4. Node.js server subscribes to MQTT topic
5. Data is pushed to the web dashboard in real time

---

## REST API Endpoints

The Node.js backend exposes REST APIs for accessing real-time sensor data and system status.

---

### GET /api/health

Returns system and MQTT connection status.

<p align="center">
  <a href="https://github.com/Levent98/Realtime_detector/blob/feature/mqtt-integration/images/api_health.png">
    <img src="https://raw.githubusercontent.com/Levent98/Realtime_detector/feature/mqtt-integration/images/api_health.png" width="700"/>
  </a>
</p>

---

### GET /api/sensor/latest

Returns the latest sensor data received via MQTT.

<p align="center">
  <a href="https://github.com/Levent98/Realtime_detector/blob/feature/mqtt-integration/images/api_sensorlatest.png">
    <img src="https://raw.githubusercontent.com/Levent98/Realtime_detector/feature/mqtt-integration/images/api_sensorlatest.png" width="700"/>
  </a>
</p>

---

### GET /api/messages

Returns recent MQTT messages history.

<p align="center">
  <a href="https://github.com/Levent98/Realtime_detector/blob/feature/mqtt-integration/images/api_messages.png">
    <img src="https://raw.githubusercontent.com/Levent98/Realtime_detector/feature/mqtt-integration/images/api_messages.png" width="700"/>
  </a>
</p>

## User Interface

### Modbus Configuration Panel

<p align="center">
  <a href="https://github.com/Levent98/Realtime_detector/blob/feature/mqtt-integration/images/Modbus_Configuration_ Panel.jpg">
    <img src="https://raw.githubusercontent.com/Levent98/Realtime_detector/feature/mqtt-integration/images/Modbus_Configuration_ Panel.jpg" width="700"/>
  </a>
</p>

---

### MQTT Communication Panel

<p align="center">
  <a href="https://github.com/Levent98/Realtime_detector/blob/feature/mqtt-integration/images/MQTT_Communication_Panel.jpg">
    <img src="https://raw.githubusercontent.com/Levent98/Realtime_detector/feature/mqtt-integration/images/MQTT_Communication_Panel.jpg" width="700"/>
  </a>
</p>

---

### Real-Time Monitoring Dashboard

<p align="center">
  <a href="https://github.com/Levent98/Realtime_detector/blob/feature/mqtt-integration/images/Real-Time_Monitoring_Dashboard.png">
    <img src="https://raw.githubusercontent.com/Levent98/Realtime_detector/feature/mqtt-integration/images/Real-Time_Monitoring_Dashboard.png" width="700"/>
  </a>
</p>

---

### System Log Panel

<p align="center">
  <a href="https://github.com/Levent98/Realtime_detector/blob/feature/mqtt-integration/images/System_Log_Panel.jpg">
    <img src="https://raw.githubusercontent.com/Levent98/Realtime_detector/feature/mqtt-integration/images/System_Log_Panel.jpg" width="700"/>
  </a>
</p>

---

## MQTT Payload Format

```json
{
  "temperature": 21.1,
  "humidity": 37.4,
  "timestamp": "2026-04-20T16:50:15"
}
```

Note: The decimal separator must be a dot (.) to ensure valid JSON parsing across systems.

---

## Configuration

### MQTT

* Broker: tcp://localhost:1883
* Topic: prosense/device1/data

---

### Modbus

* Function Code: 0x04
* Start Address: configurable
* Register Count: configurable
* Slave ID: device-specific

---

## Data Extraction Logic

Sensor values are stored in specific Modbus input registers:

* Humidity: 0x06
* Temperature: 0x0B

When performing bulk register reads, values are extracted using:

```
offset = (targetRegister - startAddress) * 2
```

Each register consists of 2 bytes and values are scaled by a factor of 10.

---

## Modes of Operation

### Manual Mode

The user triggers a single data acquisition and publish cycle using the "Read & Publish" action.

---

### Automatic Mode

The system continuously reads and publishes sensor data at fixed intervals when "Auto Start" is enabled.
The process can be stopped using "Auto Stop".

---

## Installation

### 1. Clone the repository

```bash
git clone https://github.com/Levent98/Realtime_detector.git
cd Realtime_detector
```

---

### 2. Start MQTT Broker

```bash
cd "C:\Program Files\Mosquitto"
mosquitto -v
```

---

### 3. Start Node.js Dashboard

```bash
cd mqtt_dashboard_server
npm install
npm start
```

Open in browser:
http://localhost:3000

---

### 4. Run Application

Open the project in IntelliJ IDEA and run:

Launcher.java

---

## Troubleshooting

### Timeout Error

"The read operation timed out before any data was returned"

Possible causes:

* Incorrect Modbus configuration
* Invalid register range
* Device not responding

---

### Dashboard Not Updating

* Check JSON format (decimal must be ".")
* Verify MQTT topic
* Ensure broker connection is active

---

### Mosquitto Port Error

"Only one usage of each socket address"

This indicates that the broker is already running.

---

## Author

Levent Keskin
Embedded Software Engineer

---

## Notes

This project demonstrates a complete industrial IoT workflow, combining embedded communication, data acquisition, protocol integration, backend processing, and real-time visualization.
