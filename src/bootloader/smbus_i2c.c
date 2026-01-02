#include "smbus_i2c.h"
#include "stm32.h"
#include "../shared/debug.h"
#include "../shared/defines.h"
#include <string.h>

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

// Firmware Update Read Commands
#define I2C_FW_UPDATE_READ_STATUS 0x10
#define I2C_FW_UPDATE_READ_PAGE_ADDR_LOW 0x11
#define I2C_FW_UPDATE_READ_PAGE_ADDR_HIGH 0x12
#define I2C_FW_UPDATE_READ_CRC_BYTE0 0x13
#define I2C_FW_UPDATE_READ_CRC_BYTE1 0x14
#define I2C_FW_UPDATE_READ_CRC_BYTE2 0x15
#define I2C_FW_UPDATE_READ_CRC_BYTE3 0x16
#define I2C_FW_UPDATE_READ_FIRMWARE 0x17  // Read firmware byte (auto-increment)
#define I2C_FW_UPDATE_READ_ADDR_LOW 0x18   // Read current read address low byte
#define I2C_FW_UPDATE_READ_ADDR_HIGH 0x19  // Read current read address high byte
#define I2C_FW_UPDATE_READ_SIZE_BYTE0 0x1A  // Read firmware size byte 0 (LSB)
#define I2C_FW_UPDATE_READ_SIZE_BYTE1 0x1B  // Read firmware size byte 1
#define I2C_FW_UPDATE_READ_SIZE_BYTE2 0x1C  // Read firmware size byte 2
#define I2C_FW_UPDATE_READ_SIZE_BYTE3 0x1D  // Read firmware size byte 3 (MSB)

// Firmware Update Write Commands
#define I2C_FW_UPDATE_WRITE_MODE 0x90  // 128 + 16
#define I2C_FW_UPDATE_WRITE_PAGE_ADDR_LOW 0x91
#define I2C_FW_UPDATE_WRITE_PAGE_ADDR_HIGH 0x92
#define I2C_FW_UPDATE_WRITE_DATA 0x93
#define I2C_FW_UPDATE_ERASE_PAGE 0x94
#define I2C_FW_UPDATE_COMMIT_PAGE 0x95
#define I2C_FW_UPDATE_VERIFY 0x96
#define I2C_FW_UPDATE_RESET 0x97
#define I2C_FW_UPDATE_SET_READ_ADDR_LOW 0x98   // Set firmware read address low byte
#define I2C_FW_UPDATE_SET_READ_ADDR_HIGH 0x99  // Set firmware read address high byte
#define I2C_FW_UPDATE_CALCULATE_SIZE 0x9A  // Calculate and cache firmware size

// Flash configuration for STM32F030C8T6
#define FLASH_BASE_ADDR 0x08000000
#define FLASH_TOTAL_SIZE 65536  // 64KB total
#define FLASH_PAGE_COUNT (FLASH_TOTAL_SIZE / FLASH_PAGE_SIZE)

// Application start address (adjust if bootloader present)
#define APP_START_ADDR FLASH_BASE_ADDR
#define APP_END_ADDR (FLASH_BASE_ADDR + FLASH_TOTAL_SIZE - 1)

// State flags (bit flags like reference)
#define SMBUS_SMS_NONE           ((uint32_t)0x00000000)  /*!< Uninitialized stack */
#define SMBUS_SMS_READY          ((uint32_t)0x00000001)  /*!< No operation ongoing */
#define SMBUS_SMS_TRANSMIT       ((uint32_t)0x00000002)  /*!< State of writing data to the bus */
#define SMBUS_SMS_RECEIVE        ((uint32_t)0x00000004)  /*!< State of receiving data on the bus */
#define SMBUS_SMS_PROCESSING     ((uint32_t)0x00000008)  /*!< Processing block (variable length transmissions) */
#define SMBUS_SMS_RESPONSE_READY ((uint32_t)0x00000010)  /*!< Slave has reply ready for transmission */
#define SMBUS_SMS_IGNORED        ((uint32_t)0x00000020)  /*!< The current command is not intended for this slave, ignore it */

volatile I2C_HandleTypeDef hi2c2;

static uint32_t state = SMBUS_SMS_READY;
static SMBusSettings scratchSettings = {0};
static SMBusSettings settings = {0};
static uint16_t store_bank = 0;
static uint16_t store_index = 0;

static int currentCommand = -1;  // -1 means no command, otherwise stores command byte
static uint8_t commandByte = 0;
static uint8_t dataByte = 0;
static uint8_t responseByte = 0x42;  // default response

static bool video_mode_update_pending = false;
static bool bios_took_over_control = false;

