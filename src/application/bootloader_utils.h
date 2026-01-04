// Copyright 2021, Ryan Wendland, XboxHDMI by Ryzee119
// SPDX-License-Identifier: MIT

#pragma once

#include <stdint.h>

// Function to request bootloader mode from application
// Call this function and then reset to enter bootloader
void bootloader_request(void);

// Check if bootloader is active (for application code)
uint8_t bootloader_is_active(void);
