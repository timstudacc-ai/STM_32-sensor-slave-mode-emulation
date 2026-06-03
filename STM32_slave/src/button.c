/**
 * @file button.c
 * @brief User Button implementation for STM32.
 * @details This file handles user button inputs using External Interrupts (EXTI)
 *          on Pin PA0. It uses software-based debounce timing to filter noisy
 *          switch contacts before safely incrementing the button counter.
 */

#include "button.h"
#include "sensor_regmap.h"

/**
 * @brief Debounce interval in milliseconds.
 * @details Filters out physical high-frequency switch bounces that occur during press/release.
 */
#define DEBOUNCE_DELAY_MS 50U

/**
 * @brief Button event flag.
 * @details Marked volatile since it is set inside the EXTI interrupt handler (ISR)
 *          and read/cleared in the main loop (thread context).
 */
static volatile uint8_t g_button_event_flag = 0U;

/** @brief Stores the millisecond timestamp of the last validated button press. */
static uint32_t g_last_press_time = 0U;

void Button_Init(void)
{
    g_button_event_flag = 0U;
    g_last_press_time = 0U;
}

void Button_Process(volatile uint8_t *reg_file)
{
    /* Check if the EXTI interrupt handler set the event flag */
    if (g_button_event_flag == 1U)
    {
        uint32_t current_time = HAL_GetTick();
        
        /* 
         * Software Debouncing Check:
         * Verify if the time elapsed since the last valid button press is greater
         * than the debounce delay (50ms). This ignores contact bounces.
         */
        if ((current_time - g_last_press_time) >= DEBOUNCE_DELAY_MS)
        {
            /* 
             * Pin State Verification:
             * On the BlackPill board, the key button is active-low (connected to PA0).
             * When pressed, the pin reads GPIO_PIN_RESET (0V).
             */
            if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET)
            {
                if (reg_file != NULL)
                {
                    /* 
                     * --- CRITICAL SECTION ENTRY ---
                     * The I2C callback could fire during this read-modify-write operation
                     * (which is not atomic on 32-bit registers for memory increments).
                     * We temporarily disable interrupts globally to protect register integrity.
                     */
                    __disable_irq();
                    reg_file[REGMAP_ADDR_BTN_COUNT]++;
                    __enable_irq(); /* --- CRITICAL SECTION EXIT --- */
                }
            }
            g_last_press_time = current_time; /* Save the last valid press timestamp */
        }
        
        /* Clear the interrupt trigger flag to prepare for the next press event */
        g_button_event_flag = 0U;
    }
}

void Button_EXTI_Callback(void)
{
    /* Set the event flag. Processing is deferred to the main loop to keep the ISR short. */
    g_button_event_flag = 1U;
}

/**
 * @brief Overridden HAL GPIO External Interrupt Callback.
 * @details This function overrides the weak definition in the STM32 HAL library.
 *          It checks which GPIO pin triggered the external interrupt, and redirects
 *          PA0 triggers to the Button module.
 * 
 * @param GPIO_Pin The pin that triggered the EXTI interrupt.
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_0)
    {
        Button_EXTI_Callback();
    }
}