// Firmware update state
static bool fw_update_mode = false;
static uint16_t fw_page_addr = 0;  // Current page address (0-63 for 64KB)
static uint16_t fw_byte_index = 0;  // Current byte index within page (0-1023)
static uint8_t fw_page_buffer[FLASH_PAGE_SIZE];  // Buffer for one flash page
static uint32_t fw_crc32 = 0;  // Calculated CRC32
static uint8_t fw_crc_read_index = 0;  // For reading CRC32 byte-by-byte
static uint32_t fw_read_addr = APP_START_ADDR;  // Current firmware read address
static uint32_t fw_size = 0;  // Calculated firmware size in bytes
static uint8_t fw_size_read_index = 0;  // For reading firmware size byte-by-byte

// -------------------- CRC32 Calculation --------------------
static uint32_t crc32_table[256];
static bool crc32_table_initialized = false;

static void init_crc32_table(void)
{
    uint32_t crc;
    for (uint32_t i = 0; i < 256; i++)
    {
        crc = i;
        for (uint32_t j = 0; j < 8; j++)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
        crc32_table[i] = crc;
    }
    crc32_table_initialized = true;
}

// -------------------- Flash Programming --------------------
static HAL_StatusTypeDef flash_erase_page(uint32_t page_addr)
{
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t PageError = 0;
    uint32_t flash_addr = FLASH_BASE_ADDR + (page_addr * FLASH_PAGE_SIZE);

    // Check if address is within application range
    if (flash_addr < APP_START_ADDR || flash_addr >= APP_END_ADDR)
    {
        debug_log("Flash erase: Invalid address 0x%08X\r\n", flash_addr);
        return HAL_ERROR;
    }

    // Unlock flash
    HAL_FLASH_Unlock();

    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = flash_addr;
    EraseInitStruct.NbPages = 1;

    HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);

    HAL_FLASH_Lock();

    if (status != HAL_OK)
    {
        debug_log("Flash erase failed: %d\r\n", status);
    }
    else
    {
        debug_log("Flash page %d erased at 0x%08X\r\n", page_addr, flash_addr);
    }

    return status;
}

static HAL_StatusTypeDef flash_write_page(uint32_t page_addr, const uint8_t *data)
{
    uint32_t flash_addr = FLASH_BASE_ADDR + (page_addr * FLASH_PAGE_SIZE);

    // Check if address is within application range
    if (flash_addr < APP_START_ADDR || flash_addr >= APP_END_ADDR)
    {
        debug_log("Flash write: Invalid address 0x%08X\r\n", flash_addr);
        return HAL_ERROR;
    }

    // Unlock flash
    HAL_FLASH_Unlock();

    HAL_StatusTypeDef status = HAL_OK;
    // Write half-words (16-bit) as required by STM32F0
    for (uint32_t i = 0; i < FLASH_PAGE_SIZE; i += 2)
    {
        uint16_t halfword = data[i] | (data[i + 1] << 8);
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, flash_addr + i, halfword);
        if (status != HAL_OK)
        {
            debug_log("Flash write failed at offset %d: %d\r\n", i, status);
            break;
        }
    }

    HAL_FLASH_Lock();

    if (status == HAL_OK)
    {
        debug_log("Flash page %d written at 0x%08X\r\n", page_addr, flash_addr);
    }

    return status;
}

static uint32_t flash_verify_crc32(uint32_t start_addr, uint32_t length)
{
    uint8_t buffer[256];  // Read in chunks
    uint32_t crc = 0xFFFFFFFF;

    if (!crc32_table_initialized)
        init_crc32_table();

    // Calculate CRC32 incrementally across all flash data
    for (uint32_t offset = 0; offset < length; offset += sizeof(buffer))
    {
        uint32_t chunk_size = (length - offset < sizeof(buffer)) ? (length - offset) : sizeof(buffer);
        const uint8_t *flash_ptr = (const uint8_t *)(start_addr + offset);

        // Update CRC32 for this chunk (incremental calculation)
        for (uint32_t i = 0; i < chunk_size; i++)
        {
            crc = (crc >> 8) ^ crc32_table[(crc ^ flash_ptr[i]) & 0xFF];
        }
    }

    return crc ^ 0xFFFFFFFF;
}

