/**
 * @file i2c_slave.c
 * @brief I2C Slave driver implementation for STM32 using HAL interrupts.
 * @details This file implements the state machine required to emulate an I2C register
 *          map. It handles master read/write requests, auto-increments the register
 *          pointer for burst transfers, updates control registers, and manages status flags.
 * 
 * I2C Sequence callbacks used:
 * - HAL_I2C_AddrCallback: Triggered when address matches.
 * - HAL_I2C_SlaveRxCpltCallback: Triggered when a byte from the master is received.
 * - HAL_I2C_SlaveTxCpltCallback: Triggered when a byte is sent to the master.
 * - HAL_I2C_ListenCpltCallback: Triggered when the master releases the bus (STOP/Restart).
 * - HAL_I2C_ErrorCallback: Triggered on bus errors (like NACK or bus collision).
 */

#include "i2c_slave.h"
#include "sensor_regmap.h"

/* --- Global Driver State --- */

/** @brief Pointer to the HAL I2C hardware handle. */
static I2C_HandleTypeDef *g_hi2c = NULL;

/** @brief Pointer to the volatile register map buffer defined in main.c. */
static volatile uint8_t *g_reg_file = NULL;

/** @brief The total size of the register map buffer. */
static uint8_t g_reg_size = 0U;

/* --- Internal State Machine Variables --- */

/**
 * @brief Virtual Register Pointer.
 * @details Tracks which register address the Master is currently reading from or writing to.
 */
static volatile uint8_t g_reg_ptr = 0U;

/**
 * @brief Receive buffer for incoming I2C bytes.
 * @details Holds a single byte read from the I2C bus.
 */
static volatile uint8_t g_rx_buffer[1];

/**
 * @brief Write state tracking flag.
 * @details I2C writes always begin with a register address, followed by the data byte(s).
 *          - If 1, the next byte received will be saved as the virtual register pointer (g_reg_ptr).
 *          - If 0, the next byte received will be written into the register at g_reg_ptr.
 */
static volatile uint8_t g_is_first_byte = 1U;

HAL_StatusTypeDef I2C_Slave_Init(I2C_HandleTypeDef *hi2c, volatile uint8_t *reg_file, uint8_t reg_size)
{
    /* Safety check: ensure all pointer arguments are valid */
    if (hi2c == NULL || reg_file == NULL || reg_size == 0U)
    {
        return HAL_ERROR;
    }

    g_hi2c = hi2c;
    g_reg_file = reg_file;
    g_reg_size = reg_size;

    /* 
     * Enable Address Listen Mode in Interrupt (IT) mode.
     * This instructs the I2C hardware peripheral to watch the bus for its slave address.
     * When a match occurs, the hardware generates an interrupt, calling HAL_I2C_AddrCallback.
     */
    return HAL_I2C_EnableListen_IT(g_hi2c);
}

/**
 * @brief Callback triggered when the Slave's 7-bit address matches the address sent by the Master.
 * @details This function initializes the data transfer direction and sets up the initial receive or transmit call.
 * 
 * @param hi2c Pointer to the I2C handle.
 * @param TransferDirection Direction of transfer:
 *                          - I2C_DIRECTION_TRANSMIT (0): Master is writing to Slave.
 *                          - I2C_DIRECTION_RECEIVE (1): Master is reading from Slave.
 * @param AddrMatchCode Unused. The matched address value.
 */
void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection, uint16_t AddrMatchCode)
{
    (void)AddrMatchCode; /* Unused parameter */
    
    if (hi2c == g_hi2c)
    {
        /* --- Case A: MASTER WRITES TO SLAVE --- */
        if (TransferDirection == I2C_DIRECTION_TRANSMIT)
        {
            /* 
             * Since this is a new write transaction, the first byte must be treated
             * as the register address pointer.
             */
            g_is_first_byte = 1U;
            
            /* 
             * Request receipt of 1 byte using interrupts.
             * I2C_FIRST_FRAME indicates the start of a sequence-based transmission.
             */
            HAL_I2C_Slave_Seq_Receive_IT(hi2c, (uint8_t *)&g_rx_buffer[0], 1, I2C_FIRST_FRAME);
        }
        /* --- Case B: MASTER READS FROM SLAVE --- */
        else
        {
            /*
             * Transmit the byte located at the current register pointer.
             * If the pointer is valid, we send the value from the register file.
             */
            if (g_reg_ptr < g_reg_size)
            {
                /* 
                 * Request transmission of 1 byte using interrupts.
                 * I2C_NEXT_FRAME indicates this is part of an ongoing sequence.
                 */
                HAL_I2C_Slave_Seq_Transmit_IT(hi2c, (uint8_t *)&g_reg_file[g_reg_ptr], 1, I2C_NEXT_FRAME);
            }
            else
            {
                /* If register pointer is out of bounds, return dummy 0xFF value */
                uint8_t dummy = 0xFF;
                HAL_I2C_Slave_Seq_Transmit_IT(hi2c, &dummy, 1, I2C_NEXT_FRAME);
            }
        }
    }
}

