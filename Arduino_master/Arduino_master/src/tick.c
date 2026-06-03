/**
 * @file tick.c
 * @brief System tick timer implementation using AVR Timer0.
 * @details This implementation configures Timer0 to count up to a predetermined value
 *          and trigger an interrupt every millisecond. The interrupt increments a global
 *          uptime counter, allowing the rest of the application to implement non-blocking
 *          delays and periodic tasks.
 */

#include "tick.h"
#include <avr/io.h>
#include <avr/interrupt.h>

/**
 * @brief Volatile global counter tracking elapsed milliseconds since boot.
 * @note Marked 'volatile' because it is modified inside an Interrupt Service Routine (ISR).
 *       This prevents the compiler from optimizing reads by caching its value in a CPU register.
 */
static volatile uint32_t g_tick_ms = 0U;

void Tick_Init(void)
{
    /* 
     * Configure Timer0 for CTC (Clear Timer on Compare Match) mode.
     * In CTC mode, the timer counts from 0 up to OCR0A, then resets back to 0
     * and triggers an interrupt, rather than counting all the way to 255.
     * WGM01 = 1, WGM00 = 0, WGM02 = 0 set CTC mode.
     */
    TCCR0A = (1 << WGM01);

    /*
     * Calculate the compare limit (OCR0A) for a 1 millisecond interval.
     * Microcontroller clock speed (F_CPU) = 16 MHz (16,000,000 Hz).
     * With a prescaler of 64, the timer ticks at:
     *   16,000,000 Hz / 64 = 250,000 Hz (ticks per second)
     * For a 1ms period (1/1000th of a second), we need:
     *   250,000 ticks/sec / 1000 = 250 ticks.
     * Since the timer starts counting at 0, we count from 0 to 249 (total 250 ticks).
     */
    OCR0A = 249;

    /*
     * Enable Timer0 Output Compare Match A Interrupt.
     * This will cause the hardware to call ISR(TIMER0_COMPA_vect) every time
     * the timer counter (TCNT0) matches the value in OCR0A (249).
     */
    TIMSK0 |= (1 << OCIE0A);

    /*
     * Start the timer by setting the Clock Select bits in Control Register B.
     * CS01 = 1, CS00 = 1 configures the clock source to be F_CPU / 64 (prescaler of 64).
     */
    TCCR0B = (1 << CS01) | (1 << CS00);
}

uint32_t Tick_GetMs(void)
{
    uint32_t ms;
    
    /* 
     * Read the 32-bit tick counter atomically.
     * Because an 8-bit AVR CPU takes multiple instructions (4 cycles) to read a 32-bit
     * variable, a timer interrupt could fire right in the middle of this read.
     * This would result in a corrupted value (e.g., reading a half-updated number).
     * To prevent this, we temporarily disable global interrupts, copy the value,
     * and then restore the interrupt state.
     */
    uint8_t old_sreg = SREG; /* Save current global interrupt flag state (SREG contains 'I' bit) */
    cli();                   /* Clear Interrupts (disable interrupts globally) */
    ms = g_tick_ms;          /* Copy the value safely */
    SREG = old_sreg;         /* Restore original interrupt state (re-enabling them if they were on) */
    
    return ms;
}

/**
 * @brief Interrupt Service Routine (ISR) triggered by Timer0 Compare Match A.
 * @details Executed once every millisecond. Increments the global uptime counter.
 */
ISR(TIMER0_COMPA_vect)
{
    g_tick_ms++;
}

