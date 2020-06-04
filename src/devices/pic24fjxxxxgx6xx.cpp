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

#include <stdint.h>
#include <string.h>
#include <iostream>
#include <unistd.h>

#include "pic24fjxxxxgx6xx.h"

/* delays (in microseconds; nanoseconds are rounded to 1us) */
#define DELAY_P1   			1		// 100ns
#define DELAY_P1A			1		// 40ns
#define DELAY_P1B			1		// 40ns
#define DELAY_P2			1		// 15ns
#define DELAY_P3			1		// 15ns
#define DELAY_P4			1		// 40ns
#define DELAY_P4A			1		// 40ns
#define DELAY_P5			1		// 20ns
#define DELAY_P6			1		// 100ns
#define DELAY_P7			50000		// 50ms
#define DELAY_P8			12		// 12us
#define DELAY_P9			40		// 40us
#define DELAY_P10			1		// 400ns
#define DELAY_P11			20000		// 20ms
#define DELAY_P12			20000		// 20ms
#define DELAY_P13			20		// 20us
#define DELAY_P14			1		// 1us MAX!
#define DELAY_P15			1		// 10ns
#define DELAY_P16			0		// 0s
#define DELAY_P17   		1		// 0s
#define DELAY_P18			1000		// 1ms
#define DELAY_P19			1		// 1ms
#define DELAY_P20			23		// 23us
#define DELAY_P21			100		// 100us

#define ENTER_PROGRAM_KEY	0x4D434851

#define reset_pc() send_cmd(0x040200)
#define send_nop() send_cmd(0x000000)

static unsigned int counter=0;
static uint16_t nvmcon;

/* Send a 24-bit command to the PIC (LSB first) through a SIX instruction */
void pic24fjxxxxgx6xx::send_cmd(uint32_t cmd)
{
	uint8_t i;

	GPIO_CLR(pic_data);

	/* send the SIX = 0x0000 instruction */
	for (i = 0; i < 4; i++) {
		GPIO_SET(pic_clk);
		delay_us(DELAY_P1B);
		GPIO_CLR(pic_clk);
		delay_us(DELAY_P1A);
	}

	delay_us(DELAY_P4);

	/* send the 24-bit command */
	for (i = 0; i < 24; i++) {
		if ( (cmd >> i) & 0x00000001 )
			GPIO_SET(pic_data);
		else
			GPIO_CLR(pic_data);
		delay_us(DELAY_P1A);
		GPIO_SET(pic_clk);
		delay_us(DELAY_P1B);
		GPIO_CLR(pic_clk);
	}

	delay_us(DELAY_P4A);
}

/* Read 16-bit data word from the PIC (LSB first) through a REGOUT inst */
uint16_t pic24fjxxxxgx6xx::read_data(void)
{
	uint8_t i;
	uint16_t data = 0;

	GPIO_CLR(pic_data);
	GPIO_CLR(pic_clk);

	/* send the REGOUT=0x0001 instruction */
	for (i = 0; i < 4; i++) {
		if ( (0x0001 >> i) & 0x001 )
			GPIO_SET(pic_data);
		else
			GPIO_CLR(pic_data);
		delay_us(DELAY_P1A);
		GPIO_SET(pic_clk);
		delay_us(DELAY_P1B);
		GPIO_CLR(pic_clk);
	}

	delay_us(DELAY_P4);

	/* idle for 8 clock cycles, waiting for the data to be ready */
	for (i = 0; i < 8; i++) {
		GPIO_SET(pic_clk);
		delay_us(DELAY_P1B);
		GPIO_CLR(pic_clk);
		delay_us(DELAY_P1A);
	}

	delay_us(DELAY_P5);

	GPIO_IN(pic_data);

	/* read a 16-bit data word */
	for (i = 0; i < 16; i++) {
		GPIO_SET(pic_clk);
		delay_us(DELAY_P1B);
		data |= ( GPIO_LEV(pic_data) & 0x00000001 ) << i;
		GPIO_CLR(pic_clk);
		delay_us(DELAY_P1A);
	}

	delay_us(DELAY_P4A);
	GPIO_OUT(pic_data);
	return data;
}