// -------------------- Firmware Size Calculation --------------------
static uint32_t calculate_firmware_size(void)
{
    // Start from the end and work backwards to find the last non-empty page
    // A page is considered empty if all bytes are 0xFF (erased flash)
    uint32_t last_page = 0;
    bool found_data = false;
    
    // Check pages from end to beginning
    for (int32_t page = FLASH_PAGE_COUNT - 1; page >= 0; page--)
    {
        uint32_t page_addr = FLASH_BASE_ADDR + (page * FLASH_PAGE_SIZE);
        const uint8_t *page_ptr = (const uint8_t *)page_addr;
        
        // Check if page contains any non-0xFF data
        bool page_has_data = false;
        for (uint32_t i = 0; i < FLASH_PAGE_SIZE; i++)
        {
            if (page_ptr[i] != 0xFF)
            {
                page_has_data = true;
                found_data = true;
                break;
            }
        }
        
        if (page_has_data)
        {
            last_page = page;
            break;
        }
    }
    
    if (!found_data)
    {
        // No firmware found, return minimum size (vector table is at least 8 bytes)
        // Check if vector table is valid
        const uint32_t *vector_table = (const uint32_t *)APP_START_ADDR;
        uint32_t stack_ptr = vector_table[0];
        uint32_t reset_handler = vector_table[1];
        
        // Basic validation: stack pointer should be in RAM range (0x20000000-0x20002000 for 8KB)
        // Reset handler should be in flash range (0x08000000-0x08010000)
        if ((stack_ptr >= 0x20000000 && stack_ptr < 0x20002000) &&
            (reset_handler >= 0x08000000 && reset_handler < 0x08010000))
        {
            // Vector table is valid, firmware exists but might be very small
            // Return at least one page
            return FLASH_PAGE_SIZE;
        }
        return 0;
    }
    
    // Return size up to and including the last page with data
    return (last_page + 1) * FLASH_PAGE_SIZE;
}

// Firmware update state
static bool fw_update_mode = false;
static uint16_t fw_page_addr = 0;  // Current page address (0-63 for 64KB)
static uint16_t fw_byte_index = 0;  // Current byte index within page (0-1023)
static uint8_t fw_page_buffer[FLASH_PAGE_SIZE];  // Buffer for one flash page
static uint32_t fw_crc32 = 0;  // Calculated CRC32
static uint8_t fw_crc_read_index = 0;  // For reading CRC32 byte-by-byte
static uint32_t fw_read_addr = APP_START_ADDR;  // Current firmware read address
static uint32_t fw_size = 0;  // Calculated firmware size in bytes
static uint8_t fw_size_read_index = 0;  // For reading firmware size byte-by-byte

// -------------------- CRC32 Calculation --------------------
static uint32_t crc32_table[256];
static bool crc32_table_initialized = false;

static void init_crc32_table(void)
{
    uint32_t crc;
    for (uint32_t i = 0; i < 256; i++)
    {
        crc = i;
        for (uint32_t j = 0; j < 8; j++)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
        crc32_table[i] = crc;
    }
    crc32_table_initialized = true;
}

// -------------------- Flash Programming --------------------
static HAL_StatusTypeDef flash_erase_page(uint32_t page_addr)
{
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t PageError = 0;
    uint32_t flash_addr = FLASH_BASE_ADDR + (page_addr * FLASH_PAGE_SIZE);

    // Check if address is within application range
    if (flash_addr < APP_START_ADDR || flash_addr >= APP_END_ADDR)
    {
        debug_log("Flash erase: Invalid address 0x%08X\r\n", flash_addr);
        return HAL_ERROR;
    }

    // Unlock flash
    HAL_FLASH_Unlock();

    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = flash_addr;
    EraseInitStruct.NbPages = 1;

    HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);

    HAL_FLASH_Lock();

    if (status != HAL_OK)
    {
        debug_log("Flash erase failed: %d\r\n", status);
    }
    else
    {
        debug_log("Flash page %d erased at 0x%08X\r\n", page_addr, flash_addr);
    }

    return status;
}

static HAL_StatusTypeDef flash_write_page(uint32_t page_addr, const uint8_t *data)
{
    uint32_t flash_addr = FLASH_BASE_ADDR + (page_addr * FLASH_PAGE_SIZE);

    // Check if address is within application range
    if (flash_addr < APP_START_ADDR || flash_addr >= APP_END_ADDR)
    {
        debug_log("Flash write: Invalid address 0x%08X\r\n", flash_addr);
        return HAL_ERROR;
    }

    // Unlock flash
    HAL_FLASH_Unlock();

    HAL_StatusTypeDef status = HAL_OK;
    // Write half-words (16-bit) as required by STM32F0
    for (uint32_t i = 0; i < FLASH_PAGE_SIZE; i += 2)
    {
        uint16_t halfword = data[i] | (data[i + 1] << 8);
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, flash_addr + i, halfword);
        if (status != HAL_OK)
        {
            debug_log("Flash write failed at offset %d: %d\r\n", i, status);
            break;
        }
    }

    HAL_FLASH_Lock();

    if (status == HAL_OK)
    {
        debug_log("Flash page %d written at 0x%08X\r\n", page_addr, flash_addr);
    }

    return status;
}

