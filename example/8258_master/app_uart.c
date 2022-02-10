/********************************************************************************************************
 * @file     app_uart.c 
 *
 * @brief    for TLSR chips
 *
 * @author	 public@telink-semi.com;
 * @date     May. 12, 2018
 *
 * @par      Copyright (c) Telink Semiconductor (Shanghai) Co., Ltd.
 *           All rights reserved.
 *           
 *			 The information contained herein is confidential and proprietary property of Telink 
 * 		     Semiconductor (Shanghai) Co., Ltd. and is available under the terms 
 *			 of Commercial License Agreement between Telink Semiconductor (Shanghai) 
 *			 Co., Ltd. and the licensee in separate contract or the terms described here-in. 
 *           This heading MUST NOT be removed from this file.
 *
 * 			 Licensees are granted free, non-transferable use of the information in this 
 *			 file under Mutual Non-Disclosure Agreement. NO WARRENTY of ANY KIND is provided. 
 *           
 *******************************************************************************************************/
#include <stdarg.h>
#include "tl_common.h"
#include "drivers.h"

#include "app_uart.h"


#if (UART_PRINT_ENABLE)

volatile unsigned char uart_rx_flag=0;
volatile unsigned char uart_dmairq_tx_cnt=0;
volatile unsigned char uart_dmairq_rx_cnt=0;
volatile unsigned int  uart_ndmairq_cnt=0;
volatile unsigned char uart_ndmairq_index=0;


typedef struct{
	unsigned int dma_len;        // dma len must be 4 byte
	unsigned char data[UART_DATA_LEN];
}uart_data_t;

uart_data_t rec_buff = {0,  {0, } };
uart_data_t trans_buff = {0, {0,} };

void uart_init_user(int baud)
{
	//note: dma addr must be set first before any other uart initialization! (confirmed by sihui)
	uart_recbuff_init( (unsigned char *)&rec_buff, sizeof(rec_buff));

	GPIO_PinTypeDef UART_RX_PIN = 0;

	gpio_set_func(UART_RX_PA0, AS_GPIO);//TB-02/TB-03F/TB-04
	gpio_set_func(UART_RX_PB0, AS_GPIO);//TB-01
	gpio_set_func(UART_RX_PB7, AS_GPIO);//TB-02 Kit

	gpio_set_output_en(UART_RX_PA0, 0);
	gpio_set_output_en(UART_RX_PB0, 0);
	gpio_set_output_en(UART_RX_PB7, 0);

	gpio_set_input_en(UART_RX_PA0, 1); 
	gpio_set_input_en(UART_RX_PB0, 1); 
	gpio_set_input_en(UART_RX_PB7, 1); 

	gpio_setup_up_down_resistor(UART_RX_PA0, PM_PIN_PULLDOWN_100K);
	gpio_setup_up_down_resistor(UART_RX_PB0, PM_PIN_PULLDOWN_100K);
	gpio_setup_up_down_resistor(UART_RX_PB7, PM_PIN_PULLDOWN_100K);

	while(1)//检测UART Rx 引脚
	{
		if(gpio_read(UART_RX_PA0) != 0)
		{	
			UART_RX_PIN = UART_RX_PA0;
			break;
		}
		else if (gpio_read(UART_RX_PB0) != 0)
		{
			UART_RX_PIN = UART_RX_PB0;
			break;
		}
		else if (gpio_read(UART_RX_PB7) != 0)
		{
			UART_RX_PIN = UART_RX_PB7;
			break;
		}
	}

	gpio_setup_up_down_resistor(UART_RX_PA0, PM_PIN_UP_DOWN_FLOAT);
	gpio_setup_up_down_resistor(UART_RX_PB0, PM_PIN_UP_DOWN_FLOAT);
	gpio_setup_up_down_resistor(UART_RX_PB7, PM_PIN_UP_DOWN_FLOAT);

	uart_gpio_set(UART_TX_PB1, UART_RX_PIN);

	uart_reset();  //will reset uart digital registers from 0x90 ~ 0x9f, so uart setting must set after this reset

	//baud rate: 115200
	#if (CLOCK_SYS_CLOCK_HZ == 16000000)
//		uart_init(118, 13, PARITY_NONE, STOP_BIT_ONE);
		uart_init(9, 13, PARITY_NONE, STOP_BIT_ONE);
	#elif (CLOCK_SYS_CLOCK_HZ == 24000000)
		if(baud == 115200)
		{
			uart_init(12, 15, PARITY_NONE, STOP_BIT_ONE); //baudrate = 115200
		}
		else
		{
			uart_init(1, 12, PARITY_NONE, STOP_BIT_ONE); //baudrate = 921600
		}
	#elif(CLOCK_SYS_CLOCK_HZ == 32000000)
		uart_init(19, 13, PARITY_NONE, STOP_BIT_ONE);
	#endif

	uart_dma_enable(1, 1); 	//uart data in hardware buffer moved by dma, so we need enable them first

	irq_set_mask(FLD_IRQ_DMA_EN);
	dma_chn_irq_enable(FLD_DMA_CHN_UART_RX | FLD_DMA_CHN_UART_TX, 1);   	//uart Rx/Tx dma irq enable

	uart_irq_enable(0, 0);  	//uart Rx/Tx irq no need, disable them

	//irq_enable();
}