/**
 * @brief Callback triggered when the Slave finishes receiving a byte from the Master.
 * @details This function processes the received byte (acting either as the register pointer or data to write).
 * 
 * @param hi2c Pointer to the I2C handle.
 */
void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c == g_hi2c)
    {
        /* 
         * Step 1: Handle the register pointer byte.
         * If this is the first byte of the write sequence, the master is configuring SCL/SDA
         * to point at a specific register address.
         */
        if (g_is_first_byte == 1U)
        {
            g_reg_ptr = g_rx_buffer[0]; /* Set register pointer */
            g_is_first_byte = 0U;       /* Next byte received will be data */

            /* Wait to receive the next byte (data to be written) */
            HAL_I2C_Slave_Seq_Receive_IT(hi2c, (uint8_t *)&g_rx_buffer[0], 1, I2C_NEXT_FRAME);
        }
        /* 
         * Step 2: Handle data bytes.
         * If it is not the first byte, the master is writing data into the register.
         */
        else
        {
            if (g_reg_ptr < g_reg_size)
            {
                /* 
                 * Secure Register Write Protection.
                 * Only the LED Control Register (0x06) is writeable in this system.
                 * Other registers (Device ID, Temp, Status, Button count) are Read-Only.
                 */
                if (g_reg_ptr == REGMAP_ADDR_LED_CTRL)
                {
                    g_reg_file[g_reg_ptr] = g_rx_buffer[0];
                }
                
                /* Auto-increment the register pointer to support burst writes */
                g_reg_ptr++;
            }
            
            /* Prepare to receive the next byte in case of a burst write */
            HAL_I2C_Slave_Seq_Receive_IT(hi2c, (uint8_t *)&g_rx_buffer[0], 1, I2C_NEXT_FRAME);
        }
    }
}

/**
 * @brief Callback triggered when the Slave finishes transmitting a byte to the Master.
 * @details Increments the register pointer to support burst reads, then prepares the next byte.
 * 
 * @param hi2c Pointer to the I2C handle.
 */
void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c == g_hi2c)
    {
        /* 
         * Auto-increment the register pointer after a byte is successfully transmitted.
         * This allows the Master to execute a single burst read to grab consecutive registers
         * (e.g., Raw Temp High, Low, and calculated °C).
         */
        if (g_reg_ptr < g_reg_size)
        {
            g_reg_ptr++;
        }

        /* 
         * Prepare the next byte for transmission.
         * If the updated pointer is within bounds, send the corresponding register value.
         * Otherwise, send dummy 0xFF.
         */
        if (g_reg_ptr < g_reg_size)
        {
            HAL_I2C_Slave_Seq_Transmit_IT(hi2c, (uint8_t *)&g_reg_file[g_reg_ptr], 1, I2C_NEXT_FRAME);
        }
        else
        {
            uint8_t dummy = 0xFF;
            HAL_I2C_Slave_Seq_Transmit_IT(hi2c, &dummy, 1, I2C_NEXT_FRAME);
        }
    }
}

/**
 * @brief Callback triggered when the I2C transaction finishes.
 * @details Re-enables listen mode so the slave can receive the next transaction.
 *          This is called when SCL/SDA detect a STOP condition or repeated START.
 * 
 * @param hi2c Pointer to the I2C handle.
 */
void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c == g_hi2c)
    {
        /* 
         * Re-enable address listening.
         * This resets the hardware interface back to monitoring the bus.
         */
        HAL_I2C_EnableListen_IT(hi2c);
    }
}

/**
 * @brief Callback triggered when a bus error or protocol violation occurs.
 * @details Inspects the error source. If it is a normal end-of-read NACK, clears the flag.
 *          Otherwise, sets the error bit in the Status Register.
 * 
 * @param hi2c Pointer to the I2C handle.
 */
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c == g_hi2c)
    {
        /* Default: Assume a real bus error and set the status bit */
        if (g_reg_file != NULL)
        {
            g_reg_file[REGMAP_ADDR_STATUS] |= REGMAP_STATUS_I2C_ERR_MASK;
        }

        /* Fetch the exact error bitmask from the HAL handle */
        uint32_t error = HAL_I2C_GetError(hi2c);
        
        /* 
         * Check for Acknowledge Failure (AF).
         * In I2C protocol, when a Master reads a sequence of bytes, it terminates the read
         * by sending a NACK (No Acknowledge) on the final byte instead of an ACK.
         * This causes the Slave hardware to trigger an AF error interrupt.
         * Because this is normal protocol behavior, we clear the status register's error flag.
         */
        if (error == HAL_I2C_ERROR_AF)
        {
            if (g_reg_file != NULL)
            {
                g_reg_file[REGMAP_ADDR_STATUS] &= ~REGMAP_STATUS_I2C_ERR_MASK;
            }
        }
        
        /* Re-enable listening to recover from the error state and await new calls */
        HAL_I2C_EnableListen_IT(hi2c);
    }
}

