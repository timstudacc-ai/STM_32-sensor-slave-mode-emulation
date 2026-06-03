/**
 * @file temp_sensor.c
 * @brief Internal Temperature Sensor driver implementation for STM32.
 * @details This file implements the ADC reading process for the STM32F411 internal
 *          temperature sensor. It schedules a reading every 500ms, calculates the
 *          equivalent Celsius temperature using optimized integer math, and updates the
 *          register file safely using critical sections.
 */

#include "temp_sensor.h"
#include "sensor_regmap.h"

/**
 * @brief Polling frequency for the sensor (every 500 milliseconds).
 */
#define TEMP_POLL_INTERVAL_MS 500U

/* 
 * STM32F411 Internal Temperature Sensor Constants (for Integer Math)
 * Derived from the STM32F411xC/E Datasheet:
 * - V25: Typical voltage at 25 °C is 0.76 V (760 mV).
 * - Average Slope: Typical slope is 2.5 mV/°C.
 * - Reference Voltage: VREF+ connected to VDD (3.3V / 3300 mV).
 * - ADC Resolution: 12-bit (max value = 2^12 - 1 = 4095).
 */
#define V25_MV 760       /* Typical sensor output voltage at 25 °C in mV */
#define VREF_MV 3300     /* Microcontroller supply/reference voltage in mV */
#define ADC_MAX_VAL 4095 /* Max value of a 12-bit ADC conversion */

/** @brief Static pointer to the ADC peripheral handle. */
static ADC_HandleTypeDef *g_hadc = NULL;

/** @brief Stores the system tick timestamp of the last temperature reading. */
static uint32_t g_last_temp_time = 0U;

HAL_StatusTypeDef TempSensor_Init(ADC_HandleTypeDef *hadc)
{
    if (hadc == NULL)
    {
        return HAL_ERROR;
    }
    g_hadc = hadc;
    g_last_temp_time = HAL_GetTick(); /* Record current uptime tick */
    return HAL_OK;
}

HAL_StatusTypeDef TempSensor_Process(volatile uint8_t *reg_file)
{
    if (g_hadc == NULL || reg_file == NULL)
    {
        return HAL_ERROR;
    }

    uint32_t current_time = HAL_GetTick();
    
    /* Non-blocking check: poll the ADC every 500ms */
    if ((current_time - g_last_temp_time) >= TEMP_POLL_INTERVAL_MS)
    {
        g_last_temp_time = current_time;

        /* Start the ADC conversion (software trigger mode) */
        HAL_StatusTypeDef status = HAL_ADC_Start(g_hadc);
        if (status == HAL_OK)
        {
            /* 
             * Wait for the ADC conversion to complete.
             * Since this is a single channel reading, it finishes in a few microseconds.
             * A timeout of 10ms is extremely safe and prevents deadlocking.
             */
            status = HAL_ADC_PollForConversion(g_hadc, 10);
            if (status == HAL_OK)
            {
                /* Read the raw 12-bit conversion value from the ADC data register */
                uint32_t raw_val = HAL_ADC_GetValue(g_hadc);

                /* 
                 * --- INTEGER MATHEMATICS CONVERSION ---
                 * 
                 * Step 1: Convert raw 12-bit ADC value to physical Millivolts (mV).
                 * Formula: Vsense (V) = (RawValue * Vref) / 4095
                 * Since Vref is 3300mV, we calculate vsense in mV:
                 *   vsense_mv = (raw_val * 3300) / 4095
                 */
                int32_t vsense_mv = (raw_val * VREF_MV) / ADC_MAX_VAL;

                /* 
                 * Step 2: Calculate temperature in degrees Celsius (°C).
                 * Formula from datasheet:
                 *   Temperature (°C) = ((Vsense_mv - V25_mv) / Avg_Slope) + 25
                 *   where Avg_Slope = 2.5 mV/°C.
                 *
                 * To avoid floating-point math (which increases code size and slows execution):
                 *   (Vsense_mv - V25_mv) / 2.5
                 *   = (Vsense_mv - V25_mv) / (25 / 10)
                 *   = ((Vsense_mv - V25_mv) * 10) / 25
                 * 
                 * Substituting:
                 *   temp_c = (((vsense_mv - 760) * 10) / 25) + 25
                 */
                int32_t temp_c_int32 = ((vsense_mv - V25_MV) * 10) / 25 + 25;
                int8_t temp_c_int = (int8_t)temp_c_int32;

                /* 
                 * --- CRITICAL SECTION ENTRY ---
                 * We must update the shared register map atomically.
                 * The I2C peripheral operates in an interrupt context (high priority).
                 * If an I2C read request arrives while we are halfway through writing the
                 * 2-byte raw value (e.g. writing TEMP_RAW_H but not TEMP_RAW_L), the Master
                 * would read corrupted/mismatched data.
                 * We temporarily disable interrupts globally to protect this memory write.
                 */
                __disable_irq();
                
                /* Store raw 12-bit ADC value in big-endian format (High Byte then Low Byte) */
                reg_file[REGMAP_ADDR_TEMP_RAW_H] = (uint8_t)((raw_val >> 8) & 0xFF);
                reg_file[REGMAP_ADDR_TEMP_RAW_L] = (uint8_t)(raw_val & 0xFF);
                
                /* Store calculated temperature in Celsius */
                reg_file[REGMAP_ADDR_TEMP_DEG_C] = (uint8_t)temp_c_int;

                /* Clear ADC error flag in status register if it was previously set */
                reg_file[REGMAP_ADDR_STATUS] &= ~REGMAP_STATUS_ADC_RDY_MASK;
                
                __enable_irq(); /* --- CRITICAL SECTION EXIT (Re-enable interrupts) --- */
            }
            else
            {
                /* ADC polling failed. Enter critical section to set error bit in status register */
                __disable_irq();
                reg_file[REGMAP_ADDR_STATUS] |= REGMAP_STATUS_ADC_RDY_MASK;
                __enable_irq();
            }

            /* Stop the ADC peripheral to conserve power */
            HAL_ADC_Stop(g_hadc);
        }
    }

    return HAL_OK;
}

