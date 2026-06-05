/**
 * @file tick.h
 * @brief System tick timer interface for time tracking and non-blocking delays.
 * @details This driver configures an 8-bit timer on the AVR microcontroller to generate
 *          an interrupt every millisecond, providing a reference clock for task polling.
 */

#ifndef TICK_H
#define TICK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief Initialize Timer0 to generate a 1ms system tick interrupt.
 * @details Configures Timer0 in CTC (Clear Timer on Compare Match) mode, sets the prescaler
 *          to 64 and compare limit to 249 (at 16MHz clock) to trigger every 1ms, and enables
 *          the compare match A interrupt.
 */
void Tick_Init(void);

/**
 * @brief Retrieve the current uptime of the system in milliseconds.
 * @details Performs an atomic read of the volatile tick counter to prevent corruption from
 *          interrupt context switching during the multi-byte read.
 * @return The number of milliseconds elapsed since the microcontroller booted.
 */
uint32_t Tick_GetMs(void);

#ifdef __cplusplus
}
#endif

#endif /* TICK_H */

