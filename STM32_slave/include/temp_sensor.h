/**
 * @file temp_sensor.h
 * @brief Internal Temperature Sensor driver interface for STM32.
 * @details This module handles reading the STM32F411 internal temperature channel
 *          using the ADC1 peripheral. It converts the raw ADC value to degrees
 *          Celsius using highly optimized integer math and updates the I2C register map.
 */

#ifndef TEMP_SENSOR_H
#define TEMP_SENSOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/**
 * @brief Initialize the Temperature Sensor driver.
 * @details Saves a pointer to the ADC handle (configured in main.c) and records the
 *          initial system uptime timestamp.
 * 
 * @param hadc Pointer to the ADC_HandleTypeDef structure (configured for internal channel).
 * @retval HAL_OK on success, or HAL_ERROR.
 */
HAL_StatusTypeDef TempSensor_Init(ADC_HandleTypeDef *hadc);

/**
 * @brief Poll the internal temperature sensor and update the register map.
 * @details Checks if the polling interval (500ms) has expired. If so:
 *          1. Starts an ADC conversion on the internal temperature channel.
 *          2. Waits for completion (blocking, with a short timeout).
 *          3. Computes the raw chip temperature.
 *          4. Converts the raw value to millivolts and then to °C using integer math.
 *          5. Writes the raw temperature (16-bit) and calculated value (8-bit signed)
 *             to the shared register file in a critical section.
 * 
 * @param reg_file Pointer to the volatile register map buffer.
 * @retval HAL_OK on success, or HAL_ERROR.
 */
HAL_StatusTypeDef TempSensor_Process(volatile uint8_t *reg_file);

#ifdef __cplusplus
}
#endif

#endif /* TEMP_SENSOR_H */

