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

#include <pit.h>
#include <board.h>

void pit_init(unsigned int period, unsigned int pit_frequency)
{
	pit_ops.init(period, pit_frequency);
}

void pit_enable(void)
{
	pit_ops.enable();
}

void pit_enable_it(void)
{
	pit_ops.enable_it();
}

void pit_disable_it(void)
{
	pit_ops.disable_it();
}
