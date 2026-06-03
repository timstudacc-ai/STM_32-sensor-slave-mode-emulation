/**
 * @file mpu6050.c
 * @brief Basic driver implementation for the MPU6050 6-axis IMU (Accelerometer/Gyroscope).
 * @note This driver is currently unused in the main project but is preserved for future expansion.
 *       It uses blocking I2C memory access functions to initialize and read sensor registers.
 */

#include "mpu6050.h"

void MPU6050_Init(I2C_HandleTypeDef *hi2c)
{
    uint8_t check;
    uint8_t data;

    /* Read the WHO_AM_I register (address 0x75) to verify device presence */
    HAL_I2C_Mem_Read(hi2c, MPU6050_ADDR, MPU6050_REG_WHO_AM_I, 1, &check, 1, HAL_MAX_DELAY);

    /* 0x68 is the default WHO_AM_I value returned by a functional MPU6050 sensor */
    if (check == 0x68)
    {
        /* 
         * Wake up the sensor. By default, the MPU6050 boots up in sleep mode (low-power).
         * Writing 0 to PWR_MGMT_1 (Power Management 1 register, address 0x6B) wakes it up
         * and configures it to use the internal 8MHz oscillator as the clock source.
         */
        data = 0;
        HAL_I2C_Mem_Write(hi2c, MPU6050_ADDR, MPU6050_REG_PWR_MGMT_1, 1, &data, 1, HAL_MAX_DELAY);
    }
}

void MPU6050_Read_All(I2C_HandleTypeDef *hi2c, MPU6050_Data_t *data)
{
    uint8_t Rec_Data[14];

    /* 
     * Perform a burst read of 14 registers starting from ACCEL_XOUT_H (address 0x3B).
     * This reads accelerometer data, internal temperature, and gyroscope data in one go:
     * - ACCEL_XOUT (2 bytes)
     * - ACCEL_YOUT (2 bytes)
     * - ACCEL_ZOUT (2 bytes)
     * - TEMP_OUT (2 bytes)
     * - GYRO_XOUT (2 bytes)
     * - GYRO_YOUT (2 bytes)
     * - GYRO_ZOUT (2 bytes)
     */
    HAL_I2C_Mem_Read(hi2c, MPU6050_ADDR, MPU6050_REG_ACCEL_XOUT_H, 1, Rec_Data, 14, HAL_MAX_DELAY);

    /* Reconstruct 16-bit signed accelerometer values (Big-Endian format) */
    data->Accel_X = (int16_t)(Rec_Data[0] << 8 | Rec_Data[1]);
    data->Accel_Y = (int16_t)(Rec_Data[2] << 8 | Rec_Data[3]);
    data->Accel_Z = (int16_t)(Rec_Data[4] << 8 | Rec_Data[5]);

    /* Note: Temp sensor data (bytes 6 and 7: Rec_Data[6..7]) is ignored here */

    /* Reconstruct 16-bit signed gyroscope values (Big-Endian format) */
    data->Gyro_X = (int16_t)(Rec_Data[8] << 8 | Rec_Data[9]);
    data->Gyro_Y = (int16_t)(Rec_Data[10] << 8 | Rec_Data[11]);
    data->Gyro_Z = (int16_t)(Rec_Data[12] << 8 | Rec_Data[13]);
}

