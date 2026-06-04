/**
 * @file smart_sensor_app.h
 * @brief Unified Smart Sensor application layer for STM32.
 * @details Encapsulates the ADC (Temperature), User Button (EXTI), and LED 
 *          handling into a single object-oriented structure.
 */

#ifndef SMART_SENSOR_APP_H
#define SMART_SENSOR_APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/**
 * @brief Smart Sensor State Structure.
 * @details Holds hardware handles, state variables, and a pointer to the shared I2C register map.
 */
typedef struct {
    ADC_HandleTypeDef *hadc;            /**< Hardware handle for ADC (Temp Sensor) */
    volatile uint8_t  *reg_file;        /**< Pointer to the shared I2C register map */
    
    uint32_t last_temp_read_time;       /**< State: Temp sensor polling timer */
    uint32_t last_btn_press_time;       /**< State: Button debounce timer */
    volatile uint8_t button_event_flag; /**< State: EXTI trigger flag */
} SmartSensor_HandleTypeDef;

/* API Prototypes */
HAL_StatusTypeDef SmartSensor_Init(SmartSensor_HandleTypeDef *hSensor, ADC_HandleTypeDef *hadc, volatile uint8_t *reg_file);
HAL_StatusTypeDef SmartSensor_Process(SmartSensor_HandleTypeDef *hSensor);
void SmartSensor_EXTI_Callback(void);

#ifdef __cplusplus
}
#endif

#endif /* SMART_SENSOR_APP_H */
