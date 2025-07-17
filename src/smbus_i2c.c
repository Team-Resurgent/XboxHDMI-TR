#include "smbus_i2c.h"
#include "stm32.h"
#include "debug.h"

volatile uint8_t bit_count = 0;
volatile uint8_t byte_buffer = 0;
volatile uint8_t sniffing = 0;

void smbus_i2c_init()
{
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_SYSCFG_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_10 | GPIO_PIN_11;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &gpio);

    // Setup EXTI on SDA (PB11) for START condition detection (falling edge)
    SYSCFG->EXTICR[2] &= ~(SYSCFG_EXTICR3_EXTI11);     // Clear previous
    SYSCFG->EXTICR[2] |= SYSCFG_EXTICR3_EXTI11_PB;
    EXTI->IMR |= EXTI_IMR_MR11;
    EXTI->FTSR |= EXTI_FTSR_TR11;

    // Setup EXTI on SCL (PB10) for clock bit sampling (rising edge)
    SYSCFG->EXTICR[2] &= ~(SYSCFG_EXTICR3_EXTI10);     // Clear previous
    SYSCFG->EXTICR[2] |= SYSCFG_EXTICR3_EXTI10_PB;
    EXTI->IMR |= EXTI_IMR_MR10;
    EXTI->RTSR |= EXTI_RTSR_TR10;

    NVIC_EnableIRQ(EXTI4_15_IRQn);  // PB10 and PB11 fall in EXTI4_15
}

void EXTI4_15_IRQHandler(void)
{
    // START condition: SDA falling while SCL high
    if (EXTI->PR & EXTI_PR_PR11)
    {
        if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_10)) // SCL high?
        {
            sniffing = 1;
            bit_count = 0;
            byte_buffer = 0;
        }
        EXTI->PR |= EXTI_PR_PR11;
    }

    // Bit clock: read on SCL rising edge
    if (EXTI->PR & EXTI_PR_PR10)
    {
        EXTI->PR |= EXTI_PR_PR10;

        if (!sniffing) return;

        int sda = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_11);
        byte_buffer = (byte_buffer << 1) | (sda ? 1 : 0);
        bit_count++;

        if (bit_count == 8)
        {
            sniffing = 0;
            uint8_t addr = byte_buffer >> 1;
            uint8_t rw = byte_buffer & 1;
            debug_log("I2C Addr: 0x%02X %s\r\n", addr, rw ? "READ" : "WRITE");
        }
    }
}