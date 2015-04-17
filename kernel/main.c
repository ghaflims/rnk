/*
 * Copyright (C) 2014  Raphaël Poggi <poggi.raph@gmail.com>
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <board.h>
#include <usart.h>
#include <stdio.h>
#include <scheduler.h>
#include <task.h>
#include <interrupt.h>
#include <pio.h>
#include <utils.h>
#include <mm.h>
#include <mutex.h>
#include <semaphore.h>
#include <queue.h>
#include <arch/svc.h>
#include <time.h>
#include <spi.h>
#include <dma.h>
#include <common.h>

#ifdef STM32_F429
#include <ili9341.h>
#include <ltdc.h>

struct ltdc ltdc;
#endif /* STM32_F429 */


struct usart usart;
struct spi spi;
struct dma dma;
struct dma_transfer dma_trans;

static int count = 0;

void first_task(void)
{

	printk("starting task A\r\n");
	while (1) {
		mutex_lock(&mutex);
		printk("A");
		mutex_unlock(&mutex);
	}
}

void second_task(void)
{
	printk("starting task B\r\n");
	mutex_lock(&mutex);
	while (1) {
		printk("B");
		if (count++ == 500)
			mutex_unlock(&mutex);
	}
}

void third_task(void)
{
	printk("starting task C\r\n");
	sem_wait(&sem);
	count = 0;
	while (1) {
		printk("C");
		if (count++ == 4000)
			sem_post(&sem);
	}
}

void fourth_task(void)
{
	unsigned int size = sizeof(unsigned int);
	unsigned char *p;
	unsigned int *array = (unsigned int *)kmalloc(size);
	int i = 0;

	printk("starting task D\r\n");
	printk("array (%x)\r\n", array);

	p = (unsigned char *)kmalloc(24);
	*p = 0xea;
	printk("p(%x): %x\r\n", p, *p);
	kfree(p);

	while (1) {
		for (i = 0; i < size; i++) {
			printk("D");
			array[i] = (1 << i);
			printk("%x ", array[i]);
		}

		for (i = 0; i < size; i++) {
			kfree(array);
		}
	}
}

void fifth_task(void)
{
	printk("starting task E\r\n");
	while (1) {
		sem_wait(&sem);
		printk("E");
		sem_post(&sem);
	}
}

void sixth_task(void)
{
	printk("starting task F\r\n");
	pio_set_output(GPIOE_BASE, 6, 0);
	while (1) {
		pio_set_value(GPIOE_BASE, 6);
		usleep(500);
		printk("F");
		pio_clear_value(GPIOE_BASE, 6);
	}
}

void seventh_task(void)
{
	int a = 5;
	count = 0;
	printk("starting task H\r\n");
	printk("#####a(%x): %d\r\n", &a , a);
	while (1) {
		printk("H");
		if (count++ == 4000)
			queue_post(&queue, &a, 0);
	}
}


void eighth_task(void)
{
	int b = 0;
	printk("starting task G\r\n");
	queue_receive(&queue, &b, 10000);
	printk("#####b(%x): %d\r\n", &b, b);
	while (1) {
		printk("G");
	}

}

#ifdef STM32_F429
static void ltdc_init(void)
{
	lcd_init_gpio();

	ltdc.hsync = 16;
	ltdc.vsync = 2;
	ltdc.hbp = 40;
	ltdc.hfp = 10;
	ltdc.vbp = 2;
	ltdc.vfp = 4;
	ltdc.width = 240;
	ltdc.height = 320;
	ltdc.bpp = 2;
	ltdc.fb_addr = 0xD0000000;

	lcd_init(&ltdc);
}

#define BLACK    0x0000
#define BLUE     0x001F
#define RED      0xF800
#define GREEN    0x07E0
#define CYAN     0x07FF
#define MAGENTA  0xF81F
#define YELLOW   0xFFE0
#define WHITE    0xFFFF

#define MAX_DMA_SIZE	0xFFFF

