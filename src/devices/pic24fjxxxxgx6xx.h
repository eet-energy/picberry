/*
 * Raspberry Pi PIC Programmer using GPIO connector
 * https://github.com/WallaceIT/picberry
 * Copyright 2014 Francesco Valla
 * Copyright 2016 Enric Balletbo i Serra
 * Copyright 2020 Markus Mueller
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

class pic24fjxxxxgx6xx : public Pic {

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
		pic_device piclist[16] = {{0x6000, "PIC24FJ128GA606", 0x15FFE},
					  	{0x6008, "PIC24FJ256GA606", 0x02AFFE},
					  	{0x6010, "PIC24FJ512GA606", 0x055FFE},
					  	{0x6018, "PIC24FJ1024GA606", 0x0ABFFE},
					  	{0x6001, "PIC24FJ128GA610", 0x15FFE},
					  	{0x6009, "PIC24FJ256GA610", 0x02AFFE},
					  	{0x6011, "PIC24FJ512GA610", 0x055FFE},
					  	{0x6019, "PIC24FJ1024GA610", 0x0ABEFE},
					  	{0x6004, "PIC24FJ128GB606", 0x15FFE},
					  	{0x600C, "PIC24FJ256GB606", 0x02AFFE},
					  	{0x6014, "PIC24FJ512GB606", 0x055FFE},
					  	{0x601C, "PIC24FJ1024GB606", 0x0ABFFE},
					  	{0x6005, "PIC24FJ128GB610", 0x15FFE},
					  	{0x600D, "PIC24FJ256GB610", 0x02AFFE},
					  	{0x6015, "PIC24FJ512GB610", 0x055FFE},
					  	{0x601D, "PIC24FJ1024GB610", 0x0ABEFE}};

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
