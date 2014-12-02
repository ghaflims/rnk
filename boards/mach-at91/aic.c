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

#include "aic.h"

void aic_register_handler(unsigned int source, unsigned int mode, void (*handler)(void))
{
	unsigned int source_shift = source * 0x4;

	/* Disable the interrupt */
	writel(AT91C_BASE_AIC + AIC_IDCR, (1 << source));

	/* Configure mode and handler */
	writel(AT91C_BASE_AIC + (AIC_SMR + source_shift), mode);
	writel(AT91C_BASE_AIC + (AIC_SVR + source_shift), (unsigned int)handler);

	/* Clear the interrupt */
	writel(AT91C_BASE_AIC + AIC_ICCR, (1 << source));
}

void aic_enable_it(unsigned int source)
{
	writel(AT91C_BASE_AIC + AIC_IECR, (1 << source));
}

void aic_disable_it(unsigned int source)
{
	writel(AT91C_BASE_AIC + AIC_IDCR, (1 << source));
}

void aic_enable_debug(void)
{
	writel(AT91C_BASE_AIC + AIC_DCR, AT91C_AIC_DCR_PROT);
}