/* Enter program mode */
void pic24fjxxxxgx6xx::enter_program_mode(void)
{
	int i;

	GPIO_OUT(pic_mclr);
	GPIO_OUT(pic_data);

	GPIO_CLR(pic_clk);

	GPIO_CLR(pic_mclr);		/*  remove VDD from MCLR pin */
	delay_us(DELAY_P6);
	GPIO_SET(pic_mclr);		/*  apply VDD to MCLR pin */
	delay_us(DELAY_P21);
	GPIO_CLR(pic_mclr);		/* remove VDD from MCLR pin */
	delay_us(DELAY_P18);

	/* Shift in the "enter program mode" key sequence (MSB first) */
	for (i = 31; i > -1; i--) {
		if ( (ENTER_PROGRAM_KEY >> i) & 0x01 )
			GPIO_SET(pic_data);
		else
			GPIO_CLR(pic_data);
		delay_us(DELAY_P1A);
		GPIO_SET(pic_clk);
		delay_us(DELAY_P1B);
		GPIO_CLR(pic_clk);

	}

	GPIO_CLR(pic_data);
	delay_us(DELAY_P19);
	GPIO_SET(pic_mclr);
	delay_us(DELAY_P7);
	delay_us(DELAY_P1*5);

	/*
	 * Coming out of Reset, ther first 4-bit control code is always forced
	 * to SIX and a a forced NOP instruction is executed by the CPU. Five
	 * additional PGCx clocks are needed on start-up, resulting in a 9-bit
	 * SIX command instead of the normal 4-bit SIX command.
	 */
	for (i = 0; i < 5; i++) {
		GPIO_SET(pic_clk);
		delay_us(DELAY_P1A);
		GPIO_CLR(pic_clk);
		delay_us(DELAY_P1B);
	}
}

/* Exit program mode */
void pic24fjxxxxgx6xx::exit_program_mode(void)
{
	GPIO_CLR(pic_clk);
	GPIO_CLR(pic_data);
	delay_us(DELAY_P16);
	GPIO_CLR(pic_mclr);	/* remove VDD from MCLR pin */
	delay_us(DELAY_P17);	/* wait (at least) P17 */
	GPIO_IN(pic_mclr);
}

/* Read the device ID and revision; returns only the id */
bool pic24fjxxxxgx6xx::read_device_id(void)
{
	bool found = 0;

	/* Exit Reset vector */
	send_nop();
	reset_pc();
	send_nop();

	/* Initialize TBLPAG and the Read Pointer (W6) for TBLRD instruction */
	send_cmd(0x200FF0); // MOV #<SourceAddress23:16>, W0
	send_cmd(0x8802A0); // MOV W0, TBLPAG
	send_cmd(0x200006); // MOV #<SourceAddress15:0>, W6

	/* Initialize the Write Pointer (W7) to point to the VISI register. */
	send_cmd(0x207847); // MOV #VISI, W7
	send_nop();

	/*
	 * Read and clock out the contents of the next two locations of code
	 * memory, through the VISI register, using the REGOUT command.
	 */
	send_cmd(0xBA0B96); // TBLRDL [W6], [W7]
	send_nop();
	send_nop();
	device_id = read_data(); // Clock out contents of VISI register
	send_nop();

	send_cmd(0xBADBB6); // TBLRDH.B [W6++], [W7++]
	send_nop();
	send_nop();

	send_cmd(0xBAD3D6); // TBLRDH.B [++W6], [W7--]
	send_nop();
	send_nop();
	device_rev = read_data(); // Clock out contents of VISI register
	send_nop();

	send_cmd(0xBA0BB6); // TBLRDL [W6++], [W7]
	send_nop();
	send_nop();
	device_rev = read_data(); // Clock out contents of VISI register
	send_nop();

	/* Reset device internal PC */
	reset_pc();
	send_nop();

	for (unsigned short i = 0; i < sizeof(piclist)/sizeof(piclist[0]); i++) {
		if (piclist[i].device_id == device_id) {
			strcpy(name, piclist[i].name);
			mem.code_memory_size = piclist[i].code_memory_size;
			mem.program_memory_size = 0x0ABFFE;
			mem.location = (uint16_t*) calloc(mem.program_memory_size,sizeof(uint16_t));
			mem.filled = (bool*) calloc(mem.program_memory_size,sizeof(bool));
			found = 1;
			break;
		}
	}

	return found;
}

