/*
 * Raspberry Pi PIC Programmer using GPIO connector
 * https://github.com/WallaceIT/picberry
 * Copyright 2014 Francesco Valla
 * Copyright 2017 Akimasa Tateba
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

#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <iostream>

#include "pic16f183xx.h"

/* delays (in microseconds) */
#define DELAY_SETUP	1
#define	DELAY_HOLD	1
#define DELAY_TENTS	1
#define DELAY_TENTH	250
#define DELAY_TCKH	1
#define DELAY_TCKL 	1
#define DELAY_TCO 	1
#define DELAY_TDLY	1
#define DELAY_TERAB	5000
#define DELAY_TEXIT	1
#define DELAY_TPINT_DATA	2500
#define DELAY_TPINT_CONF	5000

/* commands for programming */
#define COMM_LOAD_CONFIG			0x00
#define COMM_LOAD_FOR_NVM			0x02
#define COMM_LOAD_FOR_NVM_J			0x22
#define COMM_READ_FROM_NVM			0x04
#define COMM_READ_FROM_NVM_J		0x24
#define COMM_INC_ADDR				0x06
#define COMM_LOAD_PC_ADDR			0x1D
#define COMM_BEGIN_IN_TIMED_PROG	0x08
#define COMM_BEGIN_EXT_TIMED_PROG	0x18
#define COMM_END_PROG				0x0A
#define COMM_BULK_ERASE				0x09
#define COMM_ROW_ERASE				0x05

#define ENTER_PROGRAM_KEY	0x4D434850

void pic16f183xx::enter_program_mode(void)
{
	int i;

	//GPIO_IN(pic_mclr);
	GPIO_OUT(pic_mclr);

	GPIO_SET(pic_mclr);			/* apply VDD to MCLR pin */
	delay_us(DELAY_TENTS);	/* wait TENTS */
	GPIO_CLR(pic_mclr);			/* remove VDD from MCLR pin */
	GPIO_CLR(pic_clk);
	delay_us(DELAY_TENTH);		/* wait TENTH */
	/* Shift in the "enter program mode" key sequence (LSB! first) */
	for (i = 0; i < 32; i++) {
		if ( (ENTER_PROGRAM_KEY >> i) & 0x01 )
			GPIO_SET(pic_data);
		else
			GPIO_CLR(pic_data);

		delay_us(DELAY_TCKL);	/* Setup time */
		GPIO_SET(pic_clk);
		delay_us(DELAY_TCKH);	/* Hold time */
		GPIO_CLR(pic_clk);

	}
	GPIO_CLR(pic_data);

	//Last clock(Don't care data)
	delay_us(DELAY_TCKL);	/* Setup time */
	GPIO_SET(pic_clk);
	delay_us(DELAY_TCKH);	/* Hold time */
	GPIO_CLR(pic_clk);

	delay_us(10);	/* Hold time */
}

void pic16f183xx::exit_program_mode(void)
{

	GPIO_CLR(pic_clk);			/* stop clock on PGC */
	GPIO_CLR(pic_data);			/* clear data pin PGD */

	GPIO_IN(pic_mclr);
}

/* Send a 6-bit command to the PIC (LSB first) */
void pic16f183xx::send_cmd(uint8_t cmd, unsigned int delay)
{
	int i;

	for (i = 0; i < 6; i++) {
		GPIO_SET(pic_clk);
		if ( (cmd >> i) & 0x01 )
			GPIO_SET(pic_data);
		else
			GPIO_CLR(pic_data);
		delay_us(DELAY_TCKH);	/* Setup time */
		GPIO_CLR(pic_clk);
		delay_us(DELAY_TCKL);	/* Hold time */
	}
	GPIO_CLR(pic_data);
	delay_us(delay);
}

