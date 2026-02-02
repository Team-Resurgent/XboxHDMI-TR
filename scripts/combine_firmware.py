Import("env")

"""
Combined script for combined_stm32f0 environment.
Builds bootloader and application, then combines them into a single binary.
Appends a CRC32 footer to the app image so the bootloader can verify integrity before booting.
"""

print("DEBUG: combine_firmware.py script loaded")

# Bootloader size in bytes (20KB)
BOOTLOADER_SIZE = 20 * 1024
# App region size in bytes (44KB); last 4 bytes hold CRC32 of the preceding app image
APP_SIZE_BYTES = 44 * 1024  # FLASH_TOTAL_SIZE - BOOTLOADER_SIZE
APP_IMAGE_SIZE = APP_SIZE_BYTES - 4  # app binary + padding, before CRC footer
PADDING_VALUE = 0xFF  # Typical flash erase value


def crc32_app_footer(data):
    """CRC32 matching shared/crc32.c: init 0xFFFFFFFF, final xor 0xFFFFFFFF."""
    import binascii
    crc = binascii.crc32(data, 0xFFFFFFFF) & 0xFFFFFFFF
    return (crc ^ 0xFFFFFFFF) & 0xFFFFFFFF

def build_and_combine(target, source, env):
    """Build bootloader and application, then combine them."""
    import subprocess
    import sys
    import shutil
    from pathlib import Path
    
    # Only run if we're building the combined environment
    env_name = env.get("PIOENV", "")
    print(f"DEBUG: build_and_combine called, PIOENV: {env_name}")
    if env_name != "combined_stm32f0":
        print(f"DEBUG: Skipping - PIOENV is '{env_name}', not 'combined_stm32f0'")
        return
    
    # This function is called as a SCons action, so target/source might be SCons nodes
    # We can ignore them and just do our work
    
    project_dir = env["PROJECT_DIR"]
    
    print("=" * 60)
    print("Building bootloader and application, then combining...")
    print("=" * 60)
    
    # Build bootloader and application
    try:
        pio_cmd = shutil.which("platformio") or shutil.which("pio")
        if not pio_cmd:
            pio_base = [sys.executable, "-m", "platformio"]
        else:
            pio_base = [pio_cmd]
        
        # Build bootloader first
        print("\n[1/3] Building bootloader (bootloader_stm32f0)...")
        result = subprocess.run(
            pio_base + ["run", "-e", "bootloader_stm32f0"],
            check=True,
            cwd=project_dir
        )
        print("✓ Bootloader built successfully!")
        
        # Build application
        print("\n[2/3] Building application (application_stm32f0)...")
        result = subprocess.run(
            pio_base + ["run", "-e", "application_stm32f0"],
            check=True,
            cwd=project_dir
        )
        print("✓ Application built successfully!")
    except subprocess.CalledProcessError as e:
        print(f"✗ Error: Failed to build (return code: {e.returncode})")
        sys.exit(1)
    except Exception as e:
        print(f"✗ Error building: {e}")
        sys.exit(1)
    
    # Now combine the binaries
    print("\n[3/3] Combining binaries...")
    build_dir = Path(".pio/build")
    
    bootloader_bin = build_dir / "bootloader_stm32f0" / "firmware.bin"
    application_bin = build_dir / "application_stm32f0" / "firmware.bin"
    
    # Output to combined_stm32f0 folder
    output_dir = build_dir / "combined_stm32f0"
    output_bin = output_dir / "firmware.bin"
    
    # Delete contents of combined_stm32f0 folder before writing
    if output_dir.exists():
        print(f"Cleaning output directory: {output_dir}")
        for item in output_dir.iterdir():
            if item.is_file():
                item.unlink()
            elif item.is_dir():
                shutil.rmtree(item)
    else:
        output_dir.mkdir(parents=True, exist_ok=True)
    
    # Check if bootloader binary exists
    if not bootloader_bin.exists():
        print(f"✗ Error: Bootloader binary not found: {bootloader_bin}")
        sys.exit(1)
    
    # Check if application binary exists
    if not application_bin.exists():
        print(f"✗ Error: Application binary not found: {application_bin}")
        sys.exit(1)
    
    # Read bootloader binary
    print(f"Reading bootloader: {bootloader_bin}")
    with open(bootloader_bin, 'rb') as f:
        bootloader_data = f.read()
    
    bootloader_size = len(bootloader_data)
    print(f"Bootloader size: {bootloader_size} bytes")
    
    # Check if bootloader exceeds 16KB
    if bootloader_size > BOOTLOADER_SIZE:
        print(f"✗ Error: Bootloader size ({bootloader_size} bytes) exceeds {BOOTLOADER_SIZE} bytes!")
        sys.exit(1)
    
    # Pad bootloader to 16KB
    padding_size = BOOTLOADER_SIZE - bootloader_size
    if padding_size > 0:
        print(f"Padding bootloader with {padding_size} bytes (0xFF)")
        bootloader_data += bytes([PADDING_VALUE] * padding_size)
    
    # Read application binary
    print(f"Reading application: {application_bin}")
    with open(application_bin, 'rb') as f:
        application_data = f.read()
    
    application_size = len(application_data)
    print(f"Application size: {application_size} bytes")
    
    if application_size > APP_IMAGE_SIZE:
        print(f"✗ Error: Application size ({application_size} bytes) exceeds app image size ({APP_IMAGE_SIZE} bytes)!")
        sys.exit(1)
    
    # Pad application to APP_IMAGE_SIZE (excludes 4-byte CRC footer), then append CRC32
    padding_size = APP_IMAGE_SIZE - application_size
    if padding_size > 0:
        application_data += bytes([PADDING_VALUE] * padding_size)
    app_crc = crc32_app_footer(application_data)
    application_data += app_crc.to_bytes(4, 'little')
    print(f"App image + CRC footer: {len(application_data)} bytes (CRC32=0x{app_crc:08X})")
    
    # Combine binaries
    print(f"Combining binaries...")
    combined_data = bootloader_data + application_data
    
    # Write combined binary
    print(f"Writing combined binary: {output_bin}")
    with open(output_bin, 'wb') as f:
        f.write(combined_data)
    
    total_size = len(combined_data)
    print(f"\n✓ Combined binary created successfully!")
    print(f"  Output: {output_bin}")
    print(f"  Total size: {total_size} bytes ({total_size / 1024:.2f} KB)")
    print("=" * 60)

# Create a custom target that always runs, since we're not compiling anything
env_name = env.get("PIOENV", "")
if env_name == "combined_stm32f0":
    print("DEBUG: Creating custom target for combined environment")
    from SCons.Script import AlwaysBuild, Default
    
    # Create the target that does the actual work
    combine_target = env.Command(
        env["BUILD_DIR"] + "/combined_firmware",
        [],
        build_and_combine
    )
    AlwaysBuild(combine_target)
    Default(combine_target)
    env.Alias("combined", combine_target)
