#include "smbus_i2c.h"
#include "stm32.h"
#include "debug.h"

#define I2C_SLAVE_ADDR 0x69

typedef enum {
    STATE_IDLE = 0,
    STATE_WAIT_COMMAND,
    STATE_COMMAND_RECEIVED
} SMBusState;

static I2C_HandleTypeDef hi2c2;
static SMBusState currentState = STATE_IDLE;
static uint8_t commandByte = 0;
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

    if(HAL_I2C_Init(&hi2c2) != HAL_OK)
        while(1);

    HAL_NVIC_SetPriority(I2C2_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(I2C2_IRQn);

    // Start listening for master
    HAL_I2C_EnableListen_IT(&hi2c2);

    currentState = STATE_IDLE;
}

// -------------------- Address Match Callback --------------------
void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection, uint16_t AddrMatchCode)
{
    if(hi2c->Instance != I2C2) return;

    if(TransferDirection == I2C_DIRECTION_TRANSMIT)
    {
        // Master writes command byte
        currentState = STATE_WAIT_COMMAND;
        HAL_I2C_Slave_Seq_Receive_IT(hi2c, &commandByte, 1, I2C_FIRST_AND_LAST_FRAME);
    }
    else // Master reads
    {
        // Only transmit if command was received
        if(currentState == STATE_COMMAND_RECEIVED)
        {
            HAL_I2C_Slave_Seq_Transmit_IT(hi2c, &responseByte, 1, I2C_LAST_FRAME);
        }
        else
        {
            __HAL_I2C_GENERATE_NACK(hi2c); // NACK if no command
        }
    }
}

// -------------------- Receive Complete Callback --------------------
void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if(hi2c->Instance != I2C2) return;

    // Prepare response immediately for repeated START
    switch(commandByte)
    {
        case 0x00: responseByte = 0x42; break;
        case 0x23: responseByte = 0x55; break;
        default:   responseByte = 0xFF; break;
    }

    currentState = STATE_COMMAND_RECEIVED;

    // Arm transmit for next repeated START (ready immediately)
    HAL_I2C_Slave_Seq_Transmit_IT(hi2c, &responseByte, 1, I2C_LAST_FRAME);
}

// -------------------- Transmit Complete Callback --------------------
void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if(hi2c->Instance != I2C2) return;
    currentState = STATE_IDLE;
}

// -------------------- Listen Complete Callback --------------------
void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if(hi2c->Instance != I2C2) return;
    currentState = STATE_IDLE;
    HAL_I2C_EnableListen_IT(hi2c);
}

// -------------------- Error Callback --------------------
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    if(hi2c->Instance != I2C2) return;

    uint32_t err = hi2c->ErrorCode;

    // Recover from errors
    hi2c->ErrorCode = HAL_I2C_ERROR_NONE;
    hi2c->State = HAL_I2C_STATE_READY;
    __HAL_UNLOCK(hi2c);
    currentState = STATE_IDLE;

    HAL_I2C_EnableListen_IT(hi2c);
}

// -------------------- IRQ Handler --------------------
void I2C2_IRQHandler(void)
{
    HAL_I2C_EV_IRQHandler(&hi2c2);
    HAL_I2C_ER_IRQHandler(&hi2c2);
}