static uint32_t flash_verify_crc32(uint32_t start_addr, uint32_t length)
{
    uint8_t buffer[256];  // Read in chunks
    uint32_t crc = 0xFFFFFFFF;

    if (!crc32_table_initialized)
        init_crc32_table();

    // Calculate CRC32 incrementally across all flash data
    for (uint32_t offset = 0; offset < length; offset += sizeof(buffer))
    {
        uint32_t chunk_size = (length - offset < sizeof(buffer)) ? (length - offset) : sizeof(buffer);
        const uint8_t *flash_ptr = (const uint8_t *)(start_addr + offset);

        // Update CRC32 for this chunk (incremental calculation)
        for (uint32_t i = 0; i < chunk_size; i++)
        {
            crc = (crc >> 8) ^ crc32_table[(crc ^ flash_ptr[i]) & 0xFF];
        }
    }

    return crc ^ 0xFFFFFFFF;
}

// -------------------- Firmware Size Calculation --------------------
static uint32_t calculate_firmware_size(void)
{
    // Start from the end and work backwards to find the last non-empty page
    // A page is considered empty if all bytes are 0xFF (erased flash)
    uint32_t last_page = 0;
    bool found_data = false;
    
    // Check pages from end to beginning
    for (int32_t page = FLASH_PAGE_COUNT - 1; page >= 0; page--)
    {
        uint32_t page_addr = FLASH_BASE_ADDR + (page * FLASH_PAGE_SIZE);
        const uint8_t *page_ptr = (const uint8_t *)page_addr;
        
        // Check if page contains any non-0xFF data
        bool page_has_data = false;
        for (uint32_t i = 0; i < FLASH_PAGE_SIZE; i++)
        {
            if (page_ptr[i] != 0xFF)
            {
                page_has_data = true;
                found_data = true;
                break;
            }
        }
        
        if (page_has_data)
        {
            last_page = page;
            break;
        }
    }
    
    if (!found_data)
    {
        // No firmware found, return minimum size (vector table is at least 8 bytes)
        // Check if vector table is valid
        const uint32_t *vector_table = (const uint32_t *)APP_START_ADDR;
        uint32_t stack_ptr = vector_table[0];
        uint32_t reset_handler = vector_table[1];
        
        // Basic validation: stack pointer should be in RAM range (0x20000000-0x20002000 for 8KB)
        // Reset handler should be in flash range (0x08000000-0x08010000)
        if ((stack_ptr >= 0x20000000 && stack_ptr < 0x20002000) &&
            (reset_handler >= 0x08000000 && reset_handler < 0x08010000))
        {
            // Vector table is valid, firmware exists but might be very small
            // Return at least one page
            return FLASH_PAGE_SIZE;
        }
        return 0;
    }
    
    // Return size up to and including the last page with data
    return (last_page + 1) * FLASH_PAGE_SIZE;
}

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

    if(HAL_I2C_Init((I2C_HandleTypeDef*)&hi2c2) != HAL_OK)
    {
        debug_log("SMBUS I2C init failed\n");
        while(1);
    }

    if (HAL_I2CEx_ConfigAnalogFilter((I2C_HandleTypeDef*)&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
    {
        debug_log("SMBUS I2C config analog filter failed\n");
        while(1);
    }

    if (HAL_I2CEx_ConfigDigitalFilter((I2C_HandleTypeDef*)&hi2c2, 0) != HAL_OK)
    {
        debug_log("SMBUS I2C config digital filter failed\n");
        while(1);
    }

    HAL_NVIC_SetPriority(I2C2_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(I2C2_IRQn);

    // Start listening for master
    HAL_I2C_EnableListen_IT((I2C_HandleTypeDef*)&hi2c2);

    state = SMBUS_SMS_READY;
    currentCommand = -1;
    
    // Initialize firmware update state
    fw_update_mode = false;
    fw_page_addr = 0;
    fw_byte_index = 0;
    fw_crc32 = 0;
    fw_crc_read_index = 0;
    fw_read_addr = APP_START_ADDR;
    fw_size = 0;  // Will be calculated on demand
    fw_size_read_index = 0;
    memset(fw_page_buffer, 0xFF, FLASH_PAGE_SIZE);
    
    debug_log("SMBus: I2C Slave 0x%02X ready\r\n", I2C_SLAVE_ADDR);
}

// -------------------- Address Match Callback --------------------
void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection, uint16_t AddrMatchCode)
{
    if(hi2c->Instance != I2C2) return;

    if(TransferDirection == I2C_DIRECTION_RECEIVE)
    {
        // Master reads (slave transmits)
        if ((state & SMBUS_SMS_IGNORED) == SMBUS_SMS_IGNORED)
        {
            // Best way not to block the line
            LL_I2C_TransmitData8(hi2c->Instance, 0xFFU);
            if (LL_I2C_IsActiveFlag_TCR(hi2c->Instance))
            {
                LL_I2C_SetTransferSize(hi2c->Instance, 1);
            }
            __HAL_I2C_GENERATE_NACK(hi2c);
            LL_I2C_ClearFlag_ADDR(hi2c->Instance);
            state &= ~SMBUS_SMS_IGNORED;
            HAL_I2C_EnableListen_IT(hi2c);
        }
        else
        {
            // If we don't have a response yet, try to get one
            if (!(state & (SMBUS_SMS_READY | SMBUS_SMS_RESPONSE_READY)) && currentCommand != -1)
            {
                // For firmware read command, prepare response fresh each time (address auto-increments)
                if (currentCommand == I2C_FW_UPDATE_READ_FIRMWARE)
                {
                    // Prepare response fresh for each read
                    if (fw_read_addr >= APP_START_ADDR && fw_read_addr < APP_END_ADDR)
                    {
                        const uint8_t *flash_ptr = (const uint8_t *)fw_read_addr;
                        responseByte = *flash_ptr;
                        fw_read_addr++;
                        // Wrap around if we exceed end address
                        if (fw_read_addr > APP_END_ADDR)
                        {
                            fw_read_addr = APP_START_ADDR;
                        }
                        state |= SMBUS_SMS_RESPONSE_READY;
                    }
                    else
                    {
                        responseByte = 0xFF;  // Invalid address
                        state |= SMBUS_SMS_RESPONSE_READY;
                    }
                }
                else if (currentCommand != -1)
                {
                    // Response already prepared in SlaveRxCpltCallback for other commands
                    state |= SMBUS_SMS_RESPONSE_READY;
                }
                else
                {
                    state |= SMBUS_SMS_IGNORED;
                    __HAL_I2C_GENERATE_NACK(hi2c);
                    LL_I2C_ClearFlag_ADDR(hi2c->Instance);
                    HAL_I2C_EnableListen_IT(hi2c);
                }
            }

            if (state & SMBUS_SMS_RESPONSE_READY)
            {
                state |= SMBUS_SMS_TRANSMIT;
                // Disable SBC, cannot NACK on TX
                LL_I2C_DisableSlaveByteControl(hi2c->Instance);
                //debug_ring_log("SMBus: AddrR cmd=0x%02X resp=0x%02X\r\n", currentCommand, responseByte);
                HAL_I2C_Slave_Seq_Transmit_IT(hi2c, &responseByte, 1, I2C_LAST_FRAME);
            }
            else
            {
                //debug_ring_log("SMBus: AddrR NO_CMD\r\n");
                __HAL_I2C_GENERATE_NACK(hi2c); // NACK if no command
                LL_I2C_ClearFlag_ADDR(hi2c->Instance);
                HAL_I2C_EnableListen_IT(hi2c);
            }
        }
    }
    else
    {
        // Master writes (slave receives) - new command
        //debug_ring_log("SMBus: AddrW\r\n");
        state &= ~SMBUS_SMS_IGNORED;

        if (state & SMBUS_SMS_READY)
        {
            state &= ~SMBUS_SMS_READY;
            state |= SMBUS_SMS_RECEIVE;
            currentCommand = -1;
            // Enable SBC so we can NACK
            LL_I2C_EnableSlaveByteControl(hi2c->Instance);
            // Use FIRST_FRAME to allow chaining for write commands
            HAL_I2C_Slave_Seq_Receive_IT(hi2c, &commandByte, 1, I2C_NEXT_FRAME);
        }
    }
}

// -------------------- Receive Complete Callback --------------------
void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if(hi2c->Instance != I2C2) return;

    if (state & SMBUS_SMS_IGNORED)
    {
        __HAL_I2C_GENERATE_NACK(hi2c);
        if (LL_I2C_IsActiveFlag_TCR(hi2c->Instance))
        {
            LL_I2C_SetTransferSize(hi2c->Instance, 1);
        }
    }
    else
    {
        if (currentCommand == -1)
        {
            state &= ~SMBUS_SMS_RECEIVE;

            // Command byte received
            currentCommand = commandByte;

            // Read command - prepare response
            switch(commandByte)
            {
                case I2C_HDMI_COMMAND_READ_STORE:
                {
                    uint16_t settings_size = sizeof(SMBusSettings);
                    uint16_t settings_offset = (store_bank << 8) | store_index;
                    if (settings_offset >= settings_size)
                    {
                        break;
                    }
                    uint8_t *settings_data = (uint8_t*)&settings;
                    responseByte = settings_data[settings_offset];
                    store_index++;
                    if (store_index > 0xff)
                    {
                        store_index = 0;
                        store_bank++;
                    }
                    break;
                }
                case I2C_HDMI_COMMAND_READ_VERSION1:
                {
                    responseByte = 0x01;
                    break;
                }
                case I2C_HDMI_COMMAND_READ_VERSION2:
                {
                    responseByte = 0x02;
                    break;
                }
                case I2C_HDMI_COMMAND_READ_VERSION3:
                {
                    responseByte = 0x03;
                    break;
                }
                case I2C_HDMI_COMMAND_READ_VERSION4:
                {
                    responseByte = 0x04;
                    break;
                }
                // Firmware Update Read Commands
                case I2C_FW_UPDATE_READ_STATUS:
                {
                    // Status byte: bit 0 = update mode active, bit 1 = page ready, bit 2 = error
                    responseByte = (fw_update_mode ? 0x01 : 0x00) |
                                   ((fw_byte_index == FLASH_PAGE_SIZE) ? 0x02 : 0x00);
                    break;
                }
                case I2C_FW_UPDATE_READ_PAGE_ADDR_LOW:
                {
                    responseByte = fw_page_addr & 0xFF;
                    break;
                }
                case I2C_FW_UPDATE_READ_PAGE_ADDR_HIGH:
                {
                    responseByte = (fw_page_addr >> 8) & 0xFF;
                    break;
                }
                case I2C_FW_UPDATE_READ_CRC_BYTE0:
                {
                    fw_crc_read_index = 0;
                    responseByte = fw_crc32 & 0xFF;
                    break;
                }
                case I2C_FW_UPDATE_READ_CRC_BYTE1:
                {
                    fw_crc_read_index = 1;
                    responseByte = (fw_crc32 >> 8) & 0xFF;
                    break;
                }
                case I2C_FW_UPDATE_READ_CRC_BYTE2:
                {
                    fw_crc_read_index = 2;
                    responseByte = (fw_crc32 >> 16) & 0xFF;
                    break;
                }
                case I2C_FW_UPDATE_READ_CRC_BYTE3:
                {
                    fw_crc_read_index = 3;
                    responseByte = (fw_crc32 >> 24) & 0xFF;
                    break;
                }
                case I2C_FW_UPDATE_READ_FIRMWARE:
                {
                    // For firmware read, response is prepared fresh in AddrCallback for each read
                    // This allows sequential reads with auto-incrementing address
                    // Just mark that we have a valid command, response will be prepared on read
                    responseByte = 0x00;  // Dummy value, will be overwritten in AddrCallback
                    break;
                }
                case I2C_FW_UPDATE_READ_ADDR_LOW:
                {
                    responseByte = fw_read_addr & 0xFF;
                    break;
                }
                case I2C_FW_UPDATE_READ_ADDR_HIGH:
                {
                    // Return address as 3 bytes: high, mid-high, mid-low (since address is 24-bit)
                    // For simplicity, return low byte of high 16 bits
                    responseByte = (fw_read_addr >> 8) & 0xFF;
                    break;
                }
                case I2C_FW_UPDATE_READ_SIZE_BYTE0:
                {
                    fw_size_read_index = 0;
                    responseByte = fw_size & 0xFF;
                    break;
                }
                case I2C_FW_UPDATE_READ_SIZE_BYTE1:
                {
                    fw_size_read_index = 1;
                    responseByte = (fw_size >> 8) & 0xFF;
                    break;
                }
                case I2C_FW_UPDATE_READ_SIZE_BYTE2:
                {
                    fw_size_read_index = 2;
                    responseByte = (fw_size >> 16) & 0xFF;
                    break;
                }
                case I2C_FW_UPDATE_READ_SIZE_BYTE3:
                {
                    fw_size_read_index = 3;
                    responseByte = (fw_size >> 24) & 0xFF;
                    break;
                }
                default:
                {
                    responseByte = 0xFF;
                    break;
                }
            }


            if ((currentCommand & I2C_WRITE_BIT) == I2C_WRITE_BIT)
            {
                // Write command - receive data byte
                state |= SMBUS_SMS_RECEIVE;
                HAL_I2C_Slave_Seq_Receive_IT(hi2c, &dataByte, 1, I2C_LAST_FRAME);
            }
            else
            {
                // Release the SCL stretch
                LL_I2C_SetTransferSize(hi2c->Instance, 1);
            }

        }
        else
        {
            // Block Write, get the size
            if (state & SMBUS_SMS_PROCESSING)
            {
                state &= ~(SMBUS_SMS_PROCESSING | SMBUS_SMS_RECEIVE);
                state |= SMBUS_SMS_RECEIVE;
                HAL_I2C_Slave_Seq_Receive_IT(hi2c, &dataByte, 1, I2C_LAST_FRAME);
            }
            else
            {
                state &= ~SMBUS_SMS_RECEIVE;
            }
        }
    }
}

