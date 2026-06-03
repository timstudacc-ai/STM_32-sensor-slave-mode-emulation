/**
 * @file twi.h
 * @brief Two-Wire Interface (TWI/I2C) Master driver for AVR.
 * @details This driver provides the lower-level register control needed to initialize
 *          the AVR TWI hardware module, handle start/stop conditions, write/read bytes,
 *          and implement a software bus-recovery algorithm for stuck lines.
 */

#ifndef TWI_H
#define TWI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief TWI Communication Status and Error Codes.
 */
typedef enum {
    TWI_OK = 0,               /**< 0: Operation completed successfully */
    TWI_ERROR_START,          /**< 1: Failed to transmit START condition */
    TWI_ERROR_SLA_W_NACK,     /**< 2: Slave rejected SLA+W (Write Address) with a NACK */
    TWI_ERROR_SLA_R_NACK,     /**< 3: Slave rejected SLA+R (Read Address) with a NACK */
    TWI_ERROR_DATA_NACK,      /**< 4: Slave rejected a transmitted data byte with a NACK */
    TWI_ERROR_TIMEOUT,        /**< 5: Operation timed out waiting for TWINT hardware flag */
    TWI_ERROR_BUS             /**< 6: General TWI bus error or collision detected */
} TWI_Status_t;

/**
 * @brief Initialize the TWI (I2C) hardware module in Master Mode.
 * @details Configures the bit rate generator (TWBR and TWSR registers) for a 100 kHz bus
 *          frequency (Standard Mode) at 16 MHz CPU frequency. Enables the TWI circuitry.
 */
void TWI_Init(void);

/**
 * @brief Read a sequence of registers from an I2C slave device.
 * @details Performs a standard I2C read cycle:
 *          1. Send START condition.
 *          2. Send Slave Address + Write (SLA+W) bit.
 *          3. Send Target Register Address.
 *          4. Send Repeated START (Sr) condition.
 *          5. Send Slave Address + Read (SLA+R) bit.
 *          6. Receive `len` bytes from the slave (ACKing each byte, and NACKing the final byte).
 *          7. Send STOP condition.
 * 
 * @param dev_addr 7-bit I2C address of the target slave (e.g., SENSOR_I2C_ADDR).
 * @param reg_addr Starting register address within the slave to read from.
 * @param buf Pointer to a memory buffer where the read bytes will be stored.
 * @param len Total number of bytes to read (burst read).
 * @return TWI_OK on success, or an error code from TWI_Status_t on failure.
 */
TWI_Status_t TWI_ReadRegister(uint8_t dev_addr, uint8_t reg_addr, uint8_t *buf, uint8_t len);

/**
 * @brief Write a data byte to a specific register on an I2C slave device.
 * @details Performs a standard I2C write cycle:
 *          1. Send START condition.
 *          2. Send Slave Address + Write (SLA+W) bit.
 *          3. Send Target Register Address.
 *          4. Send Data Byte.
 *          5. Send STOP condition.
 * 
 * @param dev_addr 7-bit I2C address of the target slave (e.g., SENSOR_I2C_ADDR).
 * @param reg_addr Target register address within the slave to write to.
 * @param data Data byte to write to the register.
 * @return TWI_OK on success, or an error code from TWI_Status_t on failure.
 */
TWI_Status_t TWI_WriteRegister(uint8_t dev_addr, uint8_t reg_addr, uint8_t data);

#ifdef __cplusplus
}
#endif

#endif /* TWI_H */

