#include "smbus_i2c.h"
#include "stm32.h"
#include "debug.h"

#define I2C_SLAVE_ADDR 0x69

#define SMBUS_SMS_NONE           ((uint32_t)0x00000000)  /*!< Uninitialized stack                                     */
#define SMBUS_SMS_READY          ((uint32_t)0x00000001)  /*!< No operation ongoing                                    */
#define SMBUS_SMS_TRANSMIT       ((uint32_t)0x00000002)  /*!< State of writing data to the bus                        */
#define SMBUS_SMS_RECEIVE        ((uint32_t)0x00000004)  /*!< State of receiving data on the bus                      */
#define SMBUS_SMS_PROCESSING     ((uint32_t)0x00000008)  /*!< Processing block (variable length transmissions)        */
#define SMBUS_SMS_RESPONSE_READY ((uint32_t)0x00000010)  /*!< Slave has reply ready for transmission (or command processing finished)        */
#define SMBUS_SMS_IGNORED        ((uint32_t)0x00000020)  /*!< The current command is not intended for this slave, ignore it (for ARP mainly) */

static SMBUS_HandleTypeDef hi2c2;
static uint32_t state;
static uint8_t buffer[42];

void smbus_i2c_init()
{
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_I2C2_CLK_ENABLE();

    // Configure GPIO pins for I2C2: PB10=SCL, PB11=SDA
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_10 | GPIO_PIN_11;
    gpio.Mode = GPIO_MODE_AF_OD;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio.Alternate = GPIO_AF1_I2C2;
    HAL_GPIO_Init(GPIOB, &gpio);

    hi2c2.Instance = I2C2;
    hi2c2.Init.Timing = 0x00303D5B;
    hi2c2.Init.AnalogFilter = SMBUS_ANALOGFILTER_ENABLE;
    hi2c2.Init.OwnAddress1 = (I2C_SLAVE_ADDR << 1);;
    hi2c2.Init.AddressingMode = SMBUS_ADDRESSINGMODE_7BIT;
    hi2c2.Init.DualAddressMode = SMBUS_DUALADDRESS_DISABLE;
    hi2c2.Init.OwnAddress2 = 0;
    hi2c2.Init.OwnAddress2Masks = SMBUS_OA2_NOMASK;
    hi2c2.Init.GeneralCallMode = SMBUS_GENERALCALL_DISABLE;
    hi2c2.Init.PacketErrorCheckMode = SMBUS_PEC_DISABLE;
    hi2c2.Init.PeripheralMode = SMBUS_PERIPHERAL_MODE_SMBUS_SLAVE;
    hi2c2.Init.SMBusTimeout = 0x00008249;
    if (HAL_SMBUS_Init(&hi2c2) != HAL_OK)
    {
        debug_log("SMBUS I2C init failed\n");
        while(1);
    }

    if (HAL_SMBUS_ConfigDigitalFilter(&hi2c2, 0) != HAL_OK)
    {
        debug_log("SMBUS I2C Config digital filter failed\\n");
        while(1);
    }

    // // Enable I2C2 interrupt in NVIC
    HAL_NVIC_SetPriority(I2C2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(I2C2_IRQn);
    
    // Start listening for address match
    HAL_SMBUS_EnableListen_IT(&hi2c2);
    
    debug_log("I2C Slave 0x%02X ready\r\n", I2C_SLAVE_ADDR);
}


void HAL_SMBUS_AddrCallback(SMBUS_HandleTypeDef *psmbus, uint8_t TransferDirection, uint16_t AddrMatchCode)
{
    if (psmbus->Instance != I2C2)
    {
        return;
    }

    debug_log("HAL_SMBUS_AddrCallback\r\n");

    if (TransferDirection == I2C_DIRECTION_RECEIVE)
    {
        // Cancel the preceding write
        psmbus->State &= ~HAL_SMBUS_STATE_SLAVE_BUSY_RX;

        if ((state & SMBUS_SMS_IGNORED) == SMBUS_SMS_IGNORED)
        {
            // best way not to block the line
            LL_I2C_TransmitData8(psmbus->Instance, 0xFFU);

            if (LL_I2C_IsActiveFlag_TCR(psmbus->Instance))
            {
                LL_I2C_SetTransferSize(psmbus->Instance, 1);
            }

            __HAL_SMBUS_GENERATE_NACK(psmbus);
            LL_I2C_ClearFlag_ADDR(psmbus->Instance);
            state &= ~SMBUS_SMS_IGNORED;
            HAL_SMBUS_EnableListen_IT(psmbus);
        }
        else
        {
            // If we don't have a response yet, try to get one
            if (!(state & (SMBUS_SMS_READY | SMBUS_SMS_RESPONSE_READY)))
            {
                // if (currentCommand)
                // {
                    //currentCommand->read(buffer, std::size(buffer));
                    state |= SMBUS_SMS_RESPONSE_READY;
                // }
                // else
                // {
                //     state |= SMBUS_SMS_IGNORED;
                //     __HAL_SMBUS_GENERATE_NACK(psmbus);
                //     LL_I2C_ClearFlag_ADDR(psmbus->Instance);
                //     HAL_SMBUS_EnableListen_IT(psmbus);
                // }
            }

            if (state & SMBUS_SMS_RESPONSE_READY)
            {
                state |= SMBUS_SMS_TRANSMIT;

                // if (currentCommand->getSize() > sizeof(uint16_t))
                // {
                //     // Block read, need to prepend size to the read
                //     std::shift_right(std::begin(buffer), std::end(buffer), 1);
                //     buffer[0] = currentCommand->getSize();
                // }

                //HAL_SMBUS_Slave_Transmit_IT(psmbus, buffer, currentCommand->getSize(), SMBUS_LAST_FRAME_NO_PEC);
            }
        }
    }
    else
    {
        // New command, get the command byte
        state &= ~SMBUS_SMS_IGNORED;

        if (state & SMBUS_SMS_READY)
        {
            state &= ~SMBUS_SMS_READY;
            state |= SMBUS_SMS_RECEIVE;
            //currentCommand = nullptr;
            //HAL_SMBUS_Slave_Receive_IT(psmbus, buffer, 1U, SMBUS_NEXT_FRAME);
        }
    }
}


void HAL_SMBUS_SlaveTxCpltCallback(SMBUS_HandleTypeDef *psmbus)
{
    if (psmbus->Instance != I2C2)
    {
        return;
    }

    debug_log("HAL_SMBUS_SlaveTxCpltCallback\r\n");

    state &= ~SMBUS_SMS_TRANSMIT;
}

void HAL_SMBUS_SlaveRxCpltCallback(SMBUS_HandleTypeDef *psmbus)
{
    if (psmbus->Instance != I2C2)
    {
        return;
    }

    debug_log("HAL_SMBUS_SlaveRxCpltCallback\r\n");

    if (state & SMBUS_SMS_IGNORED)
    {
        __HAL_SMBUS_GENERATE_NACK(psmbus);
        if (LL_I2C_IsActiveFlag_TCR(psmbus->Instance))
        {
            LL_I2C_SetTransferSize(psmbus->Instance, 1);
        }
    }
    else
    {
        // if (!currentCommand)
        // {
        //     state &= ~SMBUS_SMS_RECEIVE;

        //     currentCommand = findCommand(buffer[0]);

        //     if (!currentCommand)
        //     {
        //         state |= SMBUS_SMS_IGNORED;
        //         __HAL_SMBUS_GENERATE_NACK(psmbus);
        //         if (LL_I2C_IsActiveFlag_TCR(psmbus->Instance))
        //         {
        //             LL_I2C_SetTransferSize(psmbus->Instance, 1);
        //         }
        //     }
        //     else
        //     {
        //         if (currentCommand->canWrite())
        //         {
        //             // We don't know if this is a read or a write, assume write,
        //             // and if we're wrong we'll get another START event and end
        //             // up back in AddrCallback
        //             if (currentCommand->getSize() > sizeof(uint16_t))
        //             {
        //                 // Block Write, get size
        //                 state |= SMBUS_SMS_RECEIVE | SMBUS_SMS_PROCESSING;
        //                 HAL_SMBUS_Slave_Receive_IT(psmbus, buffer, 1U, SMBUS_NEXT_FRAME);
        //             }
        //             else
        //             {
        //                 // Fixed size, get remaining bytes
        //                 state |= SMBUS_SMS_RECEIVE;
        //                 HAL_SMBUS_Slave_Receive_IT(psmbus, buffer, currentCommand->getSize(), SMBUS_LAST_FRAME_NO_PEC);
        //             }
        //         }
        //         else
        //         {
        //             // Release the SCL stretch
        //             LL_I2C_SetTransferSize(psmbus->Instance, 1);
        //         }
        //     }
        // }
        // else
        {
            // Block Write, get the size
            if (state & SMBUS_SMS_PROCESSING)
            {
                state &= ~(SMBUS_SMS_PROCESSING | SMBUS_SMS_RECEIVE);

                size_t size = buffer[1];
                // if (size != currentCommand->getSize())
                // {
                //     // Incorrect command size, NACK
                //     state |= SMBUS_SMS_IGNORED;
                //     __HAL_SMBUS_GENERATE_NACK(psmbus);
                //     if (LL_I2C_IsActiveFlag_TCR(psmbus->Instance))
                //     {
                //         LL_I2C_SetTransferSize(psmbus->Instance, 1);
                //     }
                // }

                state |= SMBUS_SMS_RECEIVE;
                HAL_SMBUS_Slave_Receive_IT(psmbus, buffer, size, SMBUS_LAST_FRAME_NO_PEC);
            }
            else
            {
                state &= ~SMBUS_SMS_RECEIVE;
            }
        }
    }
}

void HAL_SMBUS_ListenCpltCallback(SMBUS_HandleTypeDef *psmbus)
{
    if (psmbus->Instance != I2C2)
    {
        return;
    }

    debug_log("HAL_SMBUS_ListenCpltCallback\r\n");

    // if (currentCommand && !(state & SMBUS_SMS_RESPONSE_READY))
    // {
    //     // Process the write
    //     // Size of bytes received must match command size
    //     if (psmbus->XferCount == 0)
    //     {
    //         // Size was good, got all bytes
    //         currentCommand->write(buffer);
    //     }
    // }

    // We don't support simple commands or quick commands, so a STOP event will
    // just reset everything.
    state = SMBUS_SMS_READY;
    //currentCommand = nullptr;

    // Do it all again
    HAL_SMBUS_EnableListen_IT(psmbus);
}

void HAL_SMBUS_ErrorCallback(SMBUS_HandleTypeDef *psmbus)
{
    if (psmbus->Instance != I2C2)
    {
        return;
    }

    debug_log("HAL_SMBUS_ErrorCallback\r\n");

    uint32_t error = psmbus->ErrorCode;

    if (error & (HAL_SMBUS_ERROR_BERR | HAL_SMBUS_ERROR_BUSTIMEOUT | HAL_SMBUS_ERROR_HALTIMEOUT))
    {
        // Critical error - reset the stack
        if (state & (SMBUS_SMS_TRANSMIT | SMBUS_SMS_RECEIVE | SMBUS_SMS_PROCESSING))
        {
            __HAL_SMBUS_DISABLE(psmbus);
            while (LL_I2C_IsEnabled(psmbus->Instance)) {}
            __HAL_SMBUS_ENABLE(psmbus);
            if (psmbus->State != HAL_SMBUS_STATE_READY)
            {
                HAL_SMBUS_DeInit(psmbus);
                HAL_SMBUS_Init(psmbus);
                HAL_SMBUS_EnableListen_IT(psmbus);
            }
        }
        state = SMBUS_SMS_READY;
        //currentCommand = nullptr;
    }
    else if (error & HAL_SMBUS_ERROR_ARLO)
    {
        // Signal error to main thread
        //errorFlag = true;
    }
    else if (error & HAL_SMBUS_ERROR_ACKF)
    {
        psmbus->PreviousState = psmbus->State;
        psmbus->State = HAL_SMBUS_STATE_READY;

        __HAL_UNLOCK(psmbus);

        state = SMBUS_SMS_READY;
        //currentCommand = nullptr;

        HAL_SMBUS_EnableListen_IT(psmbus);
    }
}

void I2C2_IRQHandler(void)
{
    if (hi2c2.Instance->ISR & (SMBUS_FLAG_BERR | SMBUS_FLAG_ARLO | SMBUS_FLAG_OVR | SMBUS_FLAG_TIMEOUT | SMBUS_FLAG_ALERT | SMBUS_FLAG_PECERR))
    {
        HAL_SMBUS_ER_IRQHandler(&hi2c2);
    }
    else
    {
        HAL_SMBUS_EV_IRQHandler(&hi2c2);
    }
}



// bool smbus_read_byte(
//     uint8_t address,
//     uint8_t reg,
//     uint8_t* result)
// {
//     if (!result)
//     {
//         return false;
//     }

//     errorFlag = false;
//     if (HAL_SMBUS_Master_Transmit_IT(&hsmbus, address, &reg, 1, SMBUS_FIRST_AND_LAST_FRAME_NO_PEC) != HAL_OK)
//     {
//         return false;
//     }
//     // TODO: Should probably add software timeout to ensure no infinite loop
//     while(HAL_SMBUS_GetState(&hsmbus) != HAL_SMBUS_STATE_READY);

//     if (errorFlag)
//     {
//         // Got error, probably lost arbitration
//         return false;
//     }

//     if (HAL_SMBUS_Master_Receive_IT(&hsmbus, address, result, 1, SMBUS_FIRST_AND_LAST_FRAME_NO_PEC) != HAL_OK)
//     {
//         return false;
//     }
//     // TODO: Should probably add software timeout to ensure no infinite loop
//     while(HAL_SMBUS_GetState(&hsmbus) != HAL_SMBUS_STATE_READY);

//     return !errorFlag;
// }

// extern "C" void HI2C_XBOX_SMBUS_IRQ_HANDLER(void)
// {
//     if (hsmbus.Instance->ISR & (SMBUS_FLAG_BERR | SMBUS_FLAG_ARLO | SMBUS_FLAG_OVR | SMBUS_FLAG_TIMEOUT | SMBUS_FLAG_ALERT | SMBUS_FLAG_PECERR))
//     {
//         HAL_SMBUS_ER_IRQHandler(&hsmbus);
//     }
//     else
//     {
//         HAL_SMBUS_EV_IRQHandler(&hsmbus);
//     }

// }





















// // Called when we finish receiving data
// void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c)
// {
//     debug_log("I2C RX: Byte[%d]=0x%02X\r\n", rx_count, rx_data[rx_count]);
    
//     rx_count++;
    
//     // If we have room for more bytes, prepare to receive next byte
//     if (rx_count < 2)
//     {
//         HAL_I2C_Slave_Seq_Receive_IT(hi2c, &rx_data[rx_count], 1, I2C_NEXT_FRAME);
//     }
// }

// // Called when master sends STOP or repeated START
// void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c)
// {
//     debug_log("I2C Listen Complete, rx_count=%d\r\n", rx_count);
    
//     if (rx_count == 1)
//     {
//         // Received 1 byte (command only) - Xbox read command
//         last_command = rx_data[0];
//         debug_log("I2C: Read command for register 0x%02X\r\n", last_command);
//     }
//     else if (rx_count == 2)
//     {
//         // Received 2 bytes (command + data) - Write command
//         uint8_t command = rx_data[0];
//         uint8_t data = rx_data[1];
//         tx_data[command] = data;
//         last_command = command;
//         debug_log("I2C: Write 0x%02X to register 0x%02X\r\n", data, command);
//     }
    
//     rx_count = 0;
//     HAL_I2C_EnableListen_IT(hi2c);
// }

// // Called when we finish transmitting data
// void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c)
// {
//     debug_log("I2C TX: CMD=0x%02X DATA=0x%02X sent\r\n", last_command, tx_data[last_command]);
//     HAL_I2C_EnableListen_IT(hi2c);
// }

// // Called on I2C error
// void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
// {
//     debug_log("I2C Error: 0x%08X\r\n", hi2c->ErrorCode);
//     HAL_I2C_EnableListen_IT(hi2c);
// }

// void I2C2_IRQHandler(void)
// {
//     HAL_I2C_EV_IRQHandler(&hi2c2);
//     HAL_I2C_ER_IRQHandler(&hi2c2);
// }