/**
 * @file i2c_slave.h
 * @brief I2C Slave driver interface for STM32.
 * @details This driver configures the STM32 I2C peripheral to act as an I2C Slave
 *          implementing a memory-mapped register file. It interfaces with the STM32 HAL
 *          interrupt-driven sequence APIs to handle read and write requests from the Master.
 */

#ifndef I2C_SLAVE_H
#define I2C_SLAVE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/**
 * @brief Initialize the I2C Slave driver in interrupt-listening mode.
 * @details Sets up internal pointer references to the shared register map array,
 *          stores its size, and enables I2C address listening interrupts.
 * 
 * @param hi2c Pointer to a HAL I2C handle structure (configured in main.c).
 * @param reg_file Pointer to the volatile array representing the register map.
 * @param reg_size Number of registers in the register map.
 * @retval HAL_OK on successful initialization and listening enable, or HAL_ERROR.
 */
HAL_StatusTypeDef I2C_Slave_Init(I2C_HandleTypeDef *hi2c, volatile uint8_t *reg_file, uint8_t reg_size);

#ifdef __cplusplus
}
#endif

#endif /* I2C_SLAVE_H */

