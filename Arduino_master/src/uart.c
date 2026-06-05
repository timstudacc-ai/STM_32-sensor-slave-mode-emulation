/**
 * @file uart.c
 * @brief USART serial debugging interface implementation for AVR.
 * @details This file implements low-level register configuration for the ATmega328P
 *          USART0 peripheral. It provides basic blocking transmission functions for
 *          debugging output via the USB-to-Serial converter.
 * 
 * Hardware registers used:
 * - UBRR0H/L: Baud rate divisor registers.
 * - UCSR0A: Status register A (monitors the UDRE0 data-register-empty flag).
 * - UCSR0B: Control register B (enables the transmitter logic).
 * - UCSR0C: Control register C (selects 8 data bits, no parity, 1 stop bit).
 * - UDR0: USART data buffer register (writing triggers transmit).
 */

#include "uart.h"
#include <avr/io.h>

void UART_Init(uint32_t baud)
{
    /*
     * Calculate the USART Baud Rate Register (UBRR) value.
     * Formula for Normal Asynchronous Mode (U2X0 = 0):
     *   UBRR = (F_CPU / (16 * baud)) - 1
     * We add (8 * baud) to the numerator to ensure proper integer rounding instead of truncation.
     */
    uint16_t ubrr = ((F_CPU + (8UL * baud)) / (16UL * baud)) - 1;
    
    /* Load the 12-bit baud rate value into the high and low UBRR registers */
    UBRR0H = (uint8_t)(ubrr >> 8);
    UBRR0L = (uint8_t)ubrr;
    
    /* 
     * Enable the transmitter (TXEN0 = 1).
     * Receiver is left disabled since this is a write-only debug channel.
     */
    UCSR0B = (1 << TXEN0);
    
    /* 
     * Set frame format: 8 data bits, 1 stop bit, no parity (8N1).
     * UCSZ01 = 1, UCSZ00 = 1 set character size to 8 bits.
     * USBS0 = 0 (default) sets stop bit to 1.
     * UPM01 = 0, UPM00 = 0 (default) disables parity.
     */
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void UART_PutChar(uint8_t c)
{
    /* 
     * Wait for the transmit buffer to become empty.
     * UDRE0 (USART Data Register Empty) in UCSR0A is set by hardware when UDR0 is empty
     * and ready to receive a new byte. We poll this bit until it becomes 1.
     */
    while (!(UCSR0A & (1 << UDRE0)));
    
    /* 
     * Write the character byte to UDR0.
     * This action clears the UDRE0 flag and starts the physical shift-out transmission.
     */
    UDR0 = c;
}

void UART_Print(const char *str)
{
    /* Loop through the string array until we hit the null terminator character '\0' */
    while (*str)
    {
        UART_PutChar(*str++);
    }
}

void UART_PrintHex(uint8_t val)
{
    const char hex_chars[] = "0123456789ABCDEF";
    
    /* Extract and print the upper nibble (4 bits) */
    UART_PutChar(hex_chars[val >> 4]);
    
    /* Extract and print the lower nibble (4 bits) */
    UART_PutChar(hex_chars[val & 0x0F]);
}

void UART_PrintDec(int16_t val)
{
    char buf[10];
    int8_t i = 0;
    uint8_t is_neg = 0;
    
    /* Handle negative integers */
    if (val < 0)
    {
        is_neg = 1;
        val = -val;
    }
    /* Special case for zero */
    else if (val == 0)
    {
        UART_PutChar('0');
        return;
    }
    
    /* Deconstruct integer into ASCII digit characters in reverse order (ones, tens, hundreds...) */
    while (val > 0)
    {
        buf[i++] = (val % 10) + '0';
        val /= 10;
    }
    
    /* Append negative sign indicator if necessary */
    if (is_neg)
    {
        buf[i++] = '-';
    }
    
    /* Print the characters in correct order (pop off the stack/buffer backwards) */
    while (i > 0)
    {
        UART_PutChar(buf[--i]);
    }
}

