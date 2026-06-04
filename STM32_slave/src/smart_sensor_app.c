/**
 * @file smart_sensor_app.c
 * @brief Unified Smart Sensor application layer implementation for STM32.
 */

#include "smart_sensor_app.h"
#include "sensor_regmap.h"

#define TEMP_POLL_INTERVAL_MS 500U
#define DEBOUNCE_DELAY_MS     50U
#define V25_MV                760       
#define VREF_MV               3300     
#define ADC_MAX_VAL           4095 

/** 
 * @brief Global pointer for the EXTI interrupt handler.
 * @details Because HAL_GPIO_EXTI_Callback does not accept a context pointer,
 *          we must use a global pointer to map the hardware interrupt to our object instance.
 */
static SmartSensor_HandleTypeDef *g_pSensorInstance = NULL;

HAL_StatusTypeDef SmartSensor_Init(SmartSensor_HandleTypeDef *hSensor, ADC_HandleTypeDef *hadc, volatile uint8_t *reg_file)
{
    if (hSensor == NULL || hadc == NULL || reg_file == NULL)
    {
        return HAL_ERROR;
    }

    hSensor->hadc = hadc;
    hSensor->reg_file = reg_file;
    hSensor->last_temp_read_time = HAL_GetTick();
    hSensor->last_btn_press_time = 0U;
    hSensor->button_event_flag = 0U;

    /* Map the global pointer for the EXTI handler */
    g_pSensorInstance = hSensor;

    return HAL_OK;
}

HAL_StatusTypeDef SmartSensor_Process(SmartSensor_HandleTypeDef *hSensor)
{
    if (hSensor == NULL)
    {
        return HAL_ERROR;
    }

    uint32_t current_time = HAL_GetTick();

    /* =========================================================================
     * 1. PROCESS LED (Output)
     * ========================================================================= */
    uint8_t led_ctrl = hSensor->reg_file[REGMAP_ADDR_LED_CTRL];
    if (led_ctrl == REGMAP_LED_CTRL_ON_VALUE)
    {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET); /* 0V: LED ON */
    }
    else if (led_ctrl == REGMAP_LED_CTRL_OFF_VALUE)
    {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);   /* 3.3V: LED OFF */
    }

    /* =========================================================================
     * 2. PROCESS BUTTON (Input / Debounce)
     * ========================================================================= */
    if (hSensor->button_event_flag == 1U)
    {
        if ((current_time - hSensor->last_btn_press_time) >= DEBOUNCE_DELAY_MS)
        {
            if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET)
            {
                __disable_irq();
                hSensor->reg_file[REGMAP_ADDR_BTN_COUNT]++;
                __enable_irq();
            }
            hSensor->last_btn_press_time = current_time;
        }
        hSensor->button_event_flag = 0U;
    }

    /* =========================================================================
     * 3. PROCESS TEMPERATURE SENSOR (ADC)
     * ========================================================================= */
    if ((current_time - hSensor->last_temp_read_time) >= TEMP_POLL_INTERVAL_MS)
    {
        hSensor->last_temp_read_time = current_time;

        if (HAL_ADC_Start(hSensor->hadc) == HAL_OK)
        {
            if (HAL_ADC_PollForConversion(hSensor->hadc, 10) == HAL_OK)
            {
                uint32_t raw_val = HAL_ADC_GetValue(hSensor->hadc);
                int32_t vsense_mv = (raw_val * VREF_MV) / ADC_MAX_VAL;
                int32_t temp_c_int32 = ((vsense_mv - V25_MV) * 10) / 25 + 25;
                int8_t temp_c_int = (int8_t)temp_c_int32;

                __disable_irq();
                hSensor->reg_file[REGMAP_ADDR_TEMP_RAW_H] = (uint8_t)((raw_val >> 8) & 0xFF);
                hSensor->reg_file[REGMAP_ADDR_TEMP_RAW_L] = (uint8_t)(raw_val & 0xFF);
                hSensor->reg_file[REGMAP_ADDR_TEMP_DEG_C] = (uint8_t)temp_c_int;
                hSensor->reg_file[REGMAP_ADDR_STATUS] &= ~REGMAP_STATUS_ADC_RDY_MASK;
                __enable_irq();
            }
            else
            {
                __disable_irq();
                hSensor->reg_file[REGMAP_ADDR_STATUS] |= REGMAP_STATUS_ADC_RDY_MASK;
                __enable_irq();
            }
            HAL_ADC_Stop(hSensor->hadc);
        }
    }

    return HAL_OK;
}

void SmartSensor_EXTI_Callback(void)
{
    if (g_pSensorInstance != NULL)
    {
        g_pSensorInstance->button_event_flag = 1U;
    }
}

/**
 * @brief Overridden HAL GPIO External Interrupt Callback.
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_0)
    {
        SmartSensor_EXTI_Callback();
    }
}
