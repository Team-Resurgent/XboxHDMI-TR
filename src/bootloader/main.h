// Copyright 2021, Ryan Wendland, XboxHDMI by Ryzee119
// SPDX-License-Identifier: MIT

#pragma once

#include "../shared/error_handler.h"

#include <stdbool.h>

static bool can_launch_application(void);
static void jump_to_application(void);
static void enter_bootloader_mode(void);

static void setup_recovery_gpio();
static bool recovery_jumper_enabled();
