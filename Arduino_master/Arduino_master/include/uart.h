/**
 * @file uart.h
 * @brief UART serial debugging interface for AVR.
 * @details This driver manages register settings for the AVR USART0 module, allowing the Master
 *          to transmit text, hex values, and decimal values over USB serial at 115200 baud.
 */

#ifndef UART_H
#define UART_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief Initialize USART0 for asynchronous serial transmission.
 * @details Configures the baud rate registers (UBRR0), enables the transmitter (TX),
 *          and configures 8N1 frame format (8 data bits, no parity, 1 stop bit).
 * @param baud Target baud rate (e.g., 115200).
 */
void UART_Init(uint32_t baud);

/**
 * @brief Transmit a single character over USART0.
 * @details Blocks until the transmit buffer register (UDR0) is empty and ready to accept data.
 * @param c Character byte to transmit.
 */
void UART_PutChar(uint8_t c);

/**
 * @brief Transmit a null-terminated string over USART0.
 * @details Iterates through the string character by character until the null terminator is reached.
 * @param str Pointer to the C-style string to transmit.
 */
void UART_Print(const char *str);

/**
 * @brief Print a byte value in raw hexadecimal format.
 * @details Converts a byte into two ASCII hex characters (e.g. 0xA5 becomes "A" and "5")
 *          and sends them sequentially.
 * @param val Byte value to print.
 */
void UART_PrintHex(uint8_t val);

/**
 * @brief Print a signed 16-bit integer in decimal format.
 * @details Decodes a signed 16-bit number into its decimal ASCII representations.
 *          Handles negative numbers (prints a '-') and zero properly.
 * @param val Signed integer to print.
 */
void UART_PrintDec(int16_t val);

#ifdef __cplusplus
}
#endif

#endif /* UART_H */

