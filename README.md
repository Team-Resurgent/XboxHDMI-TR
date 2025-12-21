# Based of Original Xbox 100% Digital HDMI (Prototype) by Ryzee119

This project aims to a open source alternate firmware for OXHD + MakeMHz's HD+

Based on...

https://github.com/Ryzee119/XboxHDMI-Ryzee119

Notes:

Ive started to refactor code to make stm32f0 (hd+) + stm32g0 (oxhd) both compilable from the same project, for now stm32f0 is confirmed working.

Ive now added support for Cerbios V3.1.0 which will greatly improve video mode detection.

More details to come soon...

EqUiNoX

In order to program HD+ connect the following wires...

| Pin Pair | Signal      | Signal      |
|----------|-------------|-------------|
| 2 – 1    | NC          | NRST        |
| 4 – 3    | NC          | NC          |
| 6 – 5    | GND         | NC          |
| 8 – 7    | SWDIO       | SWCLK       |

Note it is recommended you power from the xbox motherboard when programming and not use the 3V3 described below.

Full pinout for reference...

| Pin Pair | Signal      | Signal      |
|----------|-------------|-------------|
| 2 – 1    | 3V3         | NRST        |
| 4 – 3    | OSC OUT     | TX          |
| 6 – 5    | GND         | RX          |
| 8 – 7    | SWDIO       | SWCLK       |


To Do:
- Enhance functionality (on-going)
- Check all video modes render correctly (on-going)
- Add ability for stm to be programmed from xbox


EqUiNoX
