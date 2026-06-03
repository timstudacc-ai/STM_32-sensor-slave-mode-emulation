/**
 * @file led_ctrl.h
 * @brief onboard LED controller interface for STM32.
 * @details This driver configures and manages the onboard LED (connected to PC13 on the BlackPill)
 *          based on the value written into the I2C register map.
 */

#ifndef LED_CTRL_H
#define LED_CTRL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/**
 * @brief Initialize the LED control module state variables.
 * @note GPIO configurations are handled separately via STM32CubeMX auto-generated code (MX_GPIO_Init).
 */
void LED_Init(void);

/**
 * @brief Read the LED control register and configure the physical pin output.
 * @details Checks the value of register REGMAP_ADDR_LED_CTRL:
 *          - If 0xFF, writes a LOW state to PC13 to turn the LED ON (active-low configuration).
 *          - If 0x00, writes a HIGH state to PC13 to turn the LED OFF.
 * 
 * @param reg_file Pointer to the volatile register map buffer.
 */
void LED_Process(volatile uint8_t *reg_file);

#ifdef __cplusplus
}
#endif

#endif /* LED_CTRL_H */

