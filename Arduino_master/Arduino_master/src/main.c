/**
 * @file main.c
 * @brief Main execution entry point for the Arduino/AVR I2C Master controller.
 * @details This file implements the main firmware execution flow of the Master device.
 *          It initializes UART, TWI, and tick systems, and then runs a periodic
 *          polling loop (every 1000ms) to read sensor data from the STM32 I2C Slave
 *          and toggle the Slave's onboard LED.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include "tick.h"
#include "twi.h"
#include "uart.h"
#include "sensor_regmap.h"

/**
 * @brief Polling frequency: time interval in milliseconds between I2C read/write cycles.
 */
#define POLL_INTERVAL_MS 1000U

/**
 * @brief Main function.
 * @details Initializes hardware modules, enables global interrupts, and enters the
 *          super-loop (infinite polling cycle) to interact with the I2C Slave.
 * @return 0 (never returns under normal execution).
 */
int main(void)
{
    /* Initialize all modules */
    Tick_Init();        /* Initialize Timer0 millisecond tick generator */
    UART_Init(9600UL);  /* Initialize debug USART at 9600 bps */
    TWI_Init();         /* Initialize TWI (I2C) master hardware at 100 kHz */
    
    /* 
     * Enable global interrupts by setting the 'I' bit in the Status Register (SREG).
     * This is required for the Timer0 CTC interrupt (system tick) to fire.
     */
    sei(); 

    UART_Print("\r\n--- Smart Sensor Master v1.0 ---\r\n");

    uint32_t last_poll = 0U; /* Tracks the timestamp of the last executed poll */
    uint8_t led_state = 0U;   /* Tracks the current target state of the Slave's LED */

    while (1)
    {
        uint32_t now = Tick_GetMs();
        
        /* 
         * Non-blocking delay check: checks if 1000ms have elapsed since the last poll.
         * Using subtraction handles tick counter overflow safely.
         */
        if ((now - last_poll) >= POLL_INTERVAL_MS)
        {
            last_poll = now;
            TWI_Status_t st;
            uint8_t buf[3]; /* Temporary buffer for burst read data */

            UART_Print("Polling Sensor...\r\n");

            /*
             * 1. Read Device Identification info.
             * Perform a burst read of 2 bytes starting from REGMAP_ADDR_DEV_ID (0x00).
             * Reads: 0x00 (DEV_ID) and 0x01 (FW_VERSION).
             */
            st = TWI_ReadRegister(SENSOR_I2C_ADDR, REGMAP_ADDR_DEV_ID, buf, 2U);
            if (st == TWI_OK)
            {
                if (buf[0] != REGMAP_DEV_ID_VALUE)
                {
                    UART_Print("  [ERROR] Invalid DevID! Expected 0x");
                    UART_PrintHex(REGMAP_DEV_ID_VALUE);
                    UART_Print(" but got 0x");
                    UART_PrintHex(buf[0]);
                    UART_Print("\r\n");
                    continue; /* Skip the rest of the polling cycle to prevent erratic behavior */
                }

                UART_Print("  DevID: 0x");
                UART_PrintHex(buf[0]); /* Expected: 0xA5 */
                UART_Print("  FW_Ver: 0x");
                UART_PrintHex(buf[1]); /* Expected: 0x01 */
                UART_Print("\r\n");
            }
            else
            {
                UART_Print("  Read DevID failed. Error: ");
                UART_PrintDec(st);
                UART_Print("\r\n");
            }

            /*
             * 2. Read Temperature sensor registers.
             * Perform a burst read of 3 bytes starting from REGMAP_ADDR_TEMP_RAW_H (0x02).
             * Reads: 0x02 (Raw High Byte), 0x03 (Raw Low Byte), and 0x04 (Calculated °C).
             */
            st = TWI_ReadRegister(SENSOR_I2C_ADDR, REGMAP_ADDR_TEMP_RAW_H, buf, 3U);
            if (st == TWI_OK)
            {
                /* Reconstruct the 16-bit raw ADC value from two 8-bit registers (Big-Endian format) */
                uint16_t raw_temp = (buf[0] << 8) | buf[1];
                
                /* Cast register data to signed 8-bit to display positive/negative temperatures */
                int8_t deg_c = (int8_t)buf[2];
                
                UART_Print("  Temp Raw: ");
                UART_PrintDec(raw_temp);
                UART_Print("  Temp: ");
                UART_PrintDec(deg_c);
                UART_Print(" C\r\n");
            }
            else
            {
                UART_Print("  Read Temp failed.\r\n");
            }

            /*
             * 3. Read Button Counter and System Status.
             * Perform a burst read of 3 bytes starting from REGMAP_ADDR_BTN_COUNT (0x05).
             * Reads: 0x05 (Button count), 0x06 (LED control readback), and 0x07 (Status byte).
             */
            st = TWI_ReadRegister(SENSOR_I2C_ADDR, REGMAP_ADDR_BTN_COUNT, buf, 3U);
            if (st == TWI_OK)
            {
                UART_Print("  Btn Count: ");
                UART_PrintDec(buf[0]);
                UART_Print("  Status: 0x");
                UART_PrintHex(buf[2]);
                UART_Print("\r\n");
            }

            /*
             * 4. Toggle the LED on the Slave.
             * We flip the bits in `led_state` using the ON mask (0xFF).
             * This efficiently toggles it between 0x00 (OFF) and 0xFF (ON).
             */
            led_state ^= REGMAP_LED_CTRL_ON_MASK;
            
            st = TWI_WriteRegister(SENSOR_I2C_ADDR, REGMAP_ADDR_LED_CTRL, led_state);
            if (st == TWI_OK)
            {
                UART_Print("  LED Toggled\r\n");
            }
            else
            {
                UART_Print("  Write LED failed.\r\n");
            }

            UART_Print("--------------------------------\r\n");
        }
    }

    return 0; /* Never reached */
}

