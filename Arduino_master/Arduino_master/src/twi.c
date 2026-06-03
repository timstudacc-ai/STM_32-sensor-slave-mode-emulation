/**
 * @file twi.c
 * @brief Two-Wire Interface (TWI/I2C) Master driver implementation for AVR.
 * @details This file implements low-level register control for AVR TWI hardware.
 *          It covers initialization, byte-level transmissions, status verification,
 *          and software-based bus recovery when lines are pulled low by error states.
 * 
 * Hardware registers used:
 * - TWCR: Control register (clears interrupt flag, configures start, stop, ack, enable)
 * - TWSR: Status register (provides hardware state codes, configures prescaler)
 * - TWDR: Data register (contains byte to transmit/receive)
 * - TWBR: Bit rate register (defines bus clock frequency)
 */

#include "twi.h"
#include "tick.h"
#include <avr/io.h>
#include <util/delay.h>

/**
 * @brief Maximum time to wait for a hardware operation before giving up.
 * @details Set to 100ms, which is extremely generous but prevents freezing if the bus dies.
 */
#define TWI_TIMEOUT_MS 100U

/* 
 * TWI Status Codes (defined in AVR datasheet)
 * These values represent the state of the TWI state machine located in TWSR[7:3].
 */
#define TWI_START         0x08  /**< START condition transmitted */
#define TWI_REP_START     0x10  /**< Repeated START condition transmitted */
#define TWI_MT_SLA_ACK    0x18  /**< SLA+W transmitted, ACK received (Master Transmitter) */
#define TWI_MT_DATA_ACK   0x28  /**< Data byte transmitted, ACK received (Master Transmitter) */
#define TWI_MR_SLA_ACK    0x40  /**< SLA+R transmitted, ACK received (Master Receiver) */
#define TWI_MR_DATA_ACK   0x50  /**< Data byte received, ACK returned (Master Receiver) */
#define TWI_MR_DATA_NACK  0x58  /**< Data byte received, NACK returned (Master Receiver, signals end of transfer) */

/**
 * @brief Helper function to poll the TWI Interrupt (TWINT) flag.
 * @details TWINT is set by hardware when a TWI operation finishes.
 *          We poll this flag with a timeout using the system millisecond tick.
 * @return TWI_OK if the operation completed, TWI_ERROR_TIMEOUT otherwise.
 */
static TWI_Status_t TWI_WaitForComplete(void)
{
    uint32_t start_time = Tick_GetMs();
    
    /* 
     * TWINT is set by hardware when it has completed its current task.
     * Note: TWINT must be cleared by writing a logical '1' to it in code
     * to begin the next operation (which we do in other functions).
     */
    while (!(TWCR & (1 << TWINT)))
    {
        if ((Tick_GetMs() - start_time) > TWI_TIMEOUT_MS)
        {
            return TWI_ERROR_TIMEOUT;
        }
    }
    return TWI_OK;
}

/**
 * @brief Send a START condition onto the I2C bus.
 * @details Sets the TWSTA bit to request a START condition.
 * @return TWI_OK if START was sent and acknowledged, or error code.
 */
static TWI_Status_t TWI_Start(void)
{
    /*
     * Clear TWINT flag (by writing 1 to it) to start transmission.
     * Enable TWI (TWEN).
     * Request START condition (TWSTA).
     */
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    
    /* Wait for hardware to finish sending START */
    if (TWI_WaitForComplete() != TWI_OK) return TWI_ERROR_TIMEOUT;
    
    /* 
     * Read status register TWSR.
     * Mask off the prescaler bits (lower 3 bits, though only 2 are prescaler, bit 2 is reserved/0)
     * using 0xF8 to get the 5-bit status code.
     */
    uint8_t status = TWSR & 0xF8;
    if ((status != TWI_START) && (status != TWI_REP_START))
    {
        return TWI_ERROR_START;
    }
    return TWI_OK;
}

/**
 * @brief Send a STOP condition onto the I2C bus.
 * @details Sets the TWSTO bit. This is a non-blocking request, but we wait for it
 *          to clear (which signals the STOP has been executed on the bus).
 */
