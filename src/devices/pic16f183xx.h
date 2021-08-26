/*
 * Raspberry Pi PIC Programmer using GPIO connector
 * https://github.com/WallaceIT/picberry
 * Copyright 2014 Francesco Valla
 * Copyright 2016 Enric Balletbo i Serra
 * Modified 2020 Markus Mueller
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>

#include "../common.h"
#include "device.h"

using namespace std;

class pic16f183xx : public Pic {

	public:
		void enter_program_mode(void);
		void exit_program_mode(void);
		bool setup_pe(void){return true;};
		bool read_device_id(void);
		void bulk_erase(void);
		void dump_configuration_registers(void);
		void read(char *outfile, uint32_t start, uint32_t count);
		void write(char *infile);
		uint8_t blank_check(void);

	protected:
		void send_cmd(uint8_t cmd, unsigned int delay);
		uint16_t read_data(void);
		void write_data(uint16_t data);
		void set_address(uint32_t addr);

		/*
		 *                         ID       NAME             MEMSIZE
		 */
		pic_device piclist[4] = {{0x30A4, "PIC16F18326", 0x003FFF},
					  	{0x30A6, "PIC16LF18326", 0x003FFF},
					  	{0x30A5, "PIC16F18346", 0x003FFF},
					  	{0x30A7, "PIC16LF18346", 0x003FFF}};

		uint32_t cword_address[9] = {0x0ABF00, 	// FSEC
									0x0ABF10, 	// FBSLIM
									0x0ABF14, 	// FSIGN
									0x0ABF18, 	// FOSCSEL
									0x0ABF1C, 	// FOSC
									0x0ABF20, 	// FWDT
									0x0ABF24, 	// FPOR
									0x0ABF28, 	// FICD
									0x0ABF2C};	// FDEVOPT1
};
