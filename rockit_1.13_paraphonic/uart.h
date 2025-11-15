/*
	This file is part of Rockit.

    Rockit is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Rockit is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rockit.  If not, see <http://www.gnu.org/licenses/>
*/

#ifndef UART_H
#define UART_H

void 
uart_init(void);

unsigned char
uart_get_byte(void);

void
uart_transmit_byte(unsigned char uc_byte);

unsigned char 
uart_tx_buffer_empty(void);

unsigned char
uart_rx_buffer_has_byte(void);



#endif //UART_H
