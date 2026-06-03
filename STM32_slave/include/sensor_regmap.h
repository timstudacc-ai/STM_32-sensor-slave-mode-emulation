/**
 * @file sensor_regmap.h
 * @brief Shared I2C register map configuration for the Master-Slave communication.
 * @details This file defines the I2C slave address, register offsets, default values,
 *          and status/control bit masks. Both the Arduino Master and STM32 Slave must
 *          agree on these values to ensure correct and synchronous communication.
 *
 * Register Layout:
 * - 0x00 [RO] : Device ID (should read 0xA5)
 * - 0x01 [RO] : Firmware Version (0x01)
 * - 0x02 [RO] : Raw Temperature High Byte
 * - 0x03 [RO] : Raw Temperature Low Byte
 * - 0x04 [RO] : Calculated Temperature in degrees Celsius (signed 8-bit integer)
 * - 0x05 [RO] : Button Press Counter (increments on slave button falling edge)
 * - 0x06 [RW] : LED Control (0xFF for ON, 0x00 for OFF)
 * - 0x07 [RO] : System Status register (bit flags for ADC status and I2C errors)
 */

#ifndef SENSOR_REGMAP_H
#define SENSOR_REGMAP_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

/**
 * @brief 7-bit I2C Slave Address assigned to the STM32 board.
 * @note In standard 7-bit addressing, this value is shifted left by 1 bit by the hardware
 *       to form the SLA+W (write) or SLA+R (read) byte (0x84 or 0x85 respectively).
 */
#define SENSOR_I2C_ADDR 0x42U

/**
 * @brief Total size of the register map file in bytes.
 */
#define REGMAP_SIZE 8U

/* --- REGISTER ADDRESSES --- */

/** @brief Device Identification Register (Read-Only) */
#define REGMAP_ADDR_DEV_ID 0x00U

/** @brief Firmware Version Register (Read-Only) */
#define REGMAP_ADDR_FW_VERSION 0x01U

/** @brief Raw Temperature Value - High Byte (Read-Only) */
#define REGMAP_ADDR_TEMP_RAW_H 0x02U

/** @brief Raw Temperature Value - Low Byte (Read-Only) */
#define REGMAP_ADDR_TEMP_RAW_L 0x03U

/** @brief Calculated Temperature in °C - 8-bit Signed Int (Read-Only) */
#define REGMAP_ADDR_TEMP_DEG_C 0x04U

/** @brief Button Press Counter (Read-Only) */
#define REGMAP_ADDR_BTN_COUNT 0x05U

/** @brief LED Control Register (Read-Write). Write 0xFF to turn on the LED, 0x00 to turn off. */
#define REGMAP_ADDR_LED_CTRL 0x06U

/** @brief System Status Register (Read-Only) */
#define REGMAP_ADDR_STATUS 0x07U


/* --- DEFAULT VALUES & CONSTANTS --- */

/** @brief Expected Device ID returned by 0x00 to verify proper communication. */
#define REGMAP_DEV_ID_VALUE 0xA5U

/** @brief Firmware version number. */
#define REGMAP_FW_VERSION_VALUE 0x01U


/* --- CONTROL & STATUS BIT MASKS --- */

/** @brief LED control value representing the "ON" state. */
#define REGMAP_LED_CTRL_ON_VALUE 0xFFU

/** @brief LED control value representing the "OFF" state. */
#define REGMAP_LED_CTRL_OFF_VALUE 0x00U

/** @brief LED control mask (useful for bitwise toggling/masking). */
#define REGMAP_LED_CTRL_ON_MASK 0xFFU

/**
 * @brief Status bit mask: ADC error flag.
 * @details If set (1), indicates that the ADC peripheral failed to complete its conversion.
 */
#define REGMAP_STATUS_ADC_RDY_MASK 0x01U

/**
 * @brief Status bit mask: I2C transmission error flag.
 * @details If set (1), indicates that the I2C bus experienced a hardware error or unexpected condition.
 */
#define REGMAP_STATUS_I2C_ERR_MASK 0x02U

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_REGMAP_H */
