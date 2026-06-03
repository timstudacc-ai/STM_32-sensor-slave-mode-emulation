# I2C Smart Sensor Emulation (Master-Slave Architecture)

[![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-orange.svg)](https://platformio.org/)
[![Master MCU](https://img.shields.io/badge/Master-ATmega328P-red.svg)](https://www.microchip.com/en-us/product/ATmega328p)
[![Slave MCU](https://img.shields.io/badge/Slave-STM32F411CEU6-blue.svg)](https://www.st.com/en/microcontrollers-microprocessors/stm32f411.html)
[![Master Framework](https://img.shields.io/badge/Framework-Bare--Metal_AVR-yellow.svg)](#)
[![Slave Framework](https://img.shields.io/badge/Framework-STM32CubeHAL-brightgreen.svg)](https://www.st.com/en/embedded-software/stm32cube-mcu-mpu-packages.html)
[![Protocol](https://img.shields.io/badge/Protocol-I2C_100kHz-purple.svg)](#)
This repository implements a complete bidirectional I2C communication system where an STM32 microcontroller acts as a memory-mapped smart sensor (I2C Slave), and an AVR microcontroller acts as the controller (I2C Master) polling data and sending commands.

## 1. Hardware Dependencies & Target Silicon

This project utilizes a dual-MCU architecture:
- **Slave Device (Sensor Emulator):**
  - **Microcontroller:** STM32F411CEU6 (ARM Cortex-M4F)
  - **Development Board:** WeAct Black Pill V2.0
  - **Core Clock:** Configured to 96 MHz.
- **Master Device (Controller):**
  - **Microcontroller:** ATmega328P (8-bit AVR)
  - **Development Board:** Arduino Uno / Nano
  - **Core Clock:** 16 MHz.

## 2. Pinout & Wiring Mapping

| Signal | Arduino Master Pin (ATmega328P) | STM32 Slave Pin (F411) | Logic Level | Description |
| :--- | :--- | :--- | :--- | :--- |
| **GND** | `GND` | `GND` | N/A | Common Ground reference (Mandatory) |
| **SDA** | `A4` (or `PC4`) | `PB7` | 3.3V / 5V | I2C Serial Data |
| **SCL** | `A5` (or `PC5`) | `PB6` | 3.3V / 5V | I2C Serial Clock |
| **LED** | N/A | `PC13` | 3.3V | Slave Status LED (Active Low) |
| **BTN** | N/A | `PA0` | 3.3V | User Button (Hardware Debounced) |

> [!WARNING]
> **Logic Level Constraints:** The ATmega328P operates at 5V logic and uses internal pull-ups, while the STM32 operates at 3.3V. Directly connecting 5V to 3.3V pins can destroy the STM32 GPIO drivers. However, pins **PB6 and PB7 on the STM32F411 are natively 5V-tolerant**, making direct connection safe for this specific implementation.

## 3. Architectural Block Diagrams

The system utilizes an asynchronous, non-blocking sequence on the STM32 side, combined with an active polling timeout loop on the Arduino side.

```mermaid
graph TD
    classDef master fill:#6A8CAF,stroke:#141413,stroke-width:1px,color:#ffffff,rx:6px,ry:6px;
    classDef slave fill:#788C5D,stroke:#141413,stroke-width:1px,color:#ffffff,rx:6px,ry:6px;
    classDef hw fill:#D97757,stroke:#141413,stroke-width:1px,color:#ffffff,rx:6px,ry:6px;

    M_APP[Arduino Main Loop<br>Every 1000ms]:::master --> M_DRV[TWI Bare-Metal Driver]:::master
    M_DRV -->|START + SLA+W + RegAddr| I2C_BUS[I2C Physical Bus]:::hw
    
    I2C_BUS -->|Hardware Match| S_HW[STM32 I2C Peripheral]:::slave
    S_HW -->|AddrCallback IT| S_ISR[I2C IT State Machine]:::slave
    
    S_ISR -->|Parse Address| S_REG[Volatile Register Map]:::slave
    
    S_MAIN[STM32 Main Loop]:::slave -->|ADC Read| S_REG
    S_MAIN -->|Button EXTI| S_REG
    
    S_REG -->|Seq_Transmit_IT| I2C_BUS
    I2C_BUS -->|Burst Read (ACK/NACK)| M_DRV
    M_DRV -->|Data Output| PC[PC Serial Monitor via UART]:::master
```

## 4. Toolchain & Build Environment

- **Build System:** PlatformIO
- **Arduino Toolchain (Master):** `avr-gcc` compiler, `avrdude` uploader.
- **STM32 Toolchain (Slave):** `arm-none-eabi-gcc` compiler, STM32Cube HAL Framework, ST-Link v2 hardware debugger.
- **Serial Debugging:** PC Serial Monitor set to **9600 baud**.

## 5. Peripheral Configuration Strategy

We actively mix hardware abstraction levels depending on the constraints of the MCU:
- **Arduino (Master):** Written using **Bare-Metal Register Manipulation** (interacting directly with `TWCR`, `TWSR`, `TWDR`). This avoids the infinite `while()` loop lockups present in the standard Arduino `Wire.h` library by implementing custom hardware state polling with `100ms` strict timeouts and software bus-recovery (bit-banging).
- **STM32 (Slave):** Written using the **STM32 HAL (Hardware Abstraction Layer)**. Uses asynchronous `_IT` (Interrupt) sequential callbacks (`HAL_I2C_Slave_Seq_Receive_IT` and `HAL_I2C_Slave_Seq_Transmit_IT`) to handle complex auto-incrementing register pointer logic without blocking the main CPU.

## 6. Memory Footprint & Resource Utilization

- **Master (ATmega328P):** 
  - **Flash:** ~2.5 KB (due to lightweight bare-metal drivers, avoiding Arduino Core bloat).
  - **RAM:** < 100 Bytes.
- **Slave (STM32F411):** 
  - **Flash:** ~15 KB (Includes HAL library overhead, ADC, and I2C stacks).
  - **RAM:** ~4 KB (Mainly system stack/heap; the `g_reg_file` itself is only 8 bytes).

## 7. Real-Time Performance Benchmarks

- **Bus Speed:** Operating at **100 kHz (Standard Mode)**.
- **Read/Write Latency:** The Master can execute a 3-byte burst read sequence in approximately ~0.4 milliseconds.
- **Robustness:** The Master incorporates an active timeout (`TWI_TIMEOUT_MS`). If the slave freezes and holds SDA low, the Master automatically drops to GPIO bit-banging to clock 9 pulses and clear the deadlocked bus, ensuring 100% uptime for the primary controller.

## 8. Power Consumption & Operating Conditions

- **Operating Voltages:** 
  - Arduino: 5.0V Input/Logic.
  - STM32: 3.3V Input/Logic.
- **Current Draw:**
  - Arduino Master: ~15-20 mA active.
  - STM32 Slave: ~20-25 mA active (ADC continuously sampled, CPU in RUN mode).
- **Watchdog:** The STM32 utilizes an Independent Watchdog (IWDG) timeout window. If high-priority I2C traffic prevents the main loop from executing, the watchdog forces a hardware reset, recovering the slave from interrupt starvation.
