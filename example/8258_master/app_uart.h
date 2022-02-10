#pragma once

/* Enable C linkage for C++ Compilers: */
#if defined(__cplusplus)
extern "C" {
#endif

#include "app_config.h"

#ifndef UART_PRINT_ENABLE
#define UART_PRINT_ENABLE                    			0
#endif

#if (UART_PRINT_ENABLE)

#define UART_DATA_LEN    12+256    //data max ?    (UART_DATA_LEN+4) must 16 byte aligned 256 + 16 


void uart_init_user(int baud);
int putchar_user(int c);
void uart_print_user(char * str);
void uart_send_user(char * data, u32 len);
int  u_printf_user(const char *fmt, ...);
int  u_sprintf_user(char* s, const char *fmt, ...);
void u_array_printf_user(unsigned char*data, unsigned int len);

#define app_uart_init	uart_init_user
#define app_putchar_user	putchar_user
#define app_uart_print	uart_print_user
#define app_uart_send	uart_send_user
#define app_printf	 		u_printf_user
#define app_sprintf	 		u_sprintf_user
#define app_array_printf	u_array_printf_user
    
#else

#define app_uart_init(baud)
#define app_uart_print(str)
#define app_uart_send(data, len)

#define app_printf
#define app_sprintf
#define app_array_printf
	
#endif

/* Disable C linkage for C++ Compilers: */
#if defined(__cplusplus)
}
#endif