#pragma once

// ============================================================================
// MEMORY
// ============================================================================

#define RAM_START_ADDRESS       0x20000000
#define RAM_TOTAL_SIZE          0x2000
#define FLASH_START_ADDRESS     0x08000000
#define FLASH_TOTAL_SIZE        0x10000
#define FLASH_PAGE_SHIFT        10

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
#define I2C_HDMI_COMMAND_READ_CONFIG 0 // Read value from config at current bank + index (post increments)
#define I2C_HDMI_COMMAND_READ_VERSION1 1 // Read version byte 1
#define I2C_HDMI_COMMAND_READ_VERSION2 2 // Read version byte 2
#define I2C_HDMI_COMMAND_READ_VERSION3 3 // Read version byte 3
#define I2C_HDMI_COMMAND_READ_VERSION4 4 // Read version byte 4
#define I2C_HDMI_COMMAND_READ_MODE 5 // Read current mode (1=Bootloader 2=Application)

#define I2C_HDMI_COMMAND_READ_RAM 6 // Read value from ram buffer at bank + index (post increments)
#define I2C_HDMI_COMMAND_READ_RAM_PAGE_CRC1 7 // Read crc byte 1 (from ram buffer)
#define I2C_HDMI_COMMAND_READ_RAM_PAGE_CRC2 8 // Read crc byte 2 (from ram buffer)
#define I2C_HDMI_COMMAND_READ_RAM_PAGE_CRC3 9 // Read crc byte 3 (from ram buffer)
#define I2C_HDMI_COMMAND_READ_RAM_PAGE_CRC4 10 // Read crc byte 4 (from ram buffer)

// Write Actions
#define I2C_HDMI_COMMAND_WRITE_CONFIG 128 // Write value to config buffer at current bank + index (post increments)
#define I2C_HDMI_COMMAND_WRITE_CONFIG_BANK 129 // Write config buffer bank (sets index to 0)
#define I2C_HDMI_COMMAND_WRITE_CONFIG_INDEX 130 // Write config buffer index
#define I2C_HDMI_COMMAND_WRITE_CONFIG_APPLY 131 // Applies config when value is 1
#define I2C_HDMI_COMMAND_WRITE_SET_MODE 132 // Reboots to Bootloader or Application (1=Bootloader 2=Application) if not already in that mode

#define I2C_HDMI_COMMAND_WRITE_READ_PAGE 133 // Reads page using given value into ram buffer
#define I2C_HDMI_COMMAND_WRITE_RAM 134 // Write value to ram buffer at bank + index (post increments)
#define I2C_HDMI_COMMAND_WRITE_RAM_BANK 135 // Write ram buffer bank (sets index to 0)
#define I2C_HDMI_COMMAND_WRITE_RAM_INDEX 136 // Write ram buffer index
#define I2C_HDMI_COMMAND_WRITE_RAM_APPLY 137 // Applies ram buffer to page with value alseo erases + updates crc (validates page for current mode)

#define I2C_HDMI_VERSION1 0
#define I2C_HDMI_VERSION2 1
#define I2C_HDMI_VERSION3 0
#define I2C_HDMI_VERSION4 0

#define I2C_HDMI_MODE_BOOTLOADER 1
#define I2C_HDMI_MODE_APPLICATION 2

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

#define RAM_BUFFER_SIZE 1024
