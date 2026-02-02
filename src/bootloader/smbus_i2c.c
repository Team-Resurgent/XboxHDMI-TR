#include "smbus_i2c.h"
#include "stm32.h"
#include "../shared/debug.h"
#include "../shared/defines.h"
#include "../shared/flash.h"
#include <string.h>

volatile I2C_HandleTypeDef hi2c2;
static uint32_t state = SMBUS_SMS_READY;

static uint16_t ram_buffer_bank = 0;
static uint16_t ram_buffer_index = 0;
static uint8_t ram_buffer[RAM_BUFFER_SIZE];
static uint32_t ram_buffer_crc = 0;

static int currentCommand = -1;  // -1 means no command, otherwise stores command byte
static uint8_t commandByte = 0;
static uint8_t dataByte = 0;
static uint8_t responseByte = 0x42;  // default response

// -------------------- Initialization --------------------
void smbus_i2c_init(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_I2C2_CLK_ENABLE();

    // I2C2 pins: PB10=SCL, PB11=SDA
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_10 | GPIO_PIN_11;
    gpio.Mode = GPIO_MODE_AF_OD;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio.Alternate = GPIO_AF1_I2C2;
    HAL_GPIO_Init(GPIOB, &gpio);

    hi2c2.Instance = I2C2;
    hi2c2.Init.Timing = 0x00303D5B;
    hi2c2.Init.OwnAddress1 = (I2C_SLAVE_ADDR << 1);
    hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c2.Init.OwnAddress2 = 0;
    hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE; // allow clock stretching

    if(HAL_I2C_Init((I2C_HandleTypeDef*)&hi2c2) != HAL_OK)
    {
        debug_log("SMBUS I2C init failed\n");
        while(1);
    }

    if (HAL_I2CEx_ConfigAnalogFilter((I2C_HandleTypeDef*)&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
    {
        debug_log("SMBUS I2C config analog filter failed\n");
        while(1);
    }

    if (HAL_I2CEx_ConfigDigitalFilter((I2C_HandleTypeDef*)&hi2c2, 0) != HAL_OK)
    {
        debug_log("SMBUS I2C config digital filter failed\n");
        while(1);
    }

    HAL_NVIC_SetPriority(I2C2_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(I2C2_IRQn);

    // Start listening for master
    HAL_I2C_EnableListen_IT((I2C_HandleTypeDef*)&hi2c2);

    state = SMBUS_SMS_READY;
    currentCommand = -1;
    debug_log("SMBus: I2C Slave 0x%02X ready\r\n", I2C_SLAVE_ADDR);
}

// -------------------- Address Match Callback --------------------
void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection, uint16_t AddrMatchCode)
{
    if(hi2c->Instance != I2C2) return;

    if(TransferDirection == I2C_DIRECTION_RECEIVE)
    {
        // Master reads (slave transmits)
        if ((state & SMBUS_SMS_IGNORED) == SMBUS_SMS_IGNORED)
        {
            // Best way not to block the line
            LL_I2C_TransmitData8(hi2c->Instance, 0xFFU);
            if (LL_I2C_IsActiveFlag_TCR(hi2c->Instance))
            {
                LL_I2C_SetTransferSize(hi2c->Instance, 1);
            }
            __HAL_I2C_GENERATE_NACK(hi2c);
            LL_I2C_ClearFlag_ADDR(hi2c->Instance);
            state &= ~SMBUS_SMS_IGNORED;
            HAL_I2C_EnableListen_IT(hi2c);
        }
        else
        {
            // If we don't have a response yet, try to get one
            if (!(state & (SMBUS_SMS_READY | SMBUS_SMS_RESPONSE_READY)) && currentCommand != -1)
            {
                // Prepare response if we have a command
                if (currentCommand != -1)
                {
                    // Response already prepared in SlaveRxCpltCallback
                    state |= SMBUS_SMS_RESPONSE_READY;
                }
                else
                {
                    state |= SMBUS_SMS_IGNORED;
                    __HAL_I2C_GENERATE_NACK(hi2c);
                    LL_I2C_ClearFlag_ADDR(hi2c->Instance);
                    HAL_I2C_EnableListen_IT(hi2c);
                }
            }

            if (state & SMBUS_SMS_RESPONSE_READY)
            {
                state |= SMBUS_SMS_TRANSMIT;
                // Disable SBC, cannot NACK on TX
                LL_I2C_DisableSlaveByteControl(hi2c->Instance);
                //debug_ring_log("SMBus: AddrR cmd=0x%02X resp=0x%02X\r\n", currentCommand, responseByte);
                HAL_I2C_Slave_Seq_Transmit_IT(hi2c, &responseByte, 1, I2C_LAST_FRAME);
            }
            else
            {
                //debug_ring_log("SMBus: AddrR NO_CMD\r\n");
                __HAL_I2C_GENERATE_NACK(hi2c); // NACK if no command
                LL_I2C_ClearFlag_ADDR(hi2c->Instance);
                HAL_I2C_EnableListen_IT(hi2c);
            }
        }
    }
    else
    {
        // Master writes (slave receives) - new command
        //debug_ring_log("SMBus: AddrW\r\n");
        state &= ~SMBUS_SMS_IGNORED;

        if (state & SMBUS_SMS_READY)
        {
            state &= ~SMBUS_SMS_READY;
            state |= SMBUS_SMS_RECEIVE;
            currentCommand = -1;
            // Enable SBC so we can NACK
            LL_I2C_EnableSlaveByteControl(hi2c->Instance);
            // Use FIRST_FRAME to allow chaining for write commands
            HAL_I2C_Slave_Seq_Receive_IT(hi2c, &commandByte, 1, I2C_NEXT_FRAME);
        }
    }
}

// -------------------- Receive Complete Callback --------------------
void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if(hi2c->Instance != I2C2) return;

    if (state & SMBUS_SMS_IGNORED)
    {
        __HAL_I2C_GENERATE_NACK(hi2c);
        if (LL_I2C_IsActiveFlag_TCR(hi2c->Instance))
        {
            LL_I2C_SetTransferSize(hi2c->Instance, 1);
        }
    }
    else
    {
        if (currentCommand == -1)
        {
            state &= ~SMBUS_SMS_RECEIVE;

            // Command byte received
            currentCommand = commandByte;
            responseByte = 0xff;

            // Read command - prepare response
            switch(commandByte)
            {
                case I2C_HDMI_COMMAND_READ_RAM:
                {
                    uint16_t ram_buffer_offset = (ram_buffer_bank << 8) | ram_buffer_index;
                    if (ram_buffer_offset >= RAM_BUFFER_SIZE)
                    {
                        break;
                    }
                    responseByte = ram_buffer[ram_buffer_offset];
                    ram_buffer_index++;
                    if (ram_buffer_index > 0xff)
                    {
                        ram_buffer_index = 0;
                        ram_buffer_bank++;
                    }
                    break;
                }
                case I2C_HDMI_COMMAND_READ_RAM_PAGE_CRC1:
                {
                    responseByte = (uint8_t)((ram_buffer_crc >> 24) & 0xff);
                    break;
                }
                case I2C_HDMI_COMMAND_READ_RAM_PAGE_CRC2:
                {
                    responseByte = (uint8_t)((ram_buffer_crc >> 16) & 0xff);
                    break;
                }
                case I2C_HDMI_COMMAND_READ_RAM_PAGE_CRC3:
                {
                    responseByte = (uint8_t)((ram_buffer_crc >> 8) & 0xff);
                    break;
                }
                case I2C_HDMI_COMMAND_READ_RAM_PAGE_CRC4:
                {
                    responseByte = (uint8_t)(ram_buffer_crc & 0xff);
                    break;
                }
                default:
                {
                    responseByte = 0xFF;
                    break;
                }
            }


            if ((currentCommand & I2C_WRITE_BIT) == I2C_WRITE_BIT)
            {
                // Write command - receive data byte
                state |= SMBUS_SMS_RECEIVE;
                HAL_I2C_Slave_Seq_Receive_IT(hi2c, &dataByte, 1, I2C_LAST_FRAME);
            }
            else
            {
                // Release the SCL stretch
                LL_I2C_SetTransferSize(hi2c->Instance, 1);
            }

        }
        else
        {
            // Block Write, get the size
            if (state & SMBUS_SMS_PROCESSING)
            {
                state &= ~(SMBUS_SMS_PROCESSING | SMBUS_SMS_RECEIVE);
                state |= SMBUS_SMS_RECEIVE;
                HAL_I2C_Slave_Seq_Receive_IT(hi2c, &dataByte, 1, I2C_LAST_FRAME);
            }
            else
            {
                state &= ~SMBUS_SMS_RECEIVE;
            }
        }
    }
}

// -------------------- Transmit Complete Callback --------------------
void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if(hi2c->Instance != I2C2) return;
    state &= ~SMBUS_SMS_TRANSMIT;
}

