#include "main.h"
#include "stm32f0xx_hal.h"
#include "stm32f0xx.h"
#include "smbus_i2c.h"
#include "../shared/defines.h"
#include "../shared/led.h"
#include <string.h>
#include "../shared/debug.h"
#include "../shared/error_handler.h"

extern void SystemClock_Config(void);
volatile uint8_t bootloader_running = 0;

int main(void)
{
    bootloader_running = 1;

    HAL_Init();
    SystemClock_Config();

    debug_init();
    debug_log("Entering Bootloader...\r\n");

    uint32_t flag_value = *BOOTLOADER_FLAG_ADDRESS;
    if (flag_value != BOOTLOADER_MAGIC_VALUE && can_launch_application())
    {
        jump_to_application();
    }
    *BOOTLOADER_FLAG_ADDRESS = 0;
    enter_bootloader_mode();
}

bool can_launch_application(void)
{
    volatile uint32_t *app_vector_table = (volatile uint32_t *)APP_START_ADDRESS;
    uint32_t stack_pointer = app_vector_table[0];
    uint32_t app_entry = app_vector_table[1];

    if (stack_pointer < RAM_START_ADDRESS || stack_pointer > (RAM_START_ADDRESS + RAM_TOTAL_SIZE)) {
        return false;
    }

    if (app_entry < FLASH_START_ADDRESS || app_entry > (FLASH_START_ADDRESS + FLASH_TOTAL_SIZE)) {
        return false;
    }

    if (stack_pointer == 0xFFFFFFFF && app_entry == 0xFFFFFFFF) {
        return false;
    }

    return true;
}

void jump_to_application(void)
{
    debug_log("Launching Application...\r\n");

    volatile uint32_t *app_vector_table = (volatile uint32_t *)APP_START_ADDRESS;
    uint32_t stack_pointer = app_vector_table[0];
    uint32_t app_entry = app_vector_table[1];

    __disable_irq();

    HAL_RCC_DeInit();
    HAL_DeInit();

    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

    bootloader_running = 0;

    asm volatile (
        "mov sp,%0 ; blx %1"
        :: "r" (stack_pointer), "r" (app_entry)
    );

    while (1);
}

void enter_bootloader_mode(void)
{
    init_led();

    debug_log("Waiting for update...\r\n");

    smbus_i2c_init();

    static uint32_t last_blink = 0;
    static bool led_state = false;

    while(1) {
        if ((HAL_GetTick() - last_blink) > 100) {
            led_state = !led_state;
            set_led_1(led_state);
            set_led_2(!led_state);
            last_blink = HAL_GetTick();
        }
        HAL_Delay(10);
    }
}