// -------------------- Transmit Complete Callback --------------------
void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if(hi2c->Instance != I2C2) return;
    state &= ~SMBUS_SMS_TRANSMIT;
}

// -------------------- Listen Complete Callback --------------------
void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if(hi2c->Instance != I2C2) return;

    // Process the write if we have a write command and all data was received
    if(currentCommand != -1 && !(state & SMBUS_SMS_RESPONSE_READY))
    {
        // Size of bytes received must match expected size
        // For single-byte writes, XferCount should be 0 (all bytes received)
        if(hi2c->XferCount == 0)
        {
            // Process the write command
            switch(currentCommand)
            {
                case I2C_HDMI_COMMAND_WRITE_STORE:
                {
                    uint16_t settings_size = sizeof(SMBusSettings);
                    uint16_t settings_offset = (store_bank << 8) | store_index;
                    if (settings_offset >= settings_size)
                    {
                        break;
                    }
                    uint8_t *scratch_settings_data = (uint8_t*)&scratchSettings;
                    scratch_settings_data[settings_offset] = dataByte;
                    store_index++;
                    if (store_index > 0xff)
                    {
                        store_index = 0;
                        store_bank++;
                    }
                    break;
                }
                case I2C_HDMI_COMMAND_WRITE_BANK:
                {
                    store_bank = dataByte;
                    store_index = 0;
                    break;
                }
                case I2C_HDMI_COMMAND_WRITE_INDEX:
                {
                     store_index = dataByte;
                     break;
                }
                case I2C_HDMI_COMMAND_WRITE_APPLY:
                {
                    bios_took_over_control = true;

                    if (dataByte == 0x01)
                    {
                        memcpy(&settings, &scratchSettings, sizeof(SMBusSettings));
                        video_mode_update_pending = true;
                        debug_ring_log("SMBus: encoder=%02X region=%02X mode=%08X title=%08X avinfo=%08X\r\n", settings.encoder, settings.region, settings.mode, settings.titleid, settings.avinfo);
                    }
                    break;
                }
                // Firmware Update Write Commands
                case I2C_FW_UPDATE_WRITE_MODE:
                {
                    // Enter (0x01) or exit (0x00) firmware update mode
                    if (dataByte == 0x01)
                    {
                        fw_update_mode = true;
                        fw_page_addr = 0;
                        fw_byte_index = 0;
                        memset(fw_page_buffer, 0xFF, FLASH_PAGE_SIZE);
                        debug_log("Firmware update mode enabled\r\n");
                    }
                    else if (dataByte == 0x00)
                    {
                        fw_update_mode = false;
                        debug_log("Firmware update mode disabled\r\n");
                    }
                    break;
                }
                case I2C_FW_UPDATE_WRITE_PAGE_ADDR_LOW:
                {
                    if (fw_update_mode)
                    {
                        fw_page_addr = (fw_page_addr & 0xFF00) | dataByte;
                        fw_byte_index = 0;
                        memset(fw_page_buffer, 0xFF, FLASH_PAGE_SIZE);
                    }
                    break;
                }
                case I2C_FW_UPDATE_WRITE_PAGE_ADDR_HIGH:
                {
                    if (fw_update_mode)
                    {
                        fw_page_addr = (fw_page_addr & 0x00FF) | (dataByte << 8);
                        fw_byte_index = 0;
                        memset(fw_page_buffer, 0xFF, FLASH_PAGE_SIZE);
                        debug_log("Set page address: %d\r\n", fw_page_addr);
                    }
                    break;
                }
                case I2C_FW_UPDATE_WRITE_DATA:
                {
                    if (fw_update_mode && fw_byte_index < FLASH_PAGE_SIZE)
                    {
                        fw_page_buffer[fw_byte_index++] = dataByte;
                    }
                    break;
                }
                case I2C_FW_UPDATE_ERASE_PAGE:
                {
                    if (fw_update_mode && fw_page_addr < FLASH_PAGE_COUNT)
                    {
                        HAL_StatusTypeDef status = flash_erase_page(fw_page_addr);
                        if (status == HAL_OK)
                        {
                            debug_log("Page %d erased successfully\r\n", fw_page_addr);
                        }
                        else
                        {
                            debug_log("Page erase failed\r\n");
                        }
                    }
                    break;
                }
                case I2C_FW_UPDATE_COMMIT_PAGE:
                {
                    if (fw_update_mode && fw_page_addr < FLASH_PAGE_COUNT)
                    {
                        if (fw_byte_index == FLASH_PAGE_SIZE)
                        {
                            HAL_StatusTypeDef status = flash_write_page(fw_page_addr, fw_page_buffer);
                            if (status == HAL_OK)
                            {
                                debug_log("Page %d committed successfully\r\n", fw_page_addr);
                                // Reset buffer for next page
                                fw_byte_index = 0;
                                memset(fw_page_buffer, 0xFF, FLASH_PAGE_SIZE);
                            }
                            else
                            {
                                debug_log("Page commit failed\r\n");
                            }
                        }
                        else
                        {
                            debug_log("Page not full: %d bytes\r\n", fw_byte_index);
                        }
                    }
                    break;
                }
                case I2C_FW_UPDATE_VERIFY:
                {
                    if (fw_update_mode)
                    {
                        // Calculate CRC32 of entire application area
                        fw_crc32 = flash_verify_crc32(APP_START_ADDR, FLASH_TOTAL_SIZE);
                        debug_log("Firmware CRC32: 0x%08X\r\n", fw_crc32);
                    }
                    break;
                }
                case I2C_FW_UPDATE_RESET:
                {
                    if (fw_update_mode && dataByte == 0xAA)  // Safety check
                    {
                        debug_log("Resetting to new firmware...\r\n");
                        HAL_Delay(100);
                        NVIC_SystemReset();
                    }
                    break;
                }
                case I2C_HDMI_COMMAND_WRITE_BOOTLOADER:
                {
                    if (dataByte == 0x01)
                    {
                        *BOOTLOADER_FLAG_ADDRESS = 0;
                        HAL_Delay(100); 
                        NVIC_SystemReset();
                    }
                }
                case I2C_FW_UPDATE_SET_READ_ADDR_LOW:
                {
                    // Set firmware read address low byte
                    fw_read_addr = (fw_read_addr & 0xFFFFFF00) | dataByte;
                    // Ensure address is within valid range
                    if (fw_read_addr < APP_START_ADDR)
                        fw_read_addr = APP_START_ADDR;
                    else if (fw_read_addr > APP_END_ADDR)
                        fw_read_addr = APP_END_ADDR;
                    break;
                }
                case I2C_FW_UPDATE_SET_READ_ADDR_HIGH:
                {
                    // Set firmware read address high bytes (16-bit address)
                    // For 64KB flash, we need 16 bits total
                    uint16_t addr_low = fw_read_addr & 0xFF;
                    uint16_t addr_high = dataByte;
                    fw_read_addr = APP_START_ADDR + ((addr_high << 8) | addr_low);
                    // Ensure address is within valid range
                    if (fw_read_addr < APP_START_ADDR)
                        fw_read_addr = APP_START_ADDR;
                    else if (fw_read_addr > APP_END_ADDR)
                        fw_read_addr = APP_END_ADDR;
                    debug_log("Set firmware read address: 0x%08X\r\n", fw_read_addr);
                    break;
                }
                case I2C_FW_UPDATE_CALCULATE_SIZE:
                {
                    // Calculate firmware size and cache it
                    fw_size = calculate_firmware_size();
                    debug_log("Calculated firmware size: %lu bytes (0x%08lX)\r\n", fw_size, fw_size);
                    break;
                }
            }
        }
    }

    // Reset state
    state = SMBUS_SMS_READY;
    currentCommand = -1;

    // Do it all again
    HAL_I2C_EnableListen_IT(hi2c);
}

