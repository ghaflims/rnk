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
#include <usart.h>

static void usart_register_functions( void )
{

        io_op.write = &usart_printl;        

} 

void usart_init( void )
{
	/* Disable UART */
	writel( UART0_CR , 0x00000000 );
	
	/*Disable pull up/down for all GPIO */
	writel( GPIO_GPPUD , 0x00000000 );
	delay( 150 );

	/* Disable pull up/down for GPIO 14 and 15 */
	writel( GPIO_GPPUDCLK0 , ( 1 << 14 ) | ( 1 << 15 ) );
	delay( 150 );

	/* Enable */
	writel( GPIO_GPPUDCLK0 , 0x00000000 );

	/* Clear pending interrupt */
	writel( UART0_ICR , 0x7F );

	/* Set divider */
	writel( UART0_IBRD , 1 );
	writel( UART0_FBRD , 40 );

	/* Enable FIFO & 8 bit data transmission */
	writel( UART0_LCRH , ( 1 << 4 ) | ( 1 << 5 ) | ( 1 << 6 ) );
	
	/* Mask all interrupts */
	writel( UART0_IMSC , ( 1 << 1 ) | ( 1 << 4 ) | ( 1 << 5 ) | ( 1 << 6 ) | ( 1 << 7 )
						| ( 1 << 8 ) | ( 1 << 9 ) | ( 1 << 10 ) );

	/* Enable UART0 */
	writel( UART0_CR , ( 1 << 0 ) | ( 1 << 8 ) | ( 1 << 9 ) );

	/* Register io functions */
    usart_register_functions();
}

void usart_print( unsigned char byte )
{
	while( 1 )
	{
		if( !( readl( UART0_FR ) & ( 1 << 5 ) ) )
			break;
	}
	
	writel( UART0_DR , byte );
}


int usart_printl( const char *string )
{
	while( *string )
	{
		usart_print( *string++ );
	}

	return 0;
}
