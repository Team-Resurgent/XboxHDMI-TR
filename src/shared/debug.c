#include "debug.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

// #define DEBUG_OUT

#ifdef DEBUG_OUT

static UART_HandleTypeDef huart2;

#define RING_BUFFER_SIZE 2048

typedef struct {
    char buffer[RING_BUFFER_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
} RingBuffer;

static RingBuffer debugBuffer = {0};

void debug_init() 
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // PA2 = TX, PA3 = RX
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF1_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    huart2.Instance = USART2;
    huart2.Init.BaudRate = 9600;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;

    HAL_UART_Init(&huart2);
}

void debug_log(const char *fmt, ...)
{
    char buffer[256]; 
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    HAL_UART_Transmit(&huart2, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
}

void debug_ring_log(const char *fmt, ...)
{
    char buffer[256]; 
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    for(int i = 0; i < len; i++)
    {
        uint16_t next = (debugBuffer.head + 1) % RING_BUFFER_SIZE;
        if(next == debugBuffer.tail) 
        {
            break;
        }
        debugBuffer.buffer[debugBuffer.head] = buffer[i];
        debugBuffer.head = next;
    }
}

void debug_ring_flush()
{
    while(debugBuffer.tail != debugBuffer.head)
    {
        char c = debugBuffer.buffer[debugBuffer.tail];
        debugBuffer.tail = (debugBuffer.tail + 1) % RING_BUFFER_SIZE;
        HAL_UART_Transmit(&huart2, (uint8_t*)&c, 1, HAL_MAX_DELAY);
    }
}

#else
void debug_init() 
{
}

void debug_log(const char *fmt, ...)
{
    (void) fmt;
}

void debug_ring_log(const char *fmt, ...)
{
    (void) fmt;
}

void debug_ring_flush()
{
}
#endif
