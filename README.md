

---

# Realtime Detector (STM32 Based Industrial Sensor System)

## Overview

This project implements a bare-metal embedded firmware for a real-time industrial temperature and humidity monitoring system based on the STM32 platform. The system is designed for reliability, deterministic timing, and industrial communication compatibility.

It integrates sensor acquisition, Modbus RTU communication over RS485, LCD output, and robust error handling mechanisms.

---

## Features

* Bare-metal firmware (no HAL or vendor libraries)
* Deterministic real-time execution
* Modbus RTU communication over RS485
* I2C-based temperature and humidity sensing (SHT3x)
* LCD interface for real-time display
* DMA-based UART communication
* Watchdog-based fail-safe system
* Median filtering for sensor data stability
* Error detection and recovery mechanisms

---

## System Architecture

The system is structured into multiple layers:

### Application Layer

* Sensor data processing
* Modbus register management
* LCD updates
* State machine for periodic tasks

### Communication Layer

* Modbus RTU protocol implementation
* Frame parsing and CRC validation
* RS485 direction control

### Driver Layer

* UART (with DMA)
* I2C (custom implementation)
* ADC
* GPIO
* Timer (TIM6 for microsecond timing)

### Hardware Layer

* STM32F4 microcontroller
* SHT3x sensor
* RS485 transceiver
* LCD (HD44780 compatible)

---

## Hardware Configuration

### UART (RS485)

* TX: PA9
* RX: PA10
* DE (Driver Enable): PA8
* DMA used for TX and RX

### I2C (Sensor)

* SCL: PB6
* SDA: PB7
* Sensor Address: 0x44 (SHT3x)

### LCD

* RS: PC1
* RW: PC0
* E: PC2
* Data Pins: PC13, PC14, PC15, PB8

---

## Timing and Performance

* System Clock: 24 MHz
* Main loop optimized for deterministic execution
* Typical loop cycle: approximately 90,000 cycles
* Worst case observed: over 150,000 cycles
* I2C fault scenarios handled with recovery logic

---

## Communication Protocol

### Modbus RTU

* Standard Modbus RTU frame structure
* CRC16 using polynomial 0xA001
* Supported operations:

  * Read Holding Registers
  * Write Multiple Registers
* Frame detection based on 3.5 character idle time

---

## Sensor Handling

* Sensor: SHT3x (I2C)
* Command used: 0x240B (Medium repeatability)
* Non-blocking state machine design
* Median filter (size 18) applied for noise reduction

---

## Error Handling

* I2C timeout and bus error detection
* Bus recovery mechanism:

  * Manual clock pulses on SCL
  * Peripheral reset and reinitialization
* CRC validation for Modbus frames
* Communication timeout detection

---

## Watchdog Strategy

* Independent Watchdog (IWDG)
* Fed through controlled state transitions
* Prevents system lock in case of:

  * I2C freeze
  * Communication deadlock
  * Unexpected infinite loops

---

## Project Structure

```
Core/
  Src/
    main.c
    uart.c
    i2c.c
    modbus.c
    lcd.c
    sensor.c
    error_check.c
  Inc/
    *.h

Drivers/
  Low-level peripheral drivers

Utilities/
  CRC, filtering, helper functions
```

---

## Build and Toolchain

* IDE: Keil uVision (ARMClang)
* Target: STM32F4 Series
* Language: C (C99)
* No external libraries used

---

## Design Decisions

* Register-level programming for full hardware control
* DMA used to reduce CPU load
* Ring buffer and frame queue for Modbus handling
* Clear separation of communication and application logic
* Minimal blocking operations

---

## Future Improvements

* Optional FreeRTOS integration (requires careful evaluation due to potential task scheduling latency)
* Advanced fault logging (Flash or SD card) (planned feature)
* Improved SCADA integration
* Configurable Modbus register mapping
* Bootloader support (planned feature)

---

## Author

Levent Keskin

Embedded Software Engineer

---