/* Check if the device is blank */
uint8_t pic24fjxxxxgx6xx::blank_check(void)
{
	uint32_t addr;
	unsigned short i;
	uint16_t data[8], raw_data[6];
	uint8_t ret = 0;

	if(!flags.debug)
	  cerr << "[ 0%]";

	counter=0;

	/* Exit Reset vector */
	send_nop();
	reset_pc();
	send_nop();

	/* Output data to W0:W5; repeat until all desired code memory is read. */
	for (addr = 0; addr < mem.code_memory_size; addr = addr + 8) {
		if ((addr & 0x0000FFFF) == 0) {
			send_cmd(0x200000 | ((addr & 0x00FF0000) >> 12) ); // MOV #<DestAddress23:16>, W0
			send_cmd(0x880190);	// MOV W0, TBLPAG
			send_cmd(0x200006 | ((addr & 0x0000FFFF) << 4) ); // MOV #<DestAddress15:0>, W6
		}

		/* Fetch the next four memory locations and put them to W0:W5 */

		/* Initialize the Write Pointer (w7) to point to the VISI register */
		send_cmd(0x207847); // MOV #VISI, W7
		send_nop();

		send_cmd(0xEB0380); // CLR W7
		send_nop();
		send_cmd(0xBA1B96); // TBLRDL [W6], [W7++]
		send_nop();
		send_nop();
		send_cmd(0xBADBB6); // TBLRDH.B [W6++], [W7++]
		send_nop();
		send_nop();
		send_cmd(0xBADBD6); // TBLRDH.B [++W6], [W7++]
		send_nop();
		send_nop();
		send_cmd(0xBA1BB6); // TBLRDL [W6++], [W7++]
		send_nop();
		send_nop();
		send_cmd(0xBA1B96); // TBLRDL [W6], [W7++]
		send_nop();
		send_nop();
		send_cmd(0xBADBB6); // TBLRDH.B [W6++], [W7++]
		send_nop();
		send_nop();
		send_cmd(0xBADBD6); // TBLRDH.B [++W6], [W7++]
		send_nop();
		send_nop();
		send_cmd(0xBA0BB6); // TBLRDL [W6++], [W7]
		send_nop();
		send_nop();

		/* Read six data words (16 bits each) */
		for (i = 0; i < 6; i++) {
			send_cmd(0x883C20 + i); // MOV (W0 + i), VISI
			send_nop();
			raw_data[i] = read_data();
			send_nop();
		}

		reset_pc();
		send_nop();

		/* store data correctly */
		data[0] = raw_data[0];
		data[1] = raw_data[1] & 0x00FF;
		data[3] = (raw_data[1] & 0xFF00) >> 8;
		data[2] = raw_data[2];
		data[4] = raw_data[3];
		data[5] = raw_data[4] & 0x00FF;
		data[7] = (raw_data[4] & 0xFF00) >> 8;
		data[6] = raw_data[5];

		if(counter != addr * 100 / mem.code_memory_size){
			counter = addr * 100 / mem.code_memory_size;
			fprintf(stderr, "\b\b\b\b\b[%2d%%]", counter);
		}

		for (i = 0; i < 8; i++) {
			/* If we are at the end of code_memory_size just break */
			if ((addr + i) > mem.code_memory_size)
			  break;
			if (flags.debug)
				fprintf(stderr, "\n addr = 0x%06X data = 0x%04X", (addr + i), data[i]);
			if ((i%2 == 0 && data[i] != 0xFFFF) || (i%2 == 1 && data[i] != 0x00FF)) {
				if (!flags.debug)
				  cerr << "\b\b\b\b\b";
				ret = 1;
				addr = mem.code_memory_size + 10;
				break;
			}
		}
	}

	if (addr <= (mem.code_memory_size + 8)) {
		if (!flags.debug)
		  cerr << "\b\b\b\b\b";
		ret = 0;
	};

	/* Exit Reset vector */
	send_nop();
	reset_pc();
	send_nop();

	return ret;
}

