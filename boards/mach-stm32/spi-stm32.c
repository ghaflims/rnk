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
#include <utils.h>
#include <stdio.h>
#include <mach/dma-stm32.h>
#include <mach/rcc-stm32.h>
#include <arch/nvic.h>
#include <errno.h>
#include <queue.h>
#include <common.h>

static int stm32_spi_get_nvic_number(struct spi *spi)
{
	int nvic = 0;

	switch (spi->base_reg) {
		case SPI1_BASE:
			nvic = SPI1_IRQn;
			break;
		case SPI2_BASE:
			nvic = SPI2_IRQn;
			break;
		case SPI3_BASE:
			nvic = SPI3_IRQn;
			break;
#ifdef CONFIG_STM32F429
		case SPI4_BASE:
			nvic = SPI4_IRQn;
			break;
		case SPI5_BASE:
			nvic = SPI5_IRQn;
			break;
#endif /* CONFIG_STM32F429 */
	}


	return nvic;
}

static short stm32_spi_find_best_pres(unsigned long parent_rate, unsigned long rate)
{
	unsigned int i;
	unsigned short pres[] = {2, 4, 8, 16, 32, 64, 128, 256};
	unsigned short best_pres;
	unsigned int diff;
	unsigned int best_diff;
	unsigned long curr_rate;


	best_diff = parent_rate - rate;

	for (i = 0; i < 8; i++) {
		curr_rate = parent_rate / pres[i];

		if (curr_rate < rate)
			diff = rate - curr_rate;
		else
			diff = curr_rate - rate;

		if (diff < best_diff) {
			best_pres = pres[i];
			best_diff = diff;
		}

		if (!best_diff || curr_rate < rate)
			break;
	}

	return best_pres;
}

static void stm32_spi_init_dma(struct spi *spi)
{
	struct dma *dma = &spi->dma;
	dma->num = 2;
	dma->stream_base = DMA2_Stream4_BASE;
	dma->stream_num = 4;
	dma->channel = 2;
	dma->dir = DMA_M_P;
	dma->mdata_size = DATA_SIZE_HALF_WORD;
	dma->pdata_size = DATA_SIZE_HALF_WORD;
	dma->mburst = INCR0;
	dma->pburst = INCR0;
	dma->minc = 0;
	dma->pinc = 0;
	dma->use_fifo = 0;

	stm32_dma_init(dma);
}

int stm32_spi_init(struct spi *spi)
{
	int ret = 0;
	SPI_TypeDef *SPI = (SPI_TypeDef *)spi->base_reg;

	ret = stm32_rcc_enable_clk(spi->base_reg);
	if (ret < 0) {
		error_printk("cannot enable SPI periph clock\r\n");
		return ret;
	}

	spi->rate = APB2_CLK;
	spi->rate = stm32_spi_find_best_pres(spi->rate, spi->speed);

	SPI->CR1 &= ~SPI_CR1_SPE;

	SPI->CR1 |= (0x2 << 3);

	/* Set master mode */
	SPI->CR1 |= SPI_CR1_MSTR;

	/* Handle slave selection via software */
	SPI->CR1 |= SPI_CR1_SSM;
	SPI->CR1 |= SPI_CR1_SSI;

	SPI->CR1 |= SPI_CR1_BIDIOE;

	SPI->CR2 |= SPI_CR2_TXEIE;
	SPI->CR2 |= SPI_CR2_ERRIE;

	if (spi->use_dma)
		stm32_spi_init_dma(spi);

	SPI->CR1 |= SPI_CR1_SPE;

	return ret;
}

static unsigned short stm32_spi_dma_write(struct spi *spi, unsigned short data)
{
	SPI_TypeDef *SPI = (SPI_TypeDef *)spi->base_reg;

	struct dma *dma = &spi->dma;
	struct dma_transfer *dma_trans = &spi->dma_trans;

	int nvic = stm32_spi_get_nvic_number(spi);
	int ready = 0;
	int ret = 0;

	stm32_dma_disable(dma);
	dma_trans->src_addr = (unsigned int)&data;
	dma_trans->dest_addr = (unsigned int)&SPI->DR;
	dma_trans->size = 1;

	nvic_enable_interrupt(nvic);

	SPI->CR2 &= ~SPI_CR2_TXDMAEN;

	queue_receive(&queue, &ready, 1000);

	stm32_dma_enable(dma);

	if (ready) {
		printk("spi ready !\r\n");

		stm32_dma_transfer(dma, dma_trans);

		if (spi->only_tx)
			SPI->CR2 |= SPI_CR2_TXDMAEN;

		ready = 0;
	} else {
		error_printk("spi not ready\r\n");
		ret = -EIO;
	}

	return ret;

}

int stm32_spi_write(struct spi *spi, unsigned char *buff, unsigned int size)
{
	SPI_TypeDef *SPI = (SPI_TypeDef *)spi->base_reg;
	int i;
	int ret;
	int n = 0;
	unsigned short data;

	for (i = 0; i < size; i += 2) {
		data = *(unsigned short *)(buff + i);

		if (spi->use_dma) {
			ret = stm32_spi_dma_write(spi, data);
			if (ret < 0)
				return ret;

			n++;
		}
		else {
			SPI->DR = data;

			while (!(SPI->SR & SPI_SR_TXE))
				;

			while (!(SPI->SR & SPI_SR_RXNE))
				;

			data = SPI->DR;

			n++;
		}
	}

	return n;
}

int stm32_spi_read(struct spi *spi, unsigned char *buff, unsigned int size)
{
	SPI_TypeDef *SPI = (SPI_TypeDef *)spi->base_reg;
	int i;
	int n = 0;

	for (i = 0; i < size; i += 2) {
		/* write dummy bytes */
		SPI->DR = 0xFF;

		while (!(SPI->SR & SPI_SR_TXE))
			;

		while (!(SPI->SR & SPI_SR_RXNE))
			;

		*(unsigned short *)(buff + i) = SPI->DR;

		n++;
	}

	return n;
}

struct spi_operations spi_ops = {
	.init = stm32_spi_init,
	.write = stm32_spi_write,
	.read = stm32_spi_read,
};
