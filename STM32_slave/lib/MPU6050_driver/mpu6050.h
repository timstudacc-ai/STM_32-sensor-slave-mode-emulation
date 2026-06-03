#ifndef INC_MPU6050_H_
#define INC_MPU6050_H_

#include "stm32f4xx_hal.h"  // або просто "main.h" якщо у вас там HAL
#include <string.h>
#include <stdio.h>

// Адреса MPU6050 (якщо пін AD0 = LOW)
#define MPU6050_ADDR  (0x68 << 1)

// Регістри MPU6050
#define MPU6050_REG_PWR_MGMT_1   0x6B
#define MPU6050_REG_ACCEL_XOUT_H 0x3B
#define MPU6050_REG_WHO_AM_I     0x75

// Структура для зберігання даних
typedef struct {
    int16_t Accel_X;
    int16_t Accel_Y;
    int16_t Accel_Z;
    int16_t Gyro_X;
    int16_t Gyro_Y;
    int16_t Gyro_Z;
} MPU6050_Data_t;

// Функції
void MPU6050_Init(I2C_HandleTypeDef *hi2c);
void MPU6050_Read_All(I2C_HandleTypeDef *hi2c, MPU6050_Data_t *data);

#endif /* INC_MPU6050_H_ */
