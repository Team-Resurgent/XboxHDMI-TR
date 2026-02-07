#include "main.h"
#include "stm32f0xx_hal.h"
#include "stm32f0xx.h"
#include <string.h>
#include "../shared/adv7511_xbox.h"
#include "../shared/crc32.h"
#include "../shared/defines.h"
#include "../shared/debug.h"
#include "../shared/error_handler.h"
#include "../shared/xbox_video_standalone.h"
#include "../shared/gpio.h"
#include "smbus_i2c.h"

extern void SystemClock_Config(void);

adv7511 encoder;

// Allow user to force any of the 3 encoders, only required for vic mode
#ifdef BUILD_XCALIBUR
    xbox_encoder xb_encoder = ENCODER_XCALIBUR;
#elif BUILD_FOCUS
    xbox_encoder xb_encoder = ENCODER_FOCUS;
#else
    xbox_encoder xb_encoder = ENCODER_CONEXANT;
#endif

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    init_gpio();

    debug_init();
    debug_log("Entering Bootloader...\r\n");

    uint32_t flag_value = *BOOTLOADER_FLAG_ADDRESS;
    bool magic_set = (flag_value == BOOTLOADER_MAGIC_VALUE);
    bool recovery = recovery_jumper_enabled();
    *BOOTLOADER_FLAG_ADDRESS = 0;

    if (!magic_set && !recovery && can_launch_application()) {
        jump_to_application();
    }
    enter_bootloader_mode();
}

bool can_launch_application(void)
{
    volatile uint32_t *app_vector_table = (volatile uint32_t *)APP_START_ADDRESS;
    uint32_t app_entry = app_vector_table[1];

    if (app_entry < FLASH_START_ADDRESS || app_entry > (FLASH_START_ADDRESS + FLASH_TOTAL_SIZE)) {
        return false;
    }

    if (app_entry == 0xFFFFFFFF) {
        return false;
    }

    uint16_t flash_flag = *(volatile uint16_t *)APP_INVALID_FLAG_ADDRESS;
    if (flash_flag == APP_INVALID_FLAG) {
        return false;
    }

    return true;
}

void jump_to_application(void)
{
    debug_log("Launching Application...\r\n");

    volatile uint32_t *app_vector_table = (volatile uint32_t *)APP_START_ADDRESS;
    uint32_t app_entry = app_vector_table[1];

    __disable_irq();

    void (*app_reset_handler)(void) = (void (*)(void))app_entry;
    __set_MSP(*(volatile uint32_t*)BOOTLOADER_SIZE);
    app_reset_handler();

    while (1);
}

void enter_bootloader_mode(void)
{
    debug_log("Waiting for update...\r\n");

    smbus_i2c_init();
    init_gpio();
    init_adv(&encoder, xb_encoder);

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

        // ADV handling for VIC mode for emergency
        adv_handle_interrupts(&encoder);
        stand_alone_loop(&encoder, xb_encoder);
    }
}