unsigned short color = 0;

void lcd_rgb565_fill(unsigned short rgb)
{
	int size = ltdc.width * ltdc.height * ltdc.bpp;
	int i = 0;
	int num = 0;
	int remain = 0;
	unsigned int p = ltdc.fb_addr;

	debug_printk("lcd_rgb565_fill\r\n");

	color = rgb;

	dma_trans.src_addr = &color;

	dma_enable(&dma);

	num = size / MAX_DMA_SIZE;
	remain = size % MAX_DMA_SIZE;

	debug_printk("transfer: ");

	for (i = 0; i < num; i++) {
		sem_wait(&sem);
		debug_printk("%d ", i);
		dma_trans.dest_addr = p;
		dma_trans.size = MAX_DMA_SIZE;
		dma_transfer(&dma, &dma_trans);
		p += MAX_DMA_SIZE;
	}

	dma_trans.dest_addr = p;
	dma_trans.size = remain;
}
#endif /* STM32_F429 */


void ninth_task(void)
{
	printk("starting task I\r\n");

#ifdef STM32_F429
	ili9341_init();
	ili9341_init_lcd();
	ltdc_init();
#endif /* STM32_F429 */

	while (1) {
		printk("I");
		lcd_rgb565_fill(BLACK);
		usleep(1000);
		lcd_rgb565_fill(BLUE);
		usleep(1000);
		lcd_rgb565_fill(RED);
		usleep(1000);
		lcd_rgb565_fill(GREEN);
		usleep(1000);
		lcd_rgb565_fill(CYAN);
		usleep(1000);
		lcd_rgb565_fill(MAGENTA);
		usleep(1000);
		lcd_rgb565_fill(YELLOW);
		usleep(1000);
		lcd_rgb565_fill(WHITE);
	}
}

int main(void)
{

#ifdef STM32_F429
	usart.num = 1;
	usart.base_reg = USART1_BASE;
	usart.baud_rate = 115200;

	usart_init(&usart);

	pio_set_alternate(GPIOA_BASE, 9, 0x7);
	pio_set_alternate(GPIOA_BASE, 10, 0x7);
#else
	usart.num = 3;
	usart.base_reg = USART3_BASE;
	usart.baud_rate = 115200;

	usart_init(&usart);

	pio_set_alternate(GPIOC_BASE, 10, 0x7);
	pio_set_alternate(GPIOC_BASE, 11, 0x7);
#endif /* STM32_F429 */

	dma.num = 2;
	dma.stream_base = DMA2_Stream0_BASE;
	dma.stream_num = 0;
	dma.channel = 0;
	dma.dir = DMA_M_M;
	dma.mdata_size = DATA_SIZE_HALF_WORD;
	dma.pdata_size = DATA_SIZE_HALF_WORD;
	dma.mburst = INCR0;
	dma.pburst = INCR0;
	dma.minc = 1;
	dma.pinc = 0;
	dma.use_fifo = 0;

	dma_init(&dma);

	/* User button */
	pio_set_input(GPIOA_BASE, 0, 0, 0);

	printk("Welcome to rnk\r\n");
	printk("- Initialise heap...\r\n");

	init_heap();

	printk("- Initialise scheduler...\r\n");
	schedule_init();

	init_mutex(&mutex);
	init_semaphore(&sem, 1);
	init_queue(&queue, sizeof(int), 5);
	time_init();

	printk("- Add task to scheduler\r\n");

//	add_task(&first_task, 1);
//	add_task(&second_task, 6);
//	add_task(&third_task, 2);
//	add_task(&fourth_task, 20);
//	add_task(&fifth_task, 1);
//	add_task(&sixth_task, 1);
//	add_task(&seventh_task, 1);
//	add_task(&eighth_task, 1);
	add_task(&ninth_task, 1);

	printk("- Start scheduling...\r\n");
	start_schedule();

	while(1)
		;

	return 0; //Never reach
}
