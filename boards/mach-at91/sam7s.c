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
#include <utils.h>
#include <scheduler.h>
#include "aic.h"

void default_spurious_handler(void)
{
	writel(AT91C_BASE_AIC + AIC_EOICR, 0x0);
}

void default_irq_handler(void)
{
	while (1)
		;
}

void default_fiq_handler(void)
{
	while (1)
		;
}

#define BOARD_FLASH	0
#define BOARD_RAM	1

static unsigned char get_remap( void )
{
    unsigned int *remap = (unsigned int *)0;
    unsigned int *ram = (unsigned int *)AT91C_ISRAM;

    // Try to write in 0 and see if this affects the RAM
    unsigned int temp = *ram;

    *ram = temp + 1;
    if (*remap == *ram) {

        *ram = temp;
        return BOARD_RAM;
    } else {

        *ram = temp;
        return BOARD_FLASH;
    }
}

void low_level_init(void)
{
	unsigned int tmp = 0;
	unsigned int i = 0;

	/* Initialise the main oscillator */
	writel(AT91C_BASE_CKGR + CKGR_MOR, BOARD_OSCOUNT | AT91C_CKGR_MOSCEN);
	while (!(readl(AT91C_BASE_PMC + PMC_SR) & AT91C_PMC_MOSCS))
		;

	/* Initliase the PLL at 96MHz and USB clock to 48MHz */
	writel(AT91C_BASE_CKGR + CKGR_PLLR, BOARD_USBDIV | BOARD_CKGR_PLL | BOARD_PLLCOUNT | BOARD_MUL | BOARD_DIV);
	while (!(readl(AT91C_BASE_PMC + PMC_SR) & AT91C_PMC_LOCK))
		;

	/* Wait for the master clock if it was already initialised */
	while (!(readl(AT91C_BASE_PMC + PMC_SR) & AT91C_PMC_MCKRDY))
		;

	/* Switch to slow clock + prescaler */
	writel(AT91C_BASE_PMC + PMC_MCKR, BOARD_PRESCALER);
	while (!(readl(AT91C_BASE_PMC + PMC_SR) & AT91C_PMC_MCKRDY))
		;

	/* Switch to fast clock + prescaler */
	tmp = readl(AT91C_BASE_PMC + PMC_MCKR);
	writel(AT91C_BASE_PMC + PMC_MCKR, tmp | AT91C_PMC_CSS_PLL_CLK);
	while (!(readl(AT91C_BASE_PMC + PMC_SR) & AT91C_PMC_MCKRDY))
		;

	/* Disable all interrupts */
	writel(AT91C_BASE_AIC + AIC_IDCR, 0xFFFFFFFF);

	/* Clear pending interrupts */
	writel(AT91C_BASE_AIC + AIC_ICCR, 0xFFFFFFFF);

	/* Initialise AIC */
	aic_register_handler(0, AT91C_AIC_PRIOR_LOWEST, default_fiq_handler);
	for (i = 2; i < 31; i++) {
		aic_register_handler(i, AT91C_AIC_PRIOR_LOWEST, default_irq_handler);
	}
	writel(AT91C_BASE_AIC + AIC_SPU, (unsigned int)default_spurious_handler);

	/* Unstack nested interrupts */
	for (i = 0; i < 8; i++) {
		writel(AT91C_BASE_AIC + AIC_EOICR, 0x0);
	}

	aic_disable_it(AT91C_ID_SYS);
	aic_register_handler(AT91C_ID_SYS, AT91C_AIC_PRIOR_LOWEST, schedule);
	aic_enable_it(AT91C_ID_SYS);

	/* Enable Debug mode */
	aic_enable_debug();

	/* Disable watchdog */
	writel(AT91C_BASE_WDTC + WDTC_WDMR, AT91C_WDTC_WDDIS);

	/* Enable PIOA clock */
	writel(AT91C_BASE_PMC + PMC_PCER, (1 << AT91C_ID_PIOA));

	/* Clear all pending interrupt */
	readl(AT91C_BASE_PIOA + PIO_ISR);

	/* Remap RAM to 0x0 */
	if (get_remap() != BOARD_RAM)
		writel(AT91C_BASE_MC + MC_RCR, AT91C_MC_RCB);
}