/* Bulk erase the chip */
void pic24fjxxxxgx6xx::bulk_erase(void)
{
	/* Exit the Reset vector */
	send_nop();
	reset_pc();
	send_nop();

	/* Configure the NVMCON register to perform a Chip Erase */
	send_cmd(0x2400E0); // MOV #0x400E, W0
	send_cmd(0x883B00); // MOV W0, NVMCON

	/* Set the WR bit */
	send_cmd(0x200550); // MOV #0x55, W0
	send_cmd(0x883B30); // MOV W0, NVMKEY
	send_cmd(0x200AA0); // MOV #0xAA, W0
	send_cmd(0x883B30); // MOV W0, NVMKEY
	send_cmd(0xA8E761);	// BSET NVMCON, #WR
	send_nop();
	send_nop();
	send_nop();

	delay_us(DELAY_P11);

	/* Wait while the erase operation completes */
	do {
		reset_pc();
		send_nop();
		send_cmd(0x803B02); // MOV NVMCON, W2
		send_cmd(0x883C22); // MOV W2, VISI
		send_nop();
		nvmcon = read_data(); // Clock out contents of the VISI register
		send_nop();
	} while((nvmcon & 0x8000) == 0x8000);

	/* Clear the WREN bit */
	send_cmd(0x200000);	//MOV #0000, W0
	send_cmd(0x883B00);	//MOV W0, NVMCON

	if(flags.client)
		fprintf(stdout, "@FIN");
}

