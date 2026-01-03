#pragma once

// ============================================================================
// MEMORY
// ============================================================================

#define RAM_START_ADDRESS       0x20000000
#define RAM_TOTAL_SIZE          0x2000
#define FLASH_START_ADDRESS     0x08000000
#define FLASH_TOTAL_SIZE        0x10000

#define BOOTLOADER_MAGIC_VALUE      0xDEADBEEF
#define BOOTLOADER_FLAG_ADDRESS    ((volatile uint32_t*)(RAM_START_ADDRESS + 0xF0))

// Bootloader and application addresses (16KB bootloader)
#define BOOTLOADER_SIZE         0x4000  // 16KB
#define APP_START_ADDRESS       (FLASH_START_ADDRESS + BOOTLOADER_SIZE)
#define APP_TOTAL_SIZE          (FLASH_START_ADDRESS +  FLASH_TOTAL_SIZE - BOOTLOADER_SIZE)

// ============================================================================
// I2C
// ============================================================================

#define I2C_SLAVE_ADDR 0x69
#define I2C_WRITE_BIT 0x80

// Read Actions
#define I2C_HDMI_COMMAND_READ_STORE 0
#define I2C_HDMI_COMMAND_READ_VERSION1 1
#define I2C_HDMI_COMMAND_READ_VERSION2 2
#define I2C_HDMI_COMMAND_READ_VERSION3 3
#define I2C_HDMI_COMMAND_READ_VERSION4 4

// Write Actions
#define I2C_HDMI_COMMAND_WRITE_STORE 128
#define I2C_HDMI_COMMAND_WRITE_BANK 129
#define I2C_HDMI_COMMAND_WRITE_INDEX 130
#define I2C_HDMI_COMMAND_WRITE_APPLY 131
#define I2C_HDMI_COMMAND_WRITE_BOOTLOADER 132

// ============================================================================
// SMBUS
// ============================================================================

#define SMBUS_SMS_NONE           ((uint32_t)0x00000000)  /*!< Uninitialized stack */
#define SMBUS_SMS_READY          ((uint32_t)0x00000001)  /*!< No operation ongoing */
#define SMBUS_SMS_TRANSMIT       ((uint32_t)0x00000002)  /*!< State of writing data to the bus */
#define SMBUS_SMS_RECEIVE        ((uint32_t)0x00000004)  /*!< State of receiving data on the bus */
#define SMBUS_SMS_PROCESSING     ((uint32_t)0x00000008)  /*!< Processing block (variable length transmissions) */
#define SMBUS_SMS_RESPONSE_READY ((uint32_t)0x00000010)  /*!< Slave has reply ready for transmission */
#define SMBUS_SMS_IGNORED        ((uint32_t)0x00000020)  /*!< The current command is not intended for this slave, ignore it */