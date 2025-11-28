# Firmware Update Over I2C Implementation Guide

## Overview

This document describes how to implement firmware updates over I2C for the XboxHDMI-TR project. The implementation extends the existing I2C command structure to support firmware programming operations.

## Memory Layout

- **STM32F030C8T6**: 64KB Flash (0x08000000 - 0x0800FFFF)
- **Flash Page Size**: 1KB (1024 bytes) for STM32F0
- **Application Start**: Typically 0x08000000 (or 0x08001000 if bootloader present)

## I2C Command Structure

The existing I2C protocol uses:
- **Read Commands**: 0-4 (with bit 7 = 0)
- **Write Commands**: 128-131 (with bit 7 = 1)

### New Firmware Update Commands

```
Read Commands:
- 0x10: Read firmware update status
- 0x11: Read flash page address (low byte)
- 0x12: Read flash page address (high byte)
- 0x13-0x16: Read CRC32 checksum (4 bytes, sequential reads)
- 0x17: Read firmware byte from flash (auto-increments address)
- 0x18: Read current firmware read address (low byte)
- 0x19: Read current firmware read address (high byte)
- 0x1A-0x1D: Read firmware size (4 bytes, sequential reads)

Write Commands:
- 0x90 (128+16): Set firmware update mode
- 0x91: Set flash page address (low byte)
- 0x92: Set flash page address (high byte)
- 0x93: Write flash data byte (auto-increments)
- 0x94: Erase flash page
- 0x95: Commit flash page (write buffered data)
- 0x96: Verify firmware (CRC32)
- 0x97: Reset to new firmware
- 0x98: Set firmware read address (low byte)
- 0x99: Set firmware read address (high byte)
- 0x9A: Calculate firmware size (scans flash to find actual size)
```

## Implementation Approach

### 1. Flash Update Buffer
- Use RAM buffer (up to 1KB) to store one flash page
- Write data byte-by-byte via I2C
- Commit entire page when complete

### 2. Safety Features
- **CRC32 Verification**: Verify firmware integrity before reset
- **Page-by-page updates**: Only erase/write one page at a time
- **Status reporting**: Report update progress and errors
- **Bootloader protection**: Never overwrite bootloader area (if present)

### 3. Update Flow
1. Enter update mode (command 0x90)
2. Set target page address (0x91, 0x92)
3. Write data bytes sequentially (0x93)
4. Erase page (0x94)
5. Commit page (0x95)
6. Repeat for all pages
7. Verify CRC32 (0x96)
8. Reset device (0x97)

## Code Structure

The implementation adds:
- Flash programming functions using HAL Flash API
- CRC32 calculation for verification
- Update state machine
- Buffer management

## Firmware Size Detection

The firmware size is calculated by scanning flash memory from the end backwards to find the last page containing non-0xFF data (erased flash is 0xFF). This method:

- **Finds actual size**: Detects where firmware ends, not just flash capacity
- **Page-based**: Works in 1KB page increments (minimum detected size is one page)
- **Validates vector table**: Checks if vector table is valid before reporting size
- **On-demand calculation**: Size is calculated when requested (command 0x9A), not at startup

**Note**: If firmware has trailing 0xFF bytes within a page, that entire page will be counted as part of the firmware.

## Limitations

- **RAM Buffer**: Limited to 1KB (one flash page)
- **I2C Speed**: Update speed limited by I2C bandwidth
- **No Bootloader**: Current implementation assumes no separate bootloader
- **Power Loss**: Update must complete or device may be bricked
- **Size Detection**: Size detection is page-based (1KB granularity), not byte-accurate

## Usage Examples

### Download Current Firmware (Backup)

```python
import smbus
import time

I2C_ADDR = 0x69
bus = smbus.SMBus(1)  # Use I2C bus 1

def set_read_address(addr):
    """Set the firmware read address (relative to APP_START_ADDR)"""
    bus.write_byte_data(I2C_ADDR, 0x98, addr & 0xFF)
    bus.write_byte_data(I2C_ADDR, 0x99, (addr >> 8) & 0xFF)

def read_firmware_byte():
    """Read one byte from firmware flash (auto-increments)"""
    return bus.read_byte(I2C_ADDR, 0x17)

def download_firmware(filename='firmware_backup.bin', size=65536):
    """Download current firmware from device"""
    print(f"Downloading {size} bytes to {filename}...")
    
    # Set starting address
    set_read_address(0)
    
    firmware_data = bytearray()
    for i in range(size):
        byte_val = read_firmware_byte()
        firmware_data.append(byte_val)
        
        if (i + 1) % 1024 == 0:
            print(f"Downloaded {i + 1}/{size} bytes ({100 * (i + 1) // size}%)")
        time.sleep(0.001)  # Small delay
    
    # Save to file
    with open(filename, 'wb') as f:
        f.write(firmware_data)
    
    print(f"Firmware saved to {filename}")
    return firmware_data

# Download firmware
firmware = download_firmware('current_firmware.bin')
```

### Get Firmware Size