/* Read PIC memory and write the contents to a .hex file */
void pic24fjxxxxgx6xx::read(char *outfile, uint32_t start, uint32_t count)
{
	uint32_t addr, startaddr, stopaddr;
	uint16_t data[8], raw_data[6];
	int i = 0;

	startaddr = start;
	stopaddr = mem.code_memory_size;

	if (count != 0 && count < stopaddr) {
		stopaddr = startaddr + count;
		fprintf(stderr, "Read only %d memory locations, from %06X to %06X\n",
			count, startaddr, stopaddr);
	}

	if (!flags.debug) cerr << "[ 0%]";
	if (flags.client) fprintf(stdout, "@000");

	counter = 0;

	/* Exit Reset vector */
	send_nop();
	reset_pc();
	send_nop();

	/* Output data to W0:W5; repeat until all desired code memory is read. */
	for (addr = startaddr; addr < stopaddr; addr = addr + 8) {
		if((addr & 0x0000FFFF) == 0 || startaddr != 0) {
			send_cmd(0x200000 | ((addr & 0x00FF0000) >> 12) ); // MOV #<DestAddress23:16>, W0
			send_cmd(0x880190); // MOV W0, TBLPAG
			send_cmd(0x200006 | ((addr & 0x0000FFFF) << 4) ); // MOV #<DestAddress15:0>, W6
			startaddr = 0;
		}

		/* Fetch the next four memory locations and put them to W0:W5 */

		/* Initialize the Write Pointer (w7) to point to the VISI register */
		send_cmd(0x207847); // MOV #VISI, W7
		send_nop();

		send_cmd(0xEB0380); // CLR W7
		send_nop();
		send_cmd(0xBA1B96); // TBLRDL [W6], [W7++]
		send_nop();
		send_nop();
		send_cmd(0xBADBB6); // TBLRDH.B [W6++], [W7++]
		send_nop();
		send_nop();
		send_cmd(0xBADBD6); // TBLRDH.B [++W6], [W7++]
		send_nop();
		send_nop();
		send_cmd(0xBA1BB6); // TBLRDL [W6++], [W7++]
		send_nop();
		send_nop();
		send_cmd(0xBA1B96); // TBLRDL [W6], [W7++]
		send_nop();
		send_nop();
		send_cmd(0xBADBB6); // TBLRDH.B [W6++], [W7++]
		send_nop();
		send_nop();
		send_cmd(0xBADBD6); // TBLRDH.B [++W6], [W7++]
		send_nop();
		send_nop();
		send_cmd(0xBA0BB6); // TBLRDL [W6++], [W7]
		send_nop();
		send_nop();

		/* Read six data words (16 bits each) */
		for (i = 0; i < 6; i++) {
			send_cmd(0x883C20 + i); // MOV (W0 + i), VISI
			send_nop();
			raw_data[i] = read_data();
			send_nop();
		}

		reset_pc();
		send_nop();

		/* store data correctly */
		data[0] = raw_data[0];
		data[1] = raw_data[1] & 0x00FF;
		data[3] = (raw_data[1] & 0xFF00) >> 8;
		data[2] = raw_data[2];
		data[4] = raw_data[3];
		data[5] = raw_data[4] & 0x00FF;
		data[7] = (raw_data[4] & 0xFF00) >> 8;
		data[6] = raw_data[5];

		for (i = 0; i < 8; i++) {
			if (flags.debug)
				fprintf(stderr, "\n addr = 0x%06X data = 0x%04X",
					(addr + i), data[i]);

			if (i % 2 == 0 && data[i] != 0xFFFF) {
				mem.location[addr + i] = data[i];
				mem.filled[addr + i] = 1;
			}

			if (i % 2 == 1 && data[i] != 0x00FF) {
				mem.location[addr+i] = data[i];
				mem.filled[addr+i] = 1;
			}
		}

		if (counter != addr * 100 / stopaddr) {
			counter = addr * 100 / stopaddr;
			if (flags.client)
				fprintf(stdout,"@%03d", counter);
			if (!flags.debug)
				fprintf(stderr,"\b\b\b\b\b[%2d%%]", counter);
		}

		/* TODO: checksum */
	}

	/* READ CONFIGURATION REGISTERS */
	addr = mem.code_memory_size;

	/* Exit Reset vector */
	send_nop();
	reset_pc();
	send_nop();

	/*
	 * Initialize TBLPAG, the Read Pointer (W6) and the Write Pointer (W7)
	 * for TBLRD instruction
	 */
	send_cmd(0x200000 | ((addr & 0x00FF0000) >> 12) ); // MOV #<DestAddress23:16>, W0
	send_cmd(0x880190); // MOV W0, TBLPAG
	send_cmd(0x200007 | ((addr & 0x0000FFFF) << 4) ); // MOV #<DestAddress15:0>, W6
	send_cmd(0x207847); // MOV #VISI, W7
	send_nop();

	for (i = 0; i < 2; i++) {
		send_cmd(0xBA0BB6); // TBLRDL [W6++], [W7]
		send_nop();
		send_nop();
		data[0] = read_data();

		if (data[0] != 0xFFFF) {
			mem.location[addr + 2 * i] = data[0];
			mem.filled[addr + 2 * i] = 1;
		}
	}

	reset_pc();
	send_nop();

	if(!flags.debug) cerr << "\b\b\b\b\b";
	if(flags.client) fprintf(stdout, "@FIN");
	write_inhx(&mem, outfile);
}