static void TWI_Stop(void)
{
    /*
     * Clear TWINT to trigger operation.
     * Request STOP condition (TWSTO).
     * Keep TWI enabled (TWEN).
     */
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
    
    /* Wait for the hardware to automatically clear the TWSTO bit (signals STOP complete) */
    uint32_t start_time = Tick_GetMs();
    while (TWCR & (1 << TWSTO))
    {
        if ((Tick_GetMs() - start_time) > TWI_TIMEOUT_MS)
        {
            break;
        }
    }
}

/**
 * @brief Write a single byte to the TWI bus and verify the status code.
 * @param data Byte to write.
 * @param expect_status The expected hardware status code (e.g. TWI_MT_SLA_ACK).
 * @return TWI_OK on success, or an error code describing the failure.
 */
static TWI_Status_t TWI_WriteByte(uint8_t data, uint8_t expect_status)
{
    /* Load data byte into data shift register TWDR */
    TWDR = data;
    
    /* Clear TWINT to trigger the shift out of data, keep TWI enabled */
    TWCR = (1 << TWINT) | (1 << TWEN);
    
    /* Wait for byte to finish transmitting */
    if (TWI_WaitForComplete() != TWI_OK) return TWI_ERROR_TIMEOUT;
    
    /* Verify if the slave returned the correct response (ACK or NACK) */
    if ((TWSR & 0xF8) != expect_status)
    {
        if (expect_status == TWI_MT_SLA_ACK) return TWI_ERROR_SLA_W_NACK;
        if (expect_status == TWI_MR_SLA_ACK) return TWI_ERROR_SLA_R_NACK;
        return TWI_ERROR_DATA_NACK;
    }
    return TWI_OK;
}

/**
 * @brief Read a single byte from the TWI bus.
 * @param data Pointer to store the received byte.
 * @param ack Set to 1 to send an ACK (request more data), or 0 to send a NACK (last byte).
 * @return TWI_OK on success, or timeout error.
 */
static TWI_Status_t TWI_ReadByte(uint8_t *data, uint8_t ack)
{
    if (ack)
    {
        /* Clear TWINT, keep enabled, and set Enable Acknowledge (TWEA) to return ACK to slave */
        TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);
    }
    else
    {
        /* Clear TWINT, keep enabled, but do NOT set TWEA. This returns a NACK to the slave */
        TWCR = (1 << TWINT) | (1 << TWEN);
    }
    
    /* Wait for byte to finish shifting in */
    if (TWI_WaitForComplete() != TWI_OK) return TWI_ERROR_TIMEOUT;
    
    /* Copy received data out of TWDR */
    *data = TWDR;
    return TWI_OK;
}

/**
 * @brief Recover a stuck I2C bus.
 * @details Sometimes, if a master is reset mid-read, a slave may be holding the SDA line low,
 *          waiting for the master to send more clock pulses. Since SDA is low, the master
 *          cannot generate a START condition (bus error).
 *          This recovery algorithm:
 *          1. Disables the TWI peripheral.
 *          2. Configures SCL and SDA as GPIOs.
 *          3. Toggles SCL 9 times (clock pulses) to convince the slave to release SDA.
 *          4. Manually bit-bangs a STOP sequence to clean up the bus state.
 *          5. Re-initializes TWI.
 */
static void TWI_RecoverBus(void)
{
    /* Disable TWI hardware module, releasing pins to GPIO control */
    TWCR = 0;
    
    /* Configure SCL (PC5) and SDA (PC4) as outputs on Port C */
    DDRC |= (1 << PC5) | (1 << PC4);
    PORTC |= (1 << PC4); /* Set SDA High */
    
    /* Toggle SCL 9 times (creates up to 9 clock pulses for the slave) */
    for (uint8_t i = 0; i < 9; i++)
    {
        PORTC |= (1 << PC5);    /* SCL High */
        _delay_us(5);
        PORTC &= ~(1 << PC5);   /* SCL Low */
        _delay_us(5);
    }
    
    /* Bit-bang a manual STOP condition: SCL goes high while SDA transitions from low to high */
    PORTC &= ~(1 << PC4); /* SDA Low */
    _delay_us(5);
    PORTC |= (1 << PC5);  /* SCL High */
    _delay_us(5);
    PORTC |= (1 << PC4);  /* SDA High (transition low-to-high while SCL is high = STOP) */
    _delay_us(5);
    
    /* Re-initialize TWI module to restore hardware control */
    TWI_Init();
}