```python
import smbus

I2C_ADDR = 0x69
bus = smbus.SMBus(1)

def calculate_firmware_size():
    """Calculate and return actual firmware size"""
    # Trigger size calculation
    bus.write_byte_data(I2C_ADDR, 0x9A, 0x00)
    time.sleep(0.1)  # Give it time to calculate
    
    # Read size as 4 bytes (little-endian)
    size0 = bus.read_byte(I2C_ADDR, 0x1A)
    size1 = bus.read_byte(I2C_ADDR, 0x1B)
    size2 = bus.read_byte(I2C_ADDR, 0x1C)
    size3 = bus.read_byte(I2C_ADDR, 0x1D)
    
    size = size0 | (size1 << 8) | (size2 << 16) | (size3 << 24)
    return size

def download_firmware_smart(filename='firmware_backup.bin'):
    """Download firmware using actual size"""
    # Calculate actual firmware size
    fw_size = calculate_firmware_size()
    print(f"Detected firmware size: {fw_size} bytes ({fw_size/1024:.2f} KB)")
    
    if fw_size == 0:
        print("No firmware detected!")
        return None
    
    # Download only the actual firmware size
    return download_firmware(filename, fw_size)

# Get firmware size
size = calculate_firmware_size()
print(f"Firmware size: {size} bytes")

# Download with smart size detection
firmware = download_firmware_smart('firmware_backup.bin')
```

### Upload New Firmware

### Python Example (using smbus library)

```python
import smbus
import time

I2C_ADDR = 0x69
bus = smbus.SMBus(1)  # Use I2C bus 1

def enter_update_mode():
    bus.write_byte_data(I2C_ADDR, 0x90, 0x01)

def set_page(page_num):
    bus.write_byte_data(I2C_ADDR, 0x91, page_num & 0xFF)
    bus.write_byte_data(I2C_ADDR, 0x92, (page_num >> 8) & 0xFF)

def write_page_data(data):
    """Write 1024 bytes of data for current page"""
    for i, byte_val in enumerate(data):
        bus.write_byte_data(I2C_ADDR, 0x93, byte_val)
        time.sleep(0.001)  # Small delay

def erase_page():
    bus.write_byte_data(I2C_ADDR, 0x94, 0x00)

def commit_page():
    bus.write_byte_data(I2C_ADDR, 0x95, 0x00)

def verify_firmware():
    bus.write_byte_data(I2C_ADDR, 0x96, 0x00)
    # Read CRC32
    crc0 = bus.read_byte(I2C_ADDR, 0x13)
    crc1 = bus.read_byte(I2C_ADDR, 0x14)
    crc2 = bus.read_byte(I2C_ADDR, 0x15)
    crc3 = bus.read_byte(I2C_ADDR, 0x16)
    crc32 = crc0 | (crc1 << 8) | (crc2 << 16) | (crc3 << 24)
    return crc32

def reset_device():
    bus.write_byte_data(I2C_ADDR, 0x97, 0xAA)

# Update firmware
enter_update_mode()

# Read firmware binary file
with open('firmware.bin', 'rb') as f:
    firmware = f.read()

# Program each page
for page in range(0, 64):  # 64 pages for 64KB
    print(f"Programming page {page}...")
    set_page(page)
    
    # Get page data (1024 bytes)
    page_data = firmware[page*1024:(page+1)*1024]
    if len(page_data) < 1024:
        page_data += b'\xFF' * (1024 - len(page_data))
    
    # Write data
    write_page_data(page_data)
    
    # Erase and commit
    erase_page()
    time.sleep(0.1)
    commit_page()
    time.sleep(0.1)

# Verify
print("Verifying firmware...")
crc32 = verify_firmware()
print(f"CRC32: 0x{crc32:08X}")

# Reset
print("Resetting device...")
reset_device()
```

## Implementation Status

✅ **Implemented Features:**
- I2C command structure for firmware updates
- Flash page erase and write functions
- CRC32 calculation and verification
- Page-by-page update mechanism
- Update mode state management
- **Firmware download/backup functionality** - Read current firmware from flash
- **Firmware size detection** - Automatically detect actual firmware size by scanning flash

⚠️ **Important Notes:**

1. **Flash Programming**: The implementation uses HAL Flash API which requires:
   - Flash to be unlocked before erase/write
   - Half-word (16-bit) programming for STM32F0
   - Proper flash latency configuration

2. **Safety Considerations**:
   - Current implementation does NOT protect bootloader area
   - No rollback mechanism if update fails
   - Power loss during update can brick device
   - Consider implementing a small bootloader

3. **Memory Layout**:
   - Application starts at 0x08000000
   - If you add a bootloader, adjust `APP_START_ADDR`
   - Bootloader should be in first few KB (e.g., 0x08000000-0x08001000)

## Recommendations

1. **Add Bootloader**: Implement a small bootloader that can update the main application
2. **Watchdog**: Use watchdog timer to detect failed updates
3. **Backup**: Keep previous firmware version for rollback
4. **Progress Indicator**: Use LED patterns to show update progress
5. **Timeout**: Add timeout mechanism to exit update mode
6. **Bootloader Protection**: Add address range check to prevent overwriting bootloader
7. **Dual Bank**: Consider using dual-bank flash (if available) for safer updates

