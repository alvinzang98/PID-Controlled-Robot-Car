#include "stm32f10x.h"
#include "stdio.h"
#include "stdlib.h"
#include "stdbool.h"
#include "string.h"

#ifndef __USART_H
#define __USART_H


#define USART_RXBUFF_SIZE 70//define

extern USART_TypeDef* printf_dest;
#define printf2(...) do{\
	printf_dest = USART2;\
	printf(__VA_ARGS__);\
}while(0)\

#define printf3(...) do{\
	printf_dest = USART3;\
	printf(__VA_ARGS__);\
}while(0)\

void USART2_WIFI_Init(void);           // A WIFI module connected to USART2




#endif