/* Read 16-bit data from the PIC (LSB first) */
uint16_t pic16f183xx::read_data(void)
{
	uint8_t i;
	uint16_t data = 0x0000;

	GPIO_IN(pic_data);

	for (i = 0; i < 16; i++) {
		GPIO_SET(pic_clk);
		delay_us(DELAY_TCKH);
		delay_us(DELAY_TCO);	/* Wait for data to be valid */
		data |= ( GPIO_LEV(pic_data) & 0x00000001 ) << i;
		GPIO_CLR(pic_clk);
		delay_us(DELAY_TCKL);
	}

	GPIO_IN(pic_data);
	GPIO_OUT(pic_data);
	data >>= 1;
	return data;
}

/* Load 16-bit data to the PIC (LSB first) */
void pic16f183xx::write_data(uint16_t data)
{
	int i;
	data <<= 1;

	for (i = 0; i < 16; i++) {
		GPIO_SET(pic_clk);
		if ( (data >> i) & 0x0001 )
			GPIO_SET(pic_data);
		else
			GPIO_CLR(pic_data);
		delay_us(DELAY_SETUP);	/* Setup time */
		GPIO_CLR(pic_clk);
		delay_us(DELAY_HOLD);	/* Hold time */
	}
	GPIO_CLR(pic_data);
}

/* set Table Pointer */
void pic16f183xx::set_address(uint32_t addr)
{
	send_cmd(COMM_LOAD_PC_ADDR, DELAY_TDLY);

	int i;
	addr <<= 1;

	for (i = 0; i < 24; i++) {
		GPIO_SET(pic_clk);
		if ( (addr >> i) & 0x0001 )
			GPIO_SET(pic_data);
		else
			GPIO_CLR(pic_data);
		delay_us(DELAY_SETUP);	/* Setup time */
		GPIO_CLR(pic_clk);
		delay_us(DELAY_HOLD);	/* Hold time */
	}
	GPIO_CLR(pic_data);
	delay_us(10);
}

/* Read PIC device id word */
bool pic16f183xx::read_device_id(void)
{
	uint16_t id, rev;
	bool found = 0;

	cout << "Reading device id\n";

	send_cmd(COMM_LOAD_CONFIG, DELAY_TDLY);
	write_data(0x00);

	for(int i=0; i < 6; i++){
		send_cmd(COMM_INC_ADDR, DELAY_TDLY);
	}
	send_cmd(COMM_READ_FROM_NVM, DELAY_TDLY);
	id = read_data();
	device_id = id & 0x3FFF;

	send_cmd(COMM_INC_ADDR, DELAY_TDLY);
	send_cmd(COMM_READ_FROM_NVM, DELAY_TDLY);
	rev = read_data();
	device_rev = rev & 0x3FFF;

	for (unsigned short i=0;i < sizeof(piclist)/sizeof(piclist[0]);i++){

		if (piclist[i].device_id == device_id){

			strcpy(name,piclist[i].name);
			mem.code_memory_size = piclist[i].code_memory_size;
			mem.program_memory_size = 0x0F80018;
			mem.location = (uint16_t*) calloc(mem.program_memory_size,sizeof(uint16_t));
			mem.filled = (bool*) calloc(mem.program_memory_size,sizeof(bool));
			found = 1;
			break;
		}
	}
	return found;
}

