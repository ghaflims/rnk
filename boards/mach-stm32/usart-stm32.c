/*
 * Copyright (C) 2015  Raphaël Poggi <poggi.raph@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Frrestore * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <board.h>
#include <mach/rcc-stm32.h>
#include <errno.h>

/* Calculates the value for the USART_BRR */
static unsigned short stm32_baud_rate(long clock, unsigned int baud)
{
	unsigned int divisor = 16 * baud;
	unsigned short mantissa = clock / divisor;
	unsigned int remainder = clock % divisor;
	unsigned short fraction = (16 * remainder) / divisor;

	return (mantissa << 4) | (fraction & 0xf);
}

static int stm32_usart_init(struct usart *usart)
{
	int ret = 0;
	USART_TypeDef *USART = (USART_TypeDef *)usart->base_reg;

	ret = stm32_rcc_enable_clk(usart->base_reg);
	if (ret < 0) {
		error_printk("cannot enable clk for usart\r\n");
		return ret;
	}

	USART->CR1 &= ~USART_CR1_M;
	USART->CR1 &= ~USART_CR1_UE;

#ifdef CONFIG_STM32F429
	USART->BRR = stm32_baud_rate(APB2_CLK, usart->baud_rate);
#else
	USART->BRR = stm32_baud_rate(APB1_CLK, usart->baud_rate);
#endif /* CONFIG_STM32F429 */

	USART->CR1 |= USART_CR1_RE;
	USART->CR1 |= USART_CR1_TE;
	USART->CR1 |= USART_CR1_UE;

	return ret;
}

static void stm32_usart_print(struct usart *usart, unsigned char byte)
{
	USART_TypeDef *USART = (USART_TypeDef *)usart->base_reg;

	while(!(USART->SR & USART_SR_TXE))
		;

	USART->DR = byte;
}

static int stm32_usart_printl(struct usart *usart, const char *string)
{
	int size = 0;

	while (*string) {
		stm32_usart_print(usart, *string++);
		size++;
	}

	return size;
}

static int stm32_usart_write(struct usart *usart, unsigned char *buff, unsigned int len)
{
	USART_TypeDef *USART = (USART_TypeDef *)usart->base_reg;
	int i = 0;
	int ret = 0;
	int timeout = 1000;

	for (i = 0; i < len;  i++) {
		while(!(USART->SR & USART_SR_TXE) && timeout--)
			;

		if (!timeout) {
			ret = -EIO;
			break;
		}

		USART->DR = buff[i];
	}


	return ret;
}

static int stm32_usart_read(struct usart *usart, unsigned char *buff, unsigned int len)
{
	USART_TypeDef *USART = (USART_TypeDef *)usart->base_reg;
	int i = 0;
	int ret = 0;
	int timeout = 1000;

	for (i = 0; i < len;  i++) {
		while(!(USART->SR & USART_SR_RXNE) && timeout--)
			;

		if (!timeout) {
			ret = -EIO;
			break;
		}

		buff[i] = USART->DR;
	}


	return ret;
}

struct usart_operations usart_ops = {
	.init = stm32_usart_init,
	.read = stm32_usart_read,
	.write = stm32_usart_write,
	.print = stm32_usart_print,
	.printl = stm32_usart_printl,
};