// -------------------- Listen Complete Callback --------------------
void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if(hi2c->Instance != I2C2) return;

    // Process the write if we have a write command and all data was received
    if(currentCommand != -1 && !(state & SMBUS_SMS_RESPONSE_READY))
    {
        // Size of bytes received must match expected size
        // For single-byte writes, XferCount should be 0 (all bytes received)
        if(hi2c->XferCount == 0)
        {
            // Process the write command
            switch(currentCommand)
            {
                case I2C_HDMI_COMMAND_WRITE_SET_MODE:
                {
                    if (dataByte == I2C_HDMI_MODE_BOOTLOADER)
                    {
                        *BOOTLOADER_FLAG_ADDRESS = BOOTLOADER_MAGIC_VALUE;
                    }
                    if (dataByte == I2C_HDMI_MODE_APPLICATION)
                    {
                        *BOOTLOADER_FLAG_ADDRESS = 0;
                    }
                    HAL_Delay(10);
                    NVIC_SystemReset();
                    break;
                }
                case I2C_HDMI_COMMAND_WRITE_READ_PAGE:
                {
                    ram_buffer_crc = flash_copy_page(dataByte, ram_buffer, RAM_BUFFER_SIZE);
                    break;
                }
                case I2C_HDMI_COMMAND_WRITE_RAM:
                {
                    uint16_t ram_offset = (ram_buffer_bank << 8) | ram_buffer_index;
                    if (ram_offset >= RAM_BUFFER_SIZE)
                    {
                        break;
                    }
                    ram_buffer[ram_offset] = dataByte;
                    ram_buffer_index++;
                    if (ram_buffer_index > 0xff)
                    {
                        ram_buffer_index = 0;
                        ram_buffer_bank++;
                    }
                    break;
                }
                case I2C_HDMI_COMMAND_WRITE_RAM_BANK:
                {
                    ram_buffer_bank = dataByte;
                    ram_buffer_index = 0;
                    break;
                }
                case I2C_HDMI_COMMAND_WRITE_RAM_INDEX:
                {
                    ram_buffer_index = dataByte;
                    break;
                }
                case I2C_HDMI_COMMAND_WRITE_RAM_APPLY:
                {
                    if (dataByte < (BOOTLOADER_SIZE >> FLASH_PAGE_SHIFT))
                    {
                        break;
                    }
                    flash_erase_page(dataByte);
                    flash_write_page(dataByte, ram_buffer, RAM_BUFFER_SIZE);
                    break;
                }
            }
        }
    }

    // Reset state
    state = SMBUS_SMS_READY;
    currentCommand = -1;

    // Do it all again
    HAL_I2C_EnableListen_IT(hi2c);
}

