#include "smbus_i2c.h"
#include "stm32.h"
#include "debug.h"

#define I2C_SLAVE_ADDR 0x69

// Simple state machine
typedef enum {
    STATE_IDLE = 0,
    STATE_WAIT_COMMAND,
    STATE_COMMAND_RECEIVED,
    STATE_WAIT_REPEATED_START
} SMBusState;

static I2C_HandleTypeDef hi2c2;
static SMBusState currentState = STATE_IDLE;
static uint8_t commandByte = 0;
static uint8_t responseByte = 0x42;  // Hardcoded response for now

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
    hi2c2.Init.OwnAddress1 = (I2C_SLAVE_ADDR << 1);
    hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c2.Init.OwnAddress2 = 0;
    hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_ENABLE;  // DISABLE clock stretching!

    if (HAL_I2C_Init(&hi2c2) != HAL_OK)
    {
        debug_log("I2C init failed\n");
        while(1);
    }

    HAL_NVIC_SetPriority(I2C2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(I2C2_IRQn);

    // Start listening for address match
    HAL_I2C_EnableListen_IT(&hi2c2);

    // PRE-LOAD TXDR with default response so it's ready immediately
    hi2c2.Instance->TXDR = 0x42;

    currentState = STATE_IDLE;
    debug_log("I2C Slave 0x%02X ready (NoStretch, TXDR pre-loaded)\r\n", I2C_SLAVE_ADDR);
}

// This gets called when our address is matched
void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection, uint16_t AddrMatchCode)
{
    if (hi2c->Instance != I2C2) return;

    if (TransferDirection == I2C_DIRECTION_TRANSMIT)  // Master reads from us
    {
        debug_log("AddrMatch READ (state=%d)\r\n", currentState);
        
        // Always send response, regardless of state
        // If no command was received, still send 0x42 for now
        debug_log("Sending response: 0x%02X\r\n", responseByte);
        HAL_I2C_Slave_Seq_Transmit_IT(hi2c, &responseByte, 1, I2C_LAST_FRAME);
    }
    else  // Master writes to us
    {
        debug_log("AddrMatch WRITE\r\n");
        currentState = STATE_WAIT_COMMAND;
        HAL_I2C_Slave_Seq_Receive_IT(hi2c, &commandByte, 1, I2C_FIRST_FRAME);
    }
}

// This gets called when we finish receiving
void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance != I2C2) return;

    debug_log("RxCplt: cmd=0x%02X\r\n", commandByte);
    currentState = STATE_COMMAND_RECEIVED;
    responseByte = 0x42;

    debug_log("Response ready\r\n");
}

// This gets called when we finish transmitting
void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance != I2C2) return;
    debug_log("TxCplt\r\n");
    currentState = STATE_IDLE;
    
    // Reload TXDR for next read
    hi2c->Instance->TXDR = responseByte;
}

// This gets called when transaction completes (STOP condition)
void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance != I2C2) return;

    debug_log("ListenCplt - STOP received\r\n");
    currentState = STATE_IDLE;
    HAL_I2C_EnableListen_IT(hi2c);
}

// This gets called on errors
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance != I2C2) return;

    uint32_t error = hi2c->ErrorCode;
    debug_log("I2C ERROR: 0x%08X\r\n", error);

    if (error & HAL_I2C_ERROR_AF)
    {
        debug_log("NACK (expected for Read Byte)\r\n");
    }

    currentState = STATE_IDLE;
    hi2c->State = HAL_I2C_STATE_READY;
    __HAL_UNLOCK(hi2c);
    HAL_I2C_EnableListen_IT(hi2c);
}

// IRQ Handler
void I2C2_IRQHandler(void)
{
    HAL_I2C_EV_IRQHandler(&hi2c2);
    HAL_I2C_ER_IRQHandler(&hi2c2);
}