/* Blank Check */
uint8_t pic16f183xx::blank_check(void)
{
	unsigned int lcounter = 0;
	int i;

	uint16_t addr, data;
	uint8_t ret = 0;

	if(!flags.debug) cerr << "[ 0%]";
	lcounter = 0;

	set_address(0);

	for(addr = 0; addr < mem.code_memory_size; addr++){
		send_cmd(COMM_READ_FROM_NVM_J, DELAY_TDLY);
		data = read_data() & 0x3FFF;

		if(data != 0x3FFF) {
			fprintf(stderr, "\nProgram MEM not Blank! Address: 0x%x, Read: 0x%x.\n",  addr, data);
			ret = 1;
			break;
		}

		if(lcounter != addr*100/mem.code_memory_size){
			lcounter = addr*100/mem.code_memory_size;
			fprintf(stderr, "\b\b\b\b\b[%2d%%]", lcounter);
		}
	}

	/* Read Configuration Fuses */
	addr = 0x8007;
	set_address(addr);

	for(i=0; i<4; i++){
		send_cmd(COMM_READ_FROM_NVM, DELAY_TDLY);
		data = read_data() & 0x3FFF;;
		if (data != 0x3FFF) {
			fprintf(stderr, "Config WORDS not Blank! Address: 0x%x, Read: 0x%x.\n",  addr, data);
			ret = 1;
			break;
		}
		send_cmd(COMM_INC_ADDR, DELAY_TDLY);
	}

	if(!flags.debug) cerr << "\b\b\b\b\b";

	return ret;

}

/* Bulk erase the chip */
void pic16f183xx::bulk_erase(void)
{
	set_address(0x8000);
	send_cmd(COMM_BULK_ERASE, DELAY_TERAB);
	if(flags.client) fprintf(stdout, "@FIN");
}

/* Read PIC memory and write the contents to a .hex file */
void pic16f183xx::read(char *outfile, uint32_t start, uint32_t count)
{
	fprintf(stderr, "\nERROR: Read Chip is not implemented!\n");
	exit(99);
}

