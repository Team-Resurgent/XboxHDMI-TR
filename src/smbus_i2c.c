#include "smbus_i2c.h"
#include "stm32.h"
#include "debug.h"

#define I2C_SLAVE_ADDR 0x69

I2C_HandleTypeDef hi2c2;
uint8_t rx_data[2];     // [0] = command, [1] = data
uint8_t tx_data[256];   // Response data indexed by command

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

    // Configure I2C2 as slave
    hi2c2.Instance = I2C2;
    hi2c2.Init.Timing = 0x2000090E;
    hi2c2.Init.OwnAddress1 = (I2C_SLAVE_ADDR << 1);
    hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    
    HAL_I2C_Init(&hi2c2);
    
    // Start listening for address match
    HAL_I2C_EnableListen_IT(&hi2c2);
    
    debug_log("I2C Slave 0x%02X ready\r\n", I2C_SLAVE_ADDR);
}

// Called when our address is matched
void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection, uint16_t AddrMatchCode)
{
    if (TransferDirection == I2C_DIRECTION_TRANSMIT)  // Master writing to us
    {
        debug_log("I2C ADDR: Master Write\r\n");
        HAL_I2C_Slave_Seq_Receive_IT(hi2c, rx_data, 2, I2C_FIRST_AND_LAST_FRAME);
    }
    else  // Master reading from us
    {
        debug_log("I2C ADDR: Master Read\r\n");
        HAL_I2C_Slave_Seq_Transmit_IT(hi2c, tx_data, 1, I2C_FIRST_AND_LAST_FRAME);
    }
}

// Called when we finish receiving data (command + data)
void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    uint8_t command = rx_data[0];
    uint8_t data = rx_data[1];
    
    debug_log("I2C RX: CMD=0x%02X DATA=0x%02X\r\n", command, data);
    
    // Store received data for later read-back
    tx_data[command] = data;
    
    // Listen for next transaction
    HAL_I2C_EnableListen_IT(hi2c);
}

// Called when we finish transmitting data
void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    debug_log("I2C TX: 0x%02X sent\r\n", tx_data[0]);
    HAL_I2C_EnableListen_IT(hi2c);
}

// Called on I2C error
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    debug_log("I2C Error: 0x%08X\r\n", hi2c->ErrorCode);
    HAL_I2C_EnableListen_IT(hi2c);
}

void I2C2_IRQHandler(void)
{
    HAL_I2C_EV_IRQHandler(&hi2c2);
    HAL_I2C_ER_IRQHandler(&hi2c2);
}