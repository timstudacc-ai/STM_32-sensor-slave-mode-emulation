/**
 * @file button.h
 * @brief User Button interface for STM32.
 * @details This driver manages input state tracking and debouncing for the onboard
 *          user button (connected to PA0 on the BlackPill). It counts button presses
 *          using an EXTI (External Interrupt) falling-edge trigger and updates the register map.
 */

#ifndef BUTTON_H
#define BUTTON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/**
 * @brief Initialize the button driver state.
 * @details Clears the interrupt event flag and sets the initial debounce timestamp.
 * @note GPIO pin and EXTI configurations are handled by CubeMX (MX_GPIO_Init).
 */
void Button_Init(void);

/**
 * @brief Process button debouncing and update the registers.
 * @details Checks if a button edge interrupt has occurred:
 *          1. Debounces the input using software millisecond timers (50ms delay).
 *          2. Verifies that the pin is still pressed (active-low state validation).
 *          3. Increments the button press counter register (REGMAP_ADDR_BTN_COUNT)
 *             atomically using critical sections.
 *          4. Clears the event flag to wait for the next interrupt.
 * 
 * @param reg_file Pointer to the volatile register map buffer.
 */
void Button_Process(volatile uint8_t *reg_file);

/**
 * @brief EXTI interrupt wrapper function.
 * @details Sets the event flag from interrupt context when the button is pressed.
 *          Must be called by the HAL EXTI callback handler.
 */
void Button_EXTI_Callback(void);

#ifdef __cplusplus
}
#endif

#endif /* BUTTON_H */

