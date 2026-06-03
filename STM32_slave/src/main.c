/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "i2c.h"
#include "iwdg.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "sensor_regmap.h"
#include "i2c_slave.h"
#include "button.h"
#include "temp_sensor.h"
#include "led_ctrl.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/**
 * @brief Volatile shared register file database.
 * @details This array holds the status and control registers for the Slave.
 *          It is declared 'volatile' because it is read and written both in the
 *          main loop (thread context) and the I2C interrupt callbacks (interrupt context).
 *          Initial values:
 *          - Register 0x00 (DEV_ID): 0xA5 (fixed device identifier)
 *          - Register 0x01 (FW_VERSION): 0x01 (fixed firmware version)
 *          - Registers 0x02-0x04 (TEMP_RAW_H, TEMP_RAW_L, TEMP_DEG_C): 0 (updated by ADC)
 *          - Register 0x05 (BTN_COUNT): 0 (updated by button EXTI)
 *          - Register 0x06 (LED_CTRL): 0 (updated by I2C master write)
 *          - Register 0x07 (STATUS): 0 (indicates system error flags)
 */
static volatile uint8_t g_reg_file[REGMAP_SIZE] = {
    REGMAP_DEV_ID_VALUE,     /* 0x00: DEV_ID */
    REGMAP_FW_VERSION_VALUE, /* 0x01: FW_VERSION */
    0, 0, 0,                 /* 0x02-0x04: TEMP */
    0,                       /* 0x05: BTN_COUNT */
    0,                       /* 0x06: LED_CTRL */
    0                        /* 0x07: STATUS */
};
extern I2C_HandleTypeDef hi2c1;
extern ADC_HandleTypeDef hadc1;
extern IWDG_HandleTypeDef hiwdg;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_IWDG_Init();
  /* USER CODE BEGIN 2 */
  /* Initialize custom hardware modules and state variables */
  Button_Init();                  /* Set up user button press flags and timers */
  LED_Init();                     /* Set up LED state control variables */
  TempSensor_Init(&hadc1);        /* Register ADC1 handle for internal temperature sensor */
  I2C_Slave_Init(&hi2c1, g_reg_file, REGMAP_SIZE); /* Start I2C slave listening on address 0x42 */
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /* 
     * Periodic/polling task scheduling block.
     * These process functions are called sequentially in the super-loop.
     * They use non-blocking internal clocks (using HAL_GetTick()) to execute
     * their work at different periodic intervals without stopping the loop.
     */
    Button_Process(g_reg_file);     /* Debounce PA0 button edge and update press counter */
    TempSensor_Process(g_reg_file); /* Read internal temperature channel every 500ms and save raw/Celsius values */
    LED_Process(g_reg_file);        /* Read LED_CTRL register and adjust GPIOC Pin 13 voltage accordingly */

    /* 
     * Kick the Independent Watchdog (IWDG) timer.
     * The watchdog monitors system execution. If the software freezes (e.g. gets stuck
     * in a loop or deadlocks in an interrupt), the watchdog will fail to get kicked.
     * Once its countdown expires, the hardware triggers a system reset to recover automatically.
     */
    HAL_IWDG_Refresh(&hiwdg);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); /* Turn OFF LED (Active low) */
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