/* Bulk erase the chip, and then write contents of the .hex file to the PIC */
void pic16f183xx::write(char *infile)
{
	int i;
	uint16_t data, fileconf;
	uint32_t addr = 0x00000000;
	uint8_t latch_size = 32;

	read_inhx(infile, &mem);

	bulk_erase();

	if(!flags.debug) cerr << "[ 0%]";
	if(flags.client) fprintf(stdout, "@000");
	unsigned int lcounter = 0;

	set_address(addr);

	for (addr = 0; addr < mem.code_memory_size; addr += latch_size){        /* address in WORDS (2 Bytes) */
		if (flags.debug)
			fprintf(stderr, "Current address 0x%08X \n", addr);
		for(i=0; i<latch_size-1; i++){		                        /* write the first 62 bytes */
			if (mem.filled[addr+i]) {
				if (flags.debug)
					fprintf(stderr, "  Writing 0x%04X to address 0x%06X \n", mem.location[addr + i], (addr+i) );
				send_cmd(COMM_LOAD_FOR_NVM_J, DELAY_TDLY);
				write_data(mem.location[addr+i]);
			}
			else {
				if (flags.debug)
					fprintf(stderr, "  Writing 0x3FFF to address 0x%06X \n", (addr+i) );
				send_cmd(COMM_LOAD_FOR_NVM_J, DELAY_TDLY);
				write_data(0x3FFF);			/* write 0x3FFF in empty locations */
			}
		}

		/* write the last 2 bytes and start programming */
		if (mem.filled[addr+latch_size-1]) {
			if (flags.debug)
				fprintf(stderr, "  Writing 0x%04X to address 0x%06X and then start programming...\n", mem.location[addr+latch_size-1], (addr+latch_size-1));
			send_cmd(COMM_LOAD_FOR_NVM, DELAY_TDLY);
			write_data(mem.location[addr+latch_size-1]);
		}
		else {
			if (flags.debug)
				fprintf(stderr, "  Writing 0x3FFF to address 0x%06X and then start programming...\n", (addr+latch_size-1));
			send_cmd(COMM_LOAD_FOR_NVM, DELAY_TDLY);
			write_data(0x3FFF);			         /* write 0x3FFF in empty locations */
		}

		/* Programming Sequence */
		send_cmd(COMM_BEGIN_IN_TIMED_PROG, DELAY_TPINT_DATA);
		/* end of Programming Sequence */

		send_cmd(COMM_INC_ADDR, DELAY_TDLY);

		if(lcounter != addr*100/mem.code_memory_size){
			lcounter = addr*100/mem.code_memory_size;
			if(flags.client)
				fprintf(stdout,"@%03d", lcounter);
			if(!flags.debug)
				fprintf(stderr,"\b\b\b\b\b[%2d%%]", lcounter);
		}
	}

	if(!flags.debug) cerr << "\b\b\b\b\b\b";
	if(flags.client) fprintf(stdout, "@100");

	/* Write Confuguration Fuses
	* Writing User ID is not implemented.
	*/
	addr = 0x8007;
	set_address(addr);

	for(i=0; i<3; i++){
		if (mem.filled[addr+i]) {
			if (flags.debug)
				fprintf(stderr, "  Writing 0x%04X to config address 0x%06X \n", mem.location[addr + i], (addr+i) );
			send_cmd(COMM_LOAD_FOR_NVM, DELAY_TDLY);
			write_data(mem.location[addr+i]);
			send_cmd(COMM_BEGIN_IN_TIMED_PROG, DELAY_TPINT_CONF);
			send_cmd(COMM_INC_ADDR, DELAY_TDLY);
		}
	}

	/* Verify Code Memory and Configuration Word */
	if(!flags.noverify){
		cout << "\nVerifying chip...";
		if(!flags.debug) cerr << "[ 0%]";
		if(flags.client) fprintf(stdout, "@000");
		lcounter = 0;

		set_address(0);

		for (addr = 0; addr < mem.code_memory_size; addr++) {
			send_cmd(COMM_READ_FROM_NVM_J, DELAY_TDLY);
			data = read_data() & 0x3FFF;
			//send_cmd(COMM_INC_ADDR, DELAY_TDLY);

			if (flags.debug)
				fprintf(stderr, "Check addr = 0x%06X:  pic = 0x%04X, file = 0x%04X\n",
						addr, data, (mem.filled[addr]) ? (mem.location[addr]) : 0x3FFF);

			if ( (data != mem.location[addr]) & ( mem.filled[addr]) ) {
				fprintf(stderr, "Error at addr = 0x%06X:  pic = 0x%04X, file = 0x%04X.\nExiting...",
						addr, data, mem.location[addr]);
				exit(32);
			}
			if(lcounter != addr*100/mem.code_memory_size){
				lcounter = addr*100/mem.code_memory_size;
				if(flags.client)
					fprintf(stdout,"@%03d", lcounter);
				if(!flags.debug)
					fprintf(stderr,"\b\b\b\b\b[%2d%%]", lcounter);
			}
		}


		/* Read Confuguration Registers */
		set_address(0x8007);

		addr = 0x8007;
		uint16_t mask = 0x3FFF;

		for(i=0; i<3; i++){
			send_cmd(COMM_READ_FROM_NVM, DELAY_TDLY);
			data = read_data() & mask;
			fileconf = mem.location[addr+i] & mask;
			if ( ( data != fileconf ) & ( mem.filled[addr+i] ) ) {
				fprintf(stderr, "Error at fuse addr = 0x%06X:  pic = 0x%04X, file = 0x%04X.\nExiting...",
						addr+i, data, mem.location[addr+i] & mask);
				exit(34);
			}
			send_cmd(COMM_INC_ADDR, DELAY_TDLY);
		}
	}

	/* Write Code protection fuse */
	addr = 0x800A;
	set_address(addr);

	if (mem.filled[addr]) {
		if (flags.debug)
			fprintf(stderr, "  Writing 0x%04X to config address 0x%06X \n", mem.location[addr], (addr) );
		send_cmd(COMM_LOAD_FOR_NVM, DELAY_TDLY);
		write_data(mem.location[addr]);
		send_cmd(COMM_BEGIN_IN_TIMED_PROG, DELAY_TPINT_CONF);
	}
}

/* Dum configuration words */
void pic16f183xx::dump_configuration_registers(void)
{
	fprintf(stderr, "\nERROR: Dump config register is not implemented!\n");
	exit(99);
}
