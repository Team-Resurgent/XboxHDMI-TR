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

void smbus_i2c_init()
{
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_I2C2_CLK_ENABLE();

    // Configure I2C2 pins: PB10=SCL, PB11=SDA
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
    hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE; // ENABLE clock stretching

    if (HAL_I2C_Init(&hi2c2) != HAL_OK)
    {
        debug_log("I2C init failed\n");
        while(1);
    }

    HAL_NVIC_SetPriority(I2C2_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(I2C2_IRQn);

    // Start listening for address match
    HAL_I2C_EnableListen_IT(&hi2c2);

    currentState = STATE_IDLE;
    debug_log("I2C Slave 0x%02X ready\r\n", I2C_SLAVE_ADDR);
}

// Address match callback
void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection, uint16_t AddrMatchCode)
{
    if (hi2c->Instance != I2C2) return;

    if (TransferDirection == I2C_DIRECTION_TRANSMIT) // Master writes
    {
        debug_log(">>> AddrMatch WRITE\r\n");
        currentState = STATE_WAIT_COMMAND;

        // Receive 1 byte (the command) - use FIRST_AND_LAST for single byte
        HAL_I2C_Slave_Seq_Receive_IT(hi2c, &commandByte, 1, I2C_FIRST_AND_LAST_FRAME_NO_PEC);
    }
    else // Master reads
    {
        debug_log(">>> AddrMatch READ (lastCmd=0x%02X, response=0x%02X)\r\n", commandByte, responseByte);

        // Now transmit the response (prepared in RxCpltCallback)
        // This is the correct place - master has sent repeated START + READ
        HAL_I2C_Slave_Seq_Transmit_IT(hi2c, &responseByte, 1, I2C_FIRST_AND_LAST_FRAME);
    }
}

// Receive complete
void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance != I2C2) return;

    debug_log(">>> RxCplt: cmd=0x%02X <<<\r\n", commandByte);

    // Process command and prepare response
    switch (commandByte)
    {
        case 0x00: responseByte = 0x42; break;
        case 0x23: responseByte = 0x55; break;
        default:   responseByte = 0xFF; break;
    }
    
    currentState = STATE_COMMAND_RECEIVED;
    debug_log(">>> Response 0x%02X ready, waiting for repeated START\r\n", responseByte);
    
    // ⚠️ DO NOT call HAL_I2C_Slave_Seq_Transmit_IT here!
    // The master is still in WRITE mode. Calling transmit now causes BERR.
    // Wait for the repeated START (which triggers AddrCallback with READ direction)
}

// Transmit complete
void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance != I2C2) return;

    debug_log(">>> TxCplt: response=0x%02X <<<\n", responseByte);
    currentState = STATE_IDLE;
}

// STOP condition / Listen complete
void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance != I2C2) return;

    debug_log("ListenCplt - STOP received\r\n");
    currentState = STATE_IDLE;
    HAL_I2C_EnableListen_IT(hi2c);
}

// Error callback
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance != I2C2) return;

    uint32_t error = hi2c->ErrorCode;
    
    // AF (Acknowledge Failure/NACK) is expected in SMBus Read Byte
    // Master sends NACK after the last byte to signal end of transaction
    if (error == HAL_I2C_ERROR_AF)
    {
        debug_log(">>> NACK received (expected at end of read)\r\n");
    }
    else
    {
        // Real error (BERR, OVR, etc.)
        debug_log("I2C ERROR: 0x%08X\r\n", error);
    }

    // Recover and prepare for next transaction
    currentState = STATE_IDLE;
    hi2c->ErrorCode = HAL_I2C_ERROR_NONE;
    hi2c->State = HAL_I2C_STATE_READY;
    __HAL_UNLOCK(hi2c);
    HAL_I2C_EnableListen_IT(hi2c);
}

// IRQ handler
void I2C2_IRQHandler(void)
{
    HAL_I2C_EV_IRQHandler(&hi2c2);
    HAL_I2C_ER_IRQHandler(&hi2c2);
}