void TWI_Init(void)
{
    /* 
     * Configure TWI Bit Rate Generator.
     * SCL Clock Frequency is calculated as:
     *   F_SCL = F_CPU / (16 + 2 * TWBR * PrescalerValue)
     * For 100 kHz (Standard Mode) at F_CPU = 16 MHz:
     *   100,000 = 16,000,000 / (16 + 2 * TWBR * 1)
     *   16 + 2 * TWBR = 160
     *   2 * TWBR = 144
     *   TWBR = 72
     */
    TWSR = 0x00; /* Set prescaler to 1 (TWPS1 = 0, TWPS0 = 0) */
    TWBR = 72;   /* Set bit rate division factor to 72 */
    TWCR = (1 << TWEN); /* Enable the TWI peripheral */
}

TWI_Status_t TWI_ReadRegister(uint8_t dev_addr, uint8_t reg_addr, uint8_t *buf, uint8_t len)
{
    TWI_Status_t st;
    
    /* 1. Generate START condition */
    st = TWI_Start();
    if (st != TWI_OK) goto error;
    
    /* 2. Write Slave Address + Write Bit (0). Address is shifted left, bit 0 is cleared. */
    st = TWI_WriteByte((dev_addr << 1) | 0, TWI_MT_SLA_ACK);
    if (st != TWI_OK) goto error;
    
    /* 3. Send target register pointer address */
    st = TWI_WriteByte(reg_addr, TWI_MT_DATA_ACK);
    if (st != TWI_OK) goto error;
    
    /* 4. Generate Repeated START to switch bus direction to read */
    st = TWI_Start();
    if (st != TWI_OK) goto error;
    
    /* 5. Write Slave Address + Read Bit (1). Address is shifted left, bit 0 is set. */
    st = TWI_WriteByte((dev_addr << 1) | 1, TWI_MR_SLA_ACK);
    if (st != TWI_OK) goto error;
    
    /* 6. Read bytes sequentially (burst read) */
    for (uint8_t i = 0; i < len; i++)
    {
        /* 
         * Return ACK (1) for all bytes except the last one.
         * The last byte must be followed by a NACK (0) to tell the slave to stop transmitting
         * and release control of the SDA line.
         */
        uint8_t ack = (i < (len - 1)) ? 1 : 0;
        st = TWI_ReadByte(&buf[i], ack);
        if (st != TWI_OK) goto error;
    }
    
    /* 7. Generate STOP condition to release the bus */
    TWI_Stop();
    return TWI_OK;

error:
    /* If a timeout or start failure occurs, recover the bus to prevent deadlock */
    TWI_Stop();
    if (st == TWI_ERROR_TIMEOUT || st == TWI_ERROR_START)
    {
        TWI_RecoverBus();
    }
    return st;
}

TWI_Status_t TWI_WriteRegister(uint8_t dev_addr, uint8_t reg_addr, uint8_t data)
{
    TWI_Status_t st;
    
    /* 1. Generate START condition */
    st = TWI_Start();
    if (st != TWI_OK) goto error;
    
    /* 2. Write Slave Address + Write Bit (0) */
    st = TWI_WriteByte((dev_addr << 1) | 0, TWI_MT_SLA_ACK);
    if (st != TWI_OK) goto error;
    
    /* 3. Send target register pointer address */
    st = TWI_WriteByte(reg_addr, TWI_MT_DATA_ACK);
    if (st != TWI_OK) goto error;
    
    /* 4. Send the data byte to write into the register */
    st = TWI_WriteByte(data, TWI_MT_DATA_ACK);
    if (st != TWI_OK) goto error;
    
    /* 5. Generate STOP condition */
    TWI_Stop();
    return TWI_OK;

error:
    /* If a timeout or start failure occurs, recover the bus to prevent deadlock */
    TWI_Stop();
    if (st == TWI_ERROR_TIMEOUT || st == TWI_ERROR_START)
    {
        TWI_RecoverBus();
    }
    return st;
}