// -------------------- Error Callback --------------------
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    if(hi2c->Instance != I2C2) return;

    uint32_t err = hi2c->ErrorCode;

    if (err & (HAL_I2C_ERROR_BERR | HAL_I2C_ERROR_TIMEOUT))
    {
        // Critical error - reset the stack
        if(state & (SMBUS_SMS_TRANSMIT | SMBUS_SMS_RECEIVE | SMBUS_SMS_PROCESSING))
        {
            __HAL_I2C_DISABLE(hi2c);
            while (LL_I2C_IsEnabled(hi2c->Instance)) {}
            __HAL_I2C_ENABLE(hi2c);
            if(hi2c->State != HAL_I2C_STATE_READY)
            {
                HAL_I2C_DeInit(hi2c);
                HAL_I2C_Init(hi2c);
                HAL_I2C_EnableListen_IT(hi2c);
            }
        }
        state = SMBUS_SMS_READY;
        currentCommand = -1;
    }
    else if (err & HAL_I2C_ERROR_ARLO)
    {
        // Arbitration lost - signal error
        state = SMBUS_SMS_READY;
        currentCommand = -1;
        HAL_I2C_EnableListen_IT(hi2c);
    }
    else if (err & HAL_I2C_ERROR_AF)
    {
        // NACK - expected at end of read, handle gracefully
        hi2c->PreviousState = hi2c->State;
        hi2c->State = HAL_I2C_STATE_READY;
        __HAL_UNLOCK(hi2c);
        state = SMBUS_SMS_READY;
        currentCommand = -1;
        HAL_I2C_EnableListen_IT(hi2c);
    }
    else if(err != HAL_I2C_ERROR_NONE)
    {
        state = SMBUS_SMS_READY;
        currentCommand = -1;
        HAL_I2C_EnableListen_IT(hi2c);
    }
}

bool video_mode_updated() {
    return video_mode_update_pending;
}

void ack_video_mode_update() {
    video_mode_update_pending = false;
}

const SMBusSettings * const getSMBusSettings() {
    return &settings;
}

bool bios_took_over() {
    return bios_took_over_control;
}