// -------------------- Error Callback --------------------
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    if(hi2c->Instance != I2C2) return;

    uint32_t err = hi2c->ErrorCode;

    if (err & (HAL_I2C_ERROR_BERR | HAL_I2C_ERROR_TIMEOUT))
    {
        // Critical error - reset the stack
        if(state & (SMBUS_SMS_TRANSMIT | SMBUS_SMS_RECEIVE | SMBUS_SMS_PROCESSING))
        {
            __HAL_I2C_DISABLE(hi2c);
            while (LL_I2C_IsEnabled(hi2c->Instance)) {}
            __HAL_I2C_ENABLE(hi2c);
            if(hi2c->State != HAL_I2C_STATE_READY)
            {
                HAL_I2C_DeInit(hi2c);
                HAL_I2C_Init(hi2c);
                HAL_I2C_EnableListen_IT(hi2c);
            }
        }
        state = SMBUS_SMS_READY;
        currentCommand = -1;
    }
    else if (err & HAL_I2C_ERROR_ARLO)
    {
        // Arbitration lost - signal error
        state = SMBUS_SMS_READY;
        currentCommand = -1;
        HAL_I2C_EnableListen_IT(hi2c);
    }
    else if (err & HAL_I2C_ERROR_AF)
    {
        // NACK - expected at end of read, handle gracefully
        hi2c->PreviousState = hi2c->State;
        hi2c->State = HAL_I2C_STATE_READY;
        __HAL_UNLOCK(hi2c);
        state = SMBUS_SMS_READY;
        currentCommand = -1;
        HAL_I2C_EnableListen_IT(hi2c);
    }
    else if(err != HAL_I2C_ERROR_NONE)
    {
        state = SMBUS_SMS_READY;
        currentCommand = -1;
        HAL_I2C_EnableListen_IT(hi2c);
    }
}