void uart_print_user(char * str)
{
	while(uart_tx_is_busy());
	trans_buff.dma_len = 0;

	while(*str)
	{
		trans_buff.data[trans_buff.dma_len] = *str++;
		trans_buff.dma_len += 1;
	}

	uart_dma_send((unsigned char*)&trans_buff);
}

void uart_send_user(char * data, u32 len)
{
	while(len > UART_DATA_LEN)
	{
		memcpy(trans_buff.data, data,  UART_DATA_LEN);
		data += UART_DATA_LEN;
		len -= UART_DATA_LEN;

		trans_buff.dma_len = UART_DATA_LEN;

		uart_dma_send((unsigned char*)&trans_buff);
		trans_buff.dma_len = 0;
		WaitMs(2);
		
	}

	if(len > 0)
	{
		memcpy(trans_buff.data, data,  len);
		trans_buff.dma_len = len;
		uart_dma_send((unsigned char*)&trans_buff);
		trans_buff.dma_len = 0;
		WaitMs(2);
	}
}


int putchar_user(int c)
{
	while(uart_tx_is_busy());
	trans_buff.data[0] = c;
	trans_buff.dma_len = 1;
	uart_dma_send((unsigned char*)&trans_buff);
	return c;
}


static void printchar_user(char **str, int c) {
	if (str) {
		**str = c;
		++(*str);
	} else
		(void) putchar_user(c);
}

#define PAD_RIGHT 1
#define PAD_ZERO 2

static int prints_user(char **out, const char *string, int width, int pad) {
	register int pc = 0, padchar = ' ';

	if (width > 0) {
		register int len = 0;
		register const char *ptr;
		for (ptr = string; *ptr; ++ptr)
			++len;
		if (len >= width)
			width = 0;
		else
			width -= len;
		if (pad & PAD_ZERO)
			padchar = '0';
	}
	if (!(pad & PAD_RIGHT)) {
		for (; width > 0; --width) {
			printchar_user(out, padchar);
			++pc;
		}
	}
	for (; *string; ++string) {
		printchar_user(out, *string);
		++pc;
	}
	for (; width > 0; --width) {
		printchar_user(out, padchar);
		++pc;
	}

	return pc;
}

/* the following should be enough for 32 bit int */
#define PRINT_BUF_LEN 12

static int printi_user(char **out, int i, int b, int sg, int width, int pad,
		int letbase) {
	char print_buf[PRINT_BUF_LEN];
	register char *s;
	register int t, neg = 0, pc = 0;
	register unsigned int u = i;

	if (i == 0) {
		print_buf[0] = '0';
		print_buf[1] = '\0';
		return prints_user(out, print_buf, width, pad);
	}

	if (sg && b == 10 && i < 0) {
		neg = 1;
		u = -i;
	}

	s = print_buf + PRINT_BUF_LEN - 1;
	*s = '\0';

	while (u) {
		t = u % b;
		if (t >= 10)
			t += letbase - '0' - 10;
		*--s = t + '0';
		u /= b;
	}

	if (neg) {
		if (width && (pad & PAD_ZERO)) {
			printchar_user(out, '-');
			++pc;
			--width;
		} else {
			*--s = '-';
		}
	}

	return pc + prints_user(out, s, width, pad);
}

static int print_user(char **out, const char *format, va_list args) {
	register int width, pad;
	register int pc = 0;
	char scr[2];

	for (; *format != 0; ++format) {
		if (*format == '%') {
			++format;
			width = pad = 0;
			if (*format == '\0')
				break;
			if (*format == '%')
				goto out;
			if (*format == '-') {
				++format;
				pad = PAD_RIGHT;
			}
			while (*format == '0') {
				++format;
				pad |= PAD_ZERO;
			}
			for (; *format >= '0' && *format <= '9'; ++format) {
				width *= 10;
				width += *format - '0';
			}
			if (*format == 's') {
				register char *s = (char *) va_arg( args, int );
				pc += prints_user(out, s ? s : "(null)", width, pad);
				continue;
			}
			if (*format == 'd') {
				pc += printi_user(out, va_arg( args, int ), 10, 1, width, pad, 'a');
				continue;
			}
			if (*format == 'x') {
				pc += printi_user(out, va_arg( args, int ), 16, 0, width, pad, 'a');
				continue;
			}
			if (*format == 'X') {
				pc += printi_user(out, va_arg( args, int ), 16, 0, width, pad, 'A');
				continue;
			}
			if (*format == 'u') {
				pc += printi_user(out, va_arg( args, int ), 10, 0, width, pad, 'a');
				continue;
			}
			if (*format == 'c') {
				/* char are converted to int then pushed on the stack */
				scr[0] = (char) va_arg( args, int );
				scr[1] = '\0';
				pc += prints_user(out, scr, width, pad);
				continue;
			}
		} else {
			out: printchar_user(out, *format);
			++pc;
		}
	}
	if (out)
		**out = '\0';
//	va_end( args );
	return pc;
}

int u_printf_user(const char *format, ...) {
	int ret;
	va_list args;

	va_start( args, format );
	ret = print_user(0, format, args);
	va_end( args );

	return ret;
}

int u_sprintf_user(char *out, const char *format, ...) {
	int ret=0;
	va_list args;

	va_start( args, format );
	print_user(&out, format, args);
	va_end( args );

	return ret;
}

void u_array_printf_user(unsigned char*data, unsigned int len) {
	u_printf("{");
	for(int i = 0; i < len; ++i){
		u_printf("%X%s", data[i], i<(len)-1? ":":" ");
	}
	u_printf("}\n");
}

#endif