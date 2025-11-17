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

static SMBUS_HandleTypeDef hi2c2;
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
    hi2c2.Init.AnalogFilter = SMBUS_ANALOGFILTER_DISABLE;  // Disable analog filter
    hi2c2.Init.OwnAddress1 = (I2C_SLAVE_ADDR << 1);
    hi2c2.Init.AddressingMode = SMBUS_ADDRESSINGMODE_7BIT;
    hi2c2.Init.DualAddressMode = SMBUS_DUALADDRESS_DISABLE;
    hi2c2.Init.OwnAddress2 = 0;
    hi2c2.Init.OwnAddress2Masks = SMBUS_OA2_NOMASK;
    hi2c2.Init.GeneralCallMode = SMBUS_GENERALCALL_DISABLE;
    hi2c2.Init.PacketErrorCheckMode = SMBUS_PEC_DISABLE;
    hi2c2.Init.PeripheralMode = SMBUS_PERIPHERAL_MODE_SMBUS_SLAVE;
    hi2c2.Init.SMBusTimeout = 0x00008249;  // Enable SMBus timeout

    if (HAL_SMBUS_Init(&hi2c2) != HAL_OK)
    {
        debug_log("SMBUS I2C init failed\n");
        while(1);
    }

    if (HAL_SMBUS_ConfigDigitalFilter(&hi2c2, 0) != HAL_OK)
    {
        debug_log("SMBUS I2C Config digital filter failed\n");
        while(1);
    }

    HAL_NVIC_SetPriority(I2C2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(I2C2_IRQn);
    
    // Start listening for address match
    HAL_SMBUS_EnableListen_IT(&hi2c2);
    
    currentState = STATE_IDLE;
    debug_log("I2C Slave 0x%02X ready\r\n", I2C_SLAVE_ADDR);
}

// This gets called when our address is matched
void HAL_SMBUS_AddrCallback(SMBUS_HandleTypeDef *psmbus, uint8_t TransferDirection, uint16_t AddrMatchCode)
{
    if (psmbus->Instance != I2C2) return;

    if (TransferDirection == I2C_DIRECTION_RECEIVE)
    {
        // Master wants to READ from us
        debug_log("AddrMatch READ\r\n");
        
        if (currentState == STATE_COMMAND_RECEIVED)
        {
            // Send our response using LAST_FRAME - not FIRST_AND_LAST
            debug_log("Sending response: 0x%02X\r\n", responseByte);
            HAL_SMBUS_Slave_Transmit_IT(psmbus, &responseByte, 1, SMBUS_LAST_FRAME_NO_PEC);
        }
    }
    else
    {
        // Master wants to WRITE to us (send command)
        debug_log("AddrMatch WRITE\r\n");
        currentState = STATE_WAIT_COMMAND;
        
        HAL_SMBUS_Slave_Receive_IT(psmbus, &commandByte, 1, SMBUS_FIRST_AND_LAST_FRAME_NO_PEC);
    }
}

// This gets called when we finish receiving
void HAL_SMBUS_SlaveRxCpltCallback(SMBUS_HandleTypeDef *psmbus)
{
    if (psmbus->Instance != I2C2) return;

    debug_log("RxCplt: cmd=0x%02X\r\n", commandByte);
    currentState = STATE_COMMAND_RECEIVED;
    
    // Prepare response based on command
    // For now, always return 0x42
    responseByte = 0x42;
    
    debug_log("Response ready, waiting for repeated START\r\n");
}

// This gets called when we finish transmitting
void HAL_SMBUS_SlaveTxCpltCallback(SMBUS_HandleTypeDef *psmbus)
{
    if (psmbus->Instance != I2C2) return;
    
    debug_log("TxCplt - Byte sent, waiting for master NACK/STOP...\r\n");
    
    // Do absolutely nothing - let timeout or error callback handle it
    // The master should NACK and send STOP, which should trigger error or listen complete
}

// This gets called when transaction completes (STOP condition)
void HAL_SMBUS_ListenCpltCallback(SMBUS_HandleTypeDef *psmbus)
{
    if (psmbus->Instance != I2C2) return;
    
    debug_log("ListenCplt - STOP received\r\n");
    currentState = STATE_IDLE;
    HAL_SMBUS_EnableListen_IT(psmbus);
}

// This gets called on errors
void HAL_SMBUS_ErrorCallback(SMBUS_HandleTypeDef *psmbus)
{
    if (psmbus->Instance != I2C2) return;

    uint32_t error = psmbus->ErrorCode;
    uint32_t isr = psmbus->Instance->ISR;
    
    debug_log("SMBUS ERROR: 0x%08X ISR=0x%08X\r\n", error, isr);
    debug_log("  ACKF=%d HALTIMEOUT=%d\r\n",
              (error & HAL_SMBUS_ERROR_ACKF) ? 1 : 0,
              (error & HAL_SMBUS_ERROR_HALTIMEOUT) ? 1 : 0);

    // NACK after slave transmit is NORMAL for SMBus Read Byte - master signals end
    if (error & HAL_SMBUS_ERROR_ACKF)
    {
        debug_log("NACK received - this is normal for Read Byte, clearing and continuing\r\n");
        __HAL_SMBUS_CLEAR_FLAG(psmbus, SMBUS_FLAG_AF);
    }

    // Reset state and re-enable listening
    currentState = STATE_IDLE;
    
    psmbus->PreviousState = psmbus->State;
    psmbus->State = HAL_SMBUS_STATE_READY;
    __HAL_UNLOCK(psmbus);
    
    HAL_SMBUS_EnableListen_IT(psmbus);
    debug_log("ErrorCallback - recovered and ready for next transaction\r\n");
}

// IRQ Handler
void I2C2_IRQHandler(void)
{
    HAL_SMBUS_EV_IRQHandler(&hi2c2);
    HAL_SMBUS_ER_IRQHandler(&hi2c2);
}
