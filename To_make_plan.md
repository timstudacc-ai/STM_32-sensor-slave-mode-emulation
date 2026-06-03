**Role:**
Act as a Senior Embedded Firmware Architect specializing in heterogeneous microcontroller systems, I2C protocol implementation, and bare-metal C programming.

**Project Context:**
I am building a portfolio project called "Smart Sensor Emulator." The goal is to emulate an I2C sensor using an STM32 as the Slave device, which will be queried and controlled by an Arduino Nano (ATmega328P) acting as the Master. 

**Technical Stack & Requirements:**
* **Master (Arduino Nano / ATmega328P):** Must be written in pure C using `avr-libc` (bare-metal register manipulation for the TWI peripheral). Absolutely NO Arduino framework or `Wire.h` libraries are allowed.
* **Slave (STM32):** Must be written using STM32CubeHAL. It acts as an I2C Slave with an internal memory array simulating "sensor registers."
* **Emulated Registers:** The STM32 must emulate a memory map containing:
    1.  Device ID (Read-Only).
    2.  Internal MCU Temperature (Read-Only, fetched via internal ADC).
    3.  Button Press Counter (Read-Only, incremented via hardware button with debouncing).
    4.  LED Status (Read/Write, writing to this register physically toggles the LED on the STM32).
* **Toolchain:** The project will be built using PlatformIO.

**Action & Chain of Thought:**
Please provide a complete implementation plan by thinking through the following steps:
1.  *Project Structure:* Briefly define the folder structure for PlatformIO, clearly separating the workspace into two distinct projects (one for AVR, one for ARM).
2.  *Memory Map Design:* Create a clear and logical I2C register map (address, size, R/W access, description) for the emulated sensor.
3.  *STM32 Slave Implementation:* Provide the C code for the STM32 (`main.c` and interrupt callbacks). Explain how to handle the HAL I2C Slave callbacks (`HAL_I2C_ListenCpltCallback`, `HAL_I2C_AddrCallback`, etc.) to properly simulate reading from and writing to specific memory addresses.
4.  *ATmega328P Master Implementation:* Provide the pure C code (`main.c`) for the ATmega328P. Implement non-blocking or timeout-protected bare-metal TWI functions (`twi_start`, `twi_write`, `twi_read`) to interact with the STM32's memory map.
5.  *Main Loop Example:* Write a simple loop for the Master that reads the temperature, reads the button counter, and toggles the STM32's LED.

**Operational Constraints:**
* Provide robust, production-like code 
* Ensure the I2C Master code handles potential bus lockups gracefully (e.g., using timeouts instead of infinite `while` loops on TWI status registers).
* The output format must be a well-structured Markdown document with language-specific code blocks.