/* Write contents of the .hex file to the PIC */
void pic24fjxxxxgx6xx::write(char *infile)
{
	uint16_t i,j,p;
	uint16_t k;
	bool skip;
	uint32_t data[8], raw_data[6];
	uint32_t addr = 0;

	unsigned int filled_locations=1;

	const char *regname[] = {"FSEC","FBSLIM", "FSIGN", "FOSCSEL", "FOSC", "FWDT", "FPOR", "FICD", "FDEVOPT"};

	filled_locations = read_inhx(infile, &mem);
	if (!filled_locations) return;

	bulk_erase();

	/* WRITE CODE MEMORY */

	/* Exit Reset vector */
	send_nop();
	reset_pc();
	send_nop();

	/* Initialize the TBLPAG register for writing to the latches */
	send_cmd(0x200FAC); // MOV #0xFA, W12
	send_cmd(0x8802AC); // MOV W12, TBLPAG

	if (!flags.debug) cerr << "[ 0%]";
	if (flags.client) fprintf(stdout, "@000");

	counter = 0;

	for (addr = 0; addr < mem.code_memory_size; ){
		
		skip = 1;

		for (k = 0; k < 4; k += 2)
			if (mem.filled[addr + k]) skip = 0;

		if (skip) {
			addr = addr + 4;
			continue;
		}

		/* Set the NVMADRU/NVMADR register pair to point to the correct address */
		send_cmd(0x200003 | ((addr & 0x0000FFFF) << 4) ); // MOV #<DestinationAddress15:0>, W3
		send_cmd(0x200004 | ((addr & 0x00FF0000) >> 12) ); // MOV #<DestinationAddress23:16>, W4
		send_cmd(0x883B13);	//MOV W3, NVMADR
		send_cmd(0x883B24);	//MOV W4, NVMADRU

		send_cmd(0x200000 | (mem.location[addr] << 4)); // MOV #<LSW0>, W0
		send_cmd(0x200001 | (0x00FFFF & ((mem.location[addr+3] << 8) | (mem.location[addr+1] & 0x00FF))) <<4); // MOV #<MSB1:MSB0>, W1
		send_cmd(0x200002 | (mem.location[addr+2] << 4)); // MOV #<LSW1>, W2

		/* Set the Read Pointer (W6) and load the (next set of) write latches */
		send_cmd(0xEB0300); // CLR W6
		send_nop();
		send_cmd(0xEB0380); // CLR W7
		send_nop();
		send_cmd(0xBB0BB6); // TBLWTL [W6++], [W7]
		send_nop();
		send_nop();
		send_cmd(0xBBDBB6); // TBLWTH.B [W6++], [W7++]
		send_nop();
		send_nop();
		send_cmd(0xBBEBB6); // TBLWTH.B [W6++], [++W7]
		send_nop();
		send_nop();
		send_cmd(0xBB1BB6); // TBLWTL [W6++], [W7++]
		send_nop();
		send_nop();

		addr = addr + 4;

		/* Set the NVMCON to program 4 instruction words */
		send_cmd(0x24001A); // MOV #0x4001, W10
		send_cmd(0x883B0A); // MOV W10, NVMCON

		/* Execute the WR bit unlock sequence and initiate the write cycle */
		send_cmd(0x200551);	// MOV #0x55, W1
		send_cmd(0x883B31);	// W1, NVMKEY
		send_cmd(0x200AA1);	// MOV #0xAA, W1
		send_cmd(0x883B31);	// MOV W1, NVMKEY
		send_cmd(0xA8E761);	// BSET NVMCON, #WR
		send_nop();
		send_nop();
		send_nop();

		/* Repeat this step to poll the WR bit until it is cleared by hardware */
		do {
			send_cmd(0x803B00);	// MOV NVMCON, W0
			send_cmd(0x883C20);	// W0, VISI
			send_nop();
			nvmcon = read_data(); // Clock out contents of the VISI register
			send_nop();
			reset_pc();
			send_nop();
		} while ((nvmcon & 0x8000) == 0x8000);

		if (counter != addr * 100 / filled_locations) {
			if (flags.client)
				fprintf(stdout,"@%03d", (addr * 100 / (filled_locations + 0x80)));
			if (!flags.debug)
				fprintf(stderr,"\b\b\b\b\b[%2d%%]", addr * 100 / (filled_locations + 0x80));
			counter = addr * 100 / filled_locations;
		}
	};

	/* Clear the WREN bit */
	send_cmd(0x200000);	//MOV #0000, W0
	send_cmd(0x883B00);	//MOV W0, NVMCON

	if (!flags.debug) cerr << "\b\b\b\b\b\b";
	if (flags.client) fprintf(stdout, "@100");

	delay_us(100000);
	
	/* WRITE CONFIGURATION REGISTERS */
	if (flags.debug)
		cerr << endl << "Writing Configuration registers..." << endl;

	/* Exit the Reset vector */
	send_nop();
	reset_pc();
	send_nop();

	/* Initialize the Write Pointer (W7) for TBLWT instruction */

	/* Initialize the TBLPAG register for writing to the latches */
	send_cmd(0x200FAC); // MOV #0xFA, W12
	send_cmd(0x8802AC); // MOV W12, TBLPAG

	for (i = 0; i < 9; i++) 
	{
		addr = cword_address[i];

		if (mem.filled[addr]) {
			/* Load W0:W1 with the next two Configuration Words to program */
			send_cmd(0x200000 | ((0x0000FFFF & mem.location[cword_address[i]]) << 4));	// MOV #<Config lower word data>, W0
			send_cmd(0x2FFFF1); 											// MOV #<Config upper word data>, W1

			/* Set the Read Pointer (W6) and Write Pointer (W7), and load the (next set of) write latches */
			send_cmd(0xEB0300);	// CLR W6
			send_nop();
			send_cmd(0xEB0380);	// CLR W7
			send_nop();
			send_cmd(0xBB0BB6);	// TBLWTL [W6++], [W7]
			send_nop();
			send_nop();
			send_cmd(0xBBDBB6);	// TBLWTH.B [W6++], [W7++]
			send_nop();
			send_nop();
			send_cmd(0xBBEBB6);	// TBLWTH.B [W6++], [++W7] 
			send_nop();
			send_nop();
			send_cmd(0xBB1BB6);	// TBLWTL.W [W6++], [W7++] 
			send_nop();
			send_nop();

			/* Set the NVMADRU/NVMADR register pair to point to the correct address */
			send_cmd(0x200003 | ((cword_address[i] & 0x0000FFFF) <<  4));	// MOV #DestinationAddress<15:0>, W3
			send_cmd(0x2000A4);												// MOV #DestinationAddress<23:16>, W4
			send_cmd(0x883B13);	// MOV W3, NVMADR
			send_cmd(0x883B24);	// MOV W4, NVMADRU

			/* Set the NVMCON register to program two instruction words */
			send_cmd(0x24001A);	// MOV #0x4001, W10
			send_cmd(0x883B0A);	// MOV W10, NVMCON
			send_nop();

			/* Initiate the write cycle. */
			send_cmd(0x200551);	// MOV #0x55, W1
			send_cmd(0x883B31);	// MOV W1, NVMKEY
			send_cmd(0x200AA1);	// MOV #0xAA, W1
			send_cmd(0x883B31);	// MOV W1, NVMKEY
			send_cmd(0xA8E761);	// BSET NVMCON, #WR
			send_nop();
			send_nop();
			send_nop();

			delay_us(DELAY_P20);

			/* Wait while the erase operation completes */
			do {
				send_cmd(0x803B00);	// MOV NVMCON, W0
				send_cmd(0x883C20);	// MOV W0, VISI
				send_nop();
				nvmcon = read_data(); // Clock out contents of the VISI register
				send_nop();
				reset_pc();
				send_nop();
			} while ((nvmcon & 0x8000) == 0x8000);

			if(flags.debug)
				fprintf(stderr,"\n - %s set to 0x%01x",
						regname[i], mem.location[addr]);
		} else if(flags.debug) {
			fprintf(stderr,"\n - %s left unchanged", regname[i]);
		}
	}

	if (flags.debug) cerr << endl;

	delay_us(100000);

	/* VERIFY CODE MEMORY */
	if (!flags.noverify){
		if (!flags.debug) cerr << "[ 0%]";
		if (flags.client) fprintf(stdout, "@000");

		counter = 0;

		send_nop();
		reset_pc();
		send_nop();

		for (addr = 0; addr < mem.code_memory_size; addr = addr + 4) {
			skip = 1;

			for(k = 0; k < 4; k += 2)
				if (mem.filled[addr + k])
					skip = 0;

			if (skip) continue;

			/* Initialize the TBLPAG register and the Read Pointer (W6) for a TBLRD instruction */
			send_cmd(0x200000 | ((addr & 0x00FF0000) >> 12)); // MOV #<DestAddress23:16>, W0
			send_cmd(0x8802A0); // MOV W0, TBLPAG
			send_cmd(0x200006 | ((addr & 0x0000FFFF) << 4)); // MOV #<DestAddress15:0>, W6

			/* Initialize the Write Pointer (W7) to point to the VISI register */
			send_cmd(0x207847); // MOV #VISI, W7
			send_nop();

			/*
			* Read and clock out the contents of the next two locations of code
			* memory, through the VISI register, using the REGOUT command.
			*/
			send_cmd(0xBA0B96); 			// TBLRDL [W6], [W7]
			send_nop();
			send_nop();
			raw_data[0] = read_data(); 		// Clock out contents of VISI register
			send_nop();

			send_cmd(0xBADBB6); 			// TBLRDH.B [W6++], [W7++]
			send_nop();
			send_nop();
			
			send_cmd(0xBAD3D6); 			// TBLRDH.B [++W6], [W7--]
			send_nop();
			send_nop();
			raw_data[1] = read_data(); 		// Clock out contents of VISI register
			send_nop();

			send_cmd(0xBA0BB6); 			// TBLRDL [W6++], [W7]
			send_nop();
			send_nop();
			raw_data[2] = read_data(); 		// Clock out contents of VISI register
			send_nop();

			/* Reset device internal PC */
			reset_pc();
			send_nop();

			data[0] = raw_data[0];
			data[1] = (raw_data[1] & 0x00FF);
			data[2] = raw_data[2];
			data[3] = ((raw_data[1] & 0xFF00) >> 8);

			for (i = 0; i < 4; i++) {
				if (flags.debug)
					fprintf(stderr, "\n addr = 0x%06X data = 0x%04X", (addr+i), data[i]);

				if (mem.filled[addr + i] && data[i] != mem.location[addr + i]) {
					fprintf(stderr,"\n\n ERROR at address %06X: written %04X but %04X read!\n\n",
						addr + i, mem.location[addr + i], data[i]);
					return;
				}
			}

			if (counter != addr * 100 / filled_locations) {
				if (flags.client)
					fprintf(stdout,"@%03d", (addr*100/(filled_locations+0x100)));
				if (!flags.debug)
					fprintf(stderr,"\b\b\b\b\b[%2d%%]", addr*100/(filled_locations+0x100));
				counter = addr * 100 / filled_locations;
			}
		}

		if (!flags.debug) cerr << "\b\b\b\b\b";
		if (flags.client) fprintf(stdout, "@FIN");
	} else {
		if (flags.client) fprintf(stdout, "@FIN");
	}
}

/* Write to screen the configuration registers, without saving them anywhere */
void pic24fjxxxxgx6xx::dump_configuration_registers(void)
{
	uint32_t addr = mem.code_memory_size;

	cerr << endl << "Configuration registers:" << endl << endl;

	/* Exit Reset vector */
	send_nop();
	reset_pc();
	send_nop();

	/*
	 * Initialize TBLPAG, the Read Pointer (W6) and the Write Pointer (W7)
	 * for TBLRD instruction
	 */

	send_cmd(0x200000 | ((addr & 0x00FF0000) >> 12) ); // MOV #<DestAddress23:16>, W0
	send_cmd(0x880190); // MOV W0, TBLPAG
	send_cmd(0x200007 | ((addr & 0x0000FFFF) << 4) ); // MOV #<DestAddress15:0>, W6
	send_cmd(0x207847); // MOV #VISI, W7
	send_nop();

	for (unsigned short i = 2; i > 0; i--) {
		send_cmd(0xBA0BB6); // TBLRDL [W6++], [W7]
		send_nop();
		send_nop();
		fprintf(stderr," - CW%d: 0x%08x\n", i, read_data());
		send_nop();
	}

	cerr << endl;

	reset_pc();
	send_nop();
}
