#pragma once

void _Error_Handler(char *file, int line);
#define Error_Handler() _Error_Handler(__FILE__, __LINE__)
