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

class pic24fxxklxxx : public Pic {

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
		void send_cmd(uint32_t cmd);
		uint16_t read_data(void);

		/*
		 *                         ID       NAME             MEMSIZE
		 */
		pic_device piclist[10] = {{0x4B14, "PIC24F16KL402", 0x2BFE},
						{0x4B1E, "PIC24F16KL401", 0x2BFE},
					  	{0x4B04, "PIC24F08KL402", 0x15FE},
						{0x4B0E, "PIC24F08KL401", 0x15FE},
					  	{0x4B00, "PIC24F08KL302", 0x15FE},
						{0x4B0A, "PIC24F08KL301", 0x15FE},
					  	{0x4B06, "PIC24F08KL201", 0x15FE},
						{0x4B05, "PIC24F08KL200", 0x15FE},
						{0x4B02, "PIC24F04KL101", 0x0AFE},
					  	{0x4B01, "PIC24F04KL100", 0x0AFE}};
};
