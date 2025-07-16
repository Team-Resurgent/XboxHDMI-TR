# Based of Original Xbox 100% Digital HDMI (Prototype) by Ryzee119

This project aims to a open source alternate firmware for OXHD + MakeMHz's HD+

Based on...

https://github.com/Ryzee119/XboxHDMI-Ryzee119 

Notes: 

Ive started to refactor code to make stm32f0 (hd+) + stm32g0 (oxhd) both compilable from the same project, for now stm32f0 is confirmed working.

More details to come soon...

EqUiNoX

HD+ Program Header Pin Out

2 1 - 3V3       NRST

4 3 - OSC OUT   TX

6 5 - GND       RX

8 7 - SWDIO     SWCLK

To Do:

Finalize stm32g0 support

Enhance functionality (on-going)

EqUiNoX
