/*
 * Raspberry Pi PIC Programmer using GPIO connector
 * https://github.com/WallaceIT/picberry
 * Copyright 2014 Francesco Valla
 *
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

#include "dspic33epxxgs50x.h"

/* delays (in microseconds; nanoseconds are rounded to 1us) */
#define DELAY_P1   			1		// 200ns
#define DELAY_P1A			1		// 80ns
#define DELAY_P1B			1		// 80ns
#define DELAY_P2			1		// 15ns
#define DELAY_P3			1		// 15ns
#define DELAY_P4			1		// 40ns
#define DELAY_P4A			1		// 40ns
#define DELAY_P5			1		// 20ns
#define DELAY_P6			1		// 100ns
#define DELAY_P7			50000	// 50ms
#define DELAY_P8			12		// 12us
#define DELAY_P9A			10		// 10us
#define DELAY_P9B			15		// 15us - 23us max!
#define DELAY_P10			1		// 400ns
#define DELAY_P11			25000	// 25ms
#define DELAY_P12			25000	// 25ms
#define DELAY_P13A			1000	// 1ms
#define DELAY_P13B			50		// 50us
#define DELAY_P14			1		// 1us MAX!
#define DELAY_P15			1		// 10ns
#define DELAY_P16			0		// 0s
#define DELAY_P17   		10		// 100ns
#define DELAY_P18			1000	// 1ms
#define DELAY_P19			1		// 25ns
#define DELAY_P20			25000	// 25ms
#define DELAY_P21			10		// 1us - 500us MAX!

#define ENTER_PROGRAM_KEY	0x4D434851

#define reset_pc() send_cmd(0x040200)
#define send_nop() send_cmd(0x000000)

static unsigned int counter=0;
static uint16_t nvmcon;

/* Send a 24-bit command to the PIC (LSB first) through a SIX instruction */
void dspic33epxxgs50x::send_cmd(uint32_t cmd)
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

	GPIO_CLR(pic_data);
	delay_us(DELAY_P4A);

}

/* Send five NOPs (should be with a frequency greater than 2MHz...) */
inline void dspic33epxxgs50x::send_prog_nop(void)
{
	uint8_t i;

	GPIO_CLR(pic_data);

	/* send 5 NOP commands */
	for (i = 0; i < 140; i++) {
		GPIO_SET(pic_clk);
		delay_us(DELAY_P1A);
		GPIO_CLR(pic_clk);
		delay_us(DELAY_P1B);
	}
}

/* Read 16-bit data word from the PIC (LSB first) through a REGOUT inst */
uint16_t dspic33epxxgs50x::read_data(void)
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

/* enter program mode */
void dspic33epxxgs50x::enter_program_mode(void)
{
	int i;

	GPIO_IN(pic_mclr);
	GPIO_OUT(pic_mclr);

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

	/* idle for 5 clock cycles */
	for (i = 0; i < 5; i++) {
		GPIO_SET(pic_clk);
		delay_us(DELAY_P1B);
		GPIO_CLR(pic_clk);
		delay_us(DELAY_P1A);
	}

}

/* exit program mode */
void dspic33epxxgs50x::exit_program_mode(void)
{
	GPIO_CLR(pic_clk);
	GPIO_CLR(pic_data);
	delay_us(DELAY_P16);
	GPIO_CLR(pic_mclr);		/* remove VDD from MCLR pin */
	delay_us(DELAY_P17);	/* wait (at least) P17 */
	GPIO_SET(pic_mclr);
	GPIO_IN(pic_mclr);
}

/* read the device ID and revision; returns only the id */
bool dspic33epxxgs50x::read_device_id(void)
{
	bool found = 0;

	send_nop();
	send_nop();
	send_nop();
	reset_pc();
	send_nop();
	send_nop();
	send_nop();

	send_cmd(0x200FF0);
	send_cmd(0x20F887);
	send_cmd(0x8802A0);
	send_cmd(0x200006);
	send_nop();

	send_cmd(0xBA8B96);
	send_nop();
	send_nop();
	send_nop();
	send_nop();
	send_nop();
	device_id = read_data();

	send_cmd(0xBA0B96);
	send_nop();
	send_nop();
	send_nop();
	send_nop();
	send_nop();

	device_id = read_data();

	reset_pc();
	send_nop();

	for (unsigned short i=0; i < sizeof(piclist)/sizeof(piclist[0]); i++) {

		if (piclist[i].device_id == device_id){

			strcpy(name, piclist[i].name);
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

/* Check if the device is blank */
uint8_t dspic33epxxgs50x::blank_check(void)
{
	uint32_t addr;
	unsigned short i;
	uint16_t data[8], raw_data[6];
	uint8_t ret = 0;

	if(!flags.debug) cerr << "[ 0%]";

	counter=0;

	/* exit reset vector */
	send_nop();
	send_nop();
	send_nop();
	reset_pc();
	send_nop();
	send_nop();
	send_nop();

	/* Output data to W0:W5; repeat until all desired code memory is read. */
	for(addr=0; addr < mem.code_memory_size; addr=addr+8) {

		if((addr & 0x0000FFFF) == 0){
			send_cmd(0x200000 | ((addr & 0x00FF0000) >> 12) );	// MOV #<DestAddress23:16>, W0
			send_cmd(0x8802A0);									// MOV W0, TBLPAG
			send_cmd(0x200006 | ((addr & 0x0000FFFF) << 4) );	// MOV #<DestAddress15:0>, W6
		}

		/* Fetch the next four memory locations and put them to W0:W5 */
		send_cmd(0xEB0380);	// CLR W7
		send_nop();
		send_cmd(0xBA1B96);
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_cmd(0xBADBB6);
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_cmd(0xBADBD6);
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_cmd(0xBA1BB6);
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_cmd(0xBA1B96);
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_cmd(0xBADBB6);
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_cmd(0xBADBD6);
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_cmd(0xBA0BB6);
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_nop();

		/* read six data words (16 bits each) */
		for(i=0; i<6; i++){
			send_cmd(0x887C40 + i);
			send_nop();
			raw_data[i] = read_data();
			send_nop();
		}

		send_nop();
		send_nop();
		send_nop();
		reset_pc();
		send_nop();
		send_nop();
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

		if(counter != addr*100/mem.code_memory_size){
			counter = addr*100/mem.code_memory_size;
			fprintf(stderr, "\b\b\b\b\b[%2d%%]", counter);
		}

		for(i=0; i<8; i++){
			if(flags.debug)
				fprintf(stderr, "\n addr = 0x%06X data = 0x%04X",
								(addr+i), data[i]);
			if ((i%2 == 0 && data[i] != 0xFFFF) || (i%2 == 1 && data[i] != 0x00FF)) {
				if(!flags.debug) cerr << "\b\b\b\b\b";
				ret = 1;
				addr = mem.code_memory_size + 10;
				break;
			}
		}
	}

	if(addr <= (mem.code_memory_size + 8)){
		if(!flags.debug) cerr << "\b\b\b\b\b";
		ret = 0;
	};

	send_nop();
	send_nop();
	send_nop();
	reset_pc();
	send_nop();
	send_nop();
	send_nop();

	return ret;
}

/* Bulk erase the chip */
void dspic33epxxgs50x::bulk_erase(void)
{
	if(flags.debug) cerr << "Erasing memory";

    send_nop();
    send_nop();
    send_nop();
    reset_pc();
    send_nop();
    send_nop();
    send_nop();

	send_cmd(0x2400EA);
	send_cmd(0x88394A);
	send_nop();
	send_nop();

	send_cmd(0x200551);
	send_cmd(0x883971);
	send_cmd(0x200AA1);
	send_cmd(0x883971);
	send_cmd(0xA8E729);
	send_nop();
	send_nop();
	send_nop();

	delay_us(DELAY_P11);

	/* wait while the erase operation completes */
	do{
		send_nop();
		send_cmd(0x803940);
		send_nop();
		send_cmd(0x887C40);
		send_nop();
		nvmcon = read_data();
		send_nop();
		send_nop();
		send_nop();
		reset_pc();
		send_nop();
		send_nop();
		send_nop();
	} while((nvmcon & 0x8000) == 0x8000);

	if(flags.debug) cerr << "Finished erasing memory";
	if(flags.client) fprintf(stdout, "@FIN");
}

/* Read PIC memory and write the contents to a .hex file */
void dspic33epxxgs50x::read(char *outfile, uint32_t start, uint32_t count)
{
	uint32_t addr, startaddr, stopaddr;
	uint16_t data[8], raw_data[6];
	int i=0;

	startaddr = start;
	stopaddr = mem.code_memory_size;
	if(count != 0 && count < stopaddr){
		stopaddr = startaddr + count;
		fprintf(stderr, "Read only %d memory locations, from %06X to %06X\n",
				count, startaddr, stopaddr);
	}

	if(!flags.debug) cerr << "[ 0%]";
	if(flags.client) fprintf(stdout, "@000");
	counter=0;

	/* exit reset vector */
	send_nop();
	send_nop();
	send_nop();
	reset_pc();
	send_nop();
	send_nop();
	send_nop();

	/* Output data to W0:W5; repeat until all desired code memory is read. */
	for(addr=startaddr; addr < stopaddr; addr=addr+8) {

		if((addr & 0x0000FFFF) == 0 || startaddr != 0){
			send_cmd(0x200000 | ((addr & 0x00FF0000) >> 12) );	// MOV #<DestAddress23:16>, W0
			send_cmd(0x8802A0);									// MOV W0, TBLPAG
			send_cmd(0x200006 | ((addr & 0x0000FFFF) << 4) );	// MOV #<DestAddress15:0>, W6
			startaddr = 0;
		}

		/* Fetch the next four memory locations and put them to W0:W5 */
		send_cmd(0xEB0380);	// CLR W7
		send_nop();
		send_cmd(0xBA1B96);
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_cmd(0xBADBB6);
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_cmd(0xBADBD6);
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_cmd(0xBA1BB6);
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_cmd(0xBA1B96);
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_cmd(0xBADBB6);
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_cmd(0xBADBD6);
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_cmd(0xBA0BB6);
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_nop();

		/* read six data words (16 bits each) */
		for(i=0; i<6; i++){
			send_cmd(0x887C40 + i);
			send_nop();
			raw_data[i] = read_data();
			send_nop();
		}

		send_nop();
		send_nop();
		send_nop();
		reset_pc();
		send_nop();
		send_nop();
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

		for(i=0; i<8; i++){
			if (flags.debug)
				fprintf(stderr, "\n addr = 0x%06X data = 0x%04X",
						(addr+i), data[i]);

			if (i%2 == 0 && data[i] != 0xFFFF) {
				mem.location[addr+i]        = data[i];
				mem.filled[addr+i]      = 1;
			}

			if (i%2 == 1 && data[i] != 0x00FF) {
				mem.location[addr+i]        = data[i];
				mem.filled[addr+i]      = 1;
			}
		}

		if(counter != addr*100/stopaddr){
			counter = addr*100/stopaddr;
			if(flags.client)
				fprintf(stdout,"@%03d", counter);
			if(!flags.debug)
				fprintf(stderr,"\b\b\b\b\b[%2d%%]", counter);
		}

		/* TODO: checksum */
	}

	send_nop();
	send_nop();
	send_nop();
	reset_pc();
	send_nop();
	send_nop();
	send_nop();

	send_cmd(0x200F80);
	send_cmd(0x8802A0);
	send_cmd(0x200046);
	send_cmd(0x20F887);
	send_nop();

	addr = 0x00F80004;

	for(i=0; i<8; i++){
		send_cmd(0xBA0BB6);
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		data[0] = read_data();
		if (data[0] != 0xFFFF) {
			mem.location[addr+2*i] = data[0];
			mem.filled[addr+2*i] = 1;
		}
	}

	send_nop();
	send_nop();
	send_nop();
	reset_pc();
	send_nop();
	send_nop();
	send_nop();

	if(!flags.debug) cerr << "\b\b\b\b\b";
	if(flags.client) fprintf(stdout, "@FIN");
	write_inhx(&mem, outfile);
}

/* Write contents of the .hex file to the PIC */
void dspic33epxxgs50x::write(char *infile)
{
	uint16_t i;
	uint16_t k;
	bool skip;
	uint32_t data[8],raw_data[6];
	uint32_t addr = 0;
	uint16_t hbyte = 0, lbyte = 0;
	uint32_t config_data = 0;

	unsigned int filled_locations=1;

	const char *regname[] = {"FSEC","FBSLIM","FOSCSEL","FOSC","FWDT","FICD", "FDEVOPT", "FALTREG"};
	const int config_addr[] = {0x005780, 0x005790, 0x005798, 0x00579C, 0x0057A0, 0x0057A8, 0x0057AC, 0x0057B0};

	filled_locations = read_inhx(infile, &mem);
	if(!filled_locations) {
		fprintf(stderr,"\n\n ERROR No filled locations!\n\n");
		exit(31);
	}

	bulk_erase();

	if(flags.debug) cerr << "Writing FBOOT register...\n";

	/* Exit reset vector */
	send_nop();
	send_nop();
	send_nop();
	reset_pc();
	send_nop();
	send_nop();
	send_nop();

	/* WRITE FBOOT REGISTER */
	addr = 0x801000;

	// Initialize the TBLPAG register for writing to the latches
	send_cmd(0x200FAC);
	send_cmd(0x8802AC);

	// Load W0:W1 with the next two Configuration Words to program.
	send_cmd(0x200000 | ((mem.location[addr] & 0x0000FFFF) << 4));
	send_cmd(0x200001 | ((mem.location[addr] & 0x00FF0000) >> 12) );

	// Set the Write Pointer (W3) and load the write latches
	send_cmd(0xEB0030);
	send_nop();
	send_cmd(0xBB0B00);
	send_nop();
	send_nop();
	send_cmd(0xBB9B01);
	send_nop();
	send_nop();

	// Set the NVMCON register to program FBOOT.
	send_cmd(0xA31000);
	send_cmd(0xB08000);
	send_cmd(0xDD004E);
	send_cmd(0x700068);
	send_cmd(0x883940);
	send_nop();
	send_nop();

	// Initiate the write cycle.
	send_cmd(0x200551);
	send_cmd(0x883971);
	send_cmd(0x200AA1);
	send_cmd(0x883971);
	send_cmd(0xA8E729);
	send_nop();
	send_nop();
	send_nop();
	send_nop();
	send_nop();

	//  Wait for program operation to complete and make sure the WR bit is clear.
	do{
		send_nop();
		send_cmd(0x803940);
		send_nop();
		send_cmd(0x887C40);
		send_nop();
		nvmcon = read_data();
		send_nop();
		send_nop();
		send_nop();
		reset_pc();
		send_nop();
		send_nop();
		send_nop();
	} while((nvmcon & 0x8000) == 0x8000);

	if(flags.debug) cerr << "Resetting device after FBOOT write...\n";

	/***** RESET DEVICE to apply new boot table*****/
	exit_program_mode();
	delay_us(100);
	enter_program_mode();

	/****** WRITE CODE MEMORY ******/

	if(flags.debug) cerr << "Writing program memory...\n";

	/* Exit reset vector */
	send_nop();
	send_nop();
	send_nop();
	reset_pc();
	send_nop();
	send_nop();
	send_nop();

	/* Initialize the TBLPAG register for writing to the latches */
	send_cmd(0x200FAC); // MOV #0xFA, W12
	send_cmd(0x8802AC); // MOV W12, TBLPAG

	if(!flags.debug) cerr << "[ 0%]";
	if(flags.client) fprintf(stdout, "@000");
	counter=0;

	for (addr = 0; addr < mem.code_memory_size; ){

		skip = 1;

		for (k = 0; k < 4; k += 2)
			if (mem.filled[addr + k]) skip = 0;

		if (skip) {
			addr = addr + 4;
			continue;
		}

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
		send_cmd(0xBB0B96); // TBLWTL [W6++], [W7++]
		send_nop();
		send_nop();

		/* Set the NVMADRU/NVMADR register pair to point to the correct address */
		send_cmd(0x200003 | ((addr & 0x0000FFFF) << 4) ); // MOV #<DestinationAddress15:0>, W3
		send_cmd(0x200004 | ((addr & 0x00FF0000) >> 12) ); // MOV #<DestinationAddress23:16>, W4
		send_cmd(0x883953);	//MOV W3, NVMADR
		send_cmd(0x883964);	//MOV W4, NVMADRU

		/* Set the NVMCON to program two instruction words */
		send_cmd(0x24001A); // MOV #0x4001, W10
		send_nop();
		send_cmd(0x88394A); // MOV W10, NVMCON
		send_nop();
		send_nop();

		/* Initiate the write cycle. */
		send_cmd(0x200551);	// MOV #0x55, W1
		send_cmd(0x883971);	// W1, NVMKEY
		send_cmd(0x200AA1);	// MOV #0xAA, W1
		send_cmd(0x883971);	// MOV W1, NVMKEY
		send_cmd(0xA8E729);	// BSET NVMCON, #WR
		send_nop();
		send_nop();
		send_nop();

		addr = addr + 4;

		do{
			send_nop();
			send_cmd(0x803940);
			send_nop();
			send_cmd(0x887C40);
			send_nop();
			nvmcon = read_data();
			send_nop();
			send_nop();
			send_nop();
			reset_pc();
			send_nop();
			send_nop();
			send_nop();
		} while((nvmcon & 0x8000) == 0x8000);

		if(counter != addr*100/filled_locations){
			if(flags.client)
				fprintf(stdout,"@%03d", (addr*100/(filled_locations+0x100)));
			if(!flags.debug)
				fprintf(stderr,"\b\b\b\b\b[%2d%%]", addr*100/(filled_locations+0x100));
			counter = addr*100/filled_locations;
		}
	};

	if(!flags.debug) cerr << "\b\b\b\b\b\b";
	if(flags.client) fprintf(stdout, "@100");

	delay_us(100000);

	/* WRITE CONFIGURATION WORDS */
	if(flags.debug)
		cerr << endl << "Writing Configuration registers..." << endl;

	//  Exit the Reset vector.
	send_nop();
	send_nop();
	send_nop();
	reset_pc();
	send_nop();
	send_nop();
	send_nop();

	/* Initialize the TBLPAG register for writing to the latches */
	send_cmd(0x200FAC); // MOV #0xFA, W12
	send_cmd(0x8802AC); // MOV W12, TBLPAG

	for(i=0; i<8; i++)
	{
		addr = config_addr[i];

		if(mem.filled[addr]){

			/* Load W0:W1 with the next two Configuration Words to program. */
			send_cmd(0x200000 | ((0x0000FFFF & mem.location[addr]) << 4));
			send_cmd(0x200001 | ((mem.location[addr] & 0x00FF0000) >> 12) );
			send_cmd(0x2FFFF2);//send_cmd(0x200002 | ((0x0000FFFF & mem.location[addr+1]) << 4));
			send_cmd(0x2FFFF3);//send_cmd(0x200003 | ((mem.location[addr+1] & 0x00FF0000) >> 12) );

			/* Set the Write Pointer (W3) and load the write latches.*/
			send_cmd(0xEB0300);
			send_nop();
			send_cmd(0xBB0B00);
			send_nop();
			send_nop();
			send_cmd(0xBB9B01);
			send_nop();
			send_nop();
			send_cmd(0xBB0B02);
			send_nop();
			send_nop();
			send_cmd(0xBB9B03);
			send_nop();
			send_nop();

			/* Set the NVMADRU/NVMADR register pair to point to the correct Configuration Word address */
			send_cmd(0x200004 | ((addr & 0x0000FFFF) << 4) ); // MOV #<DestinationAddress15:0>, W3
			send_cmd(0x200005 | ((addr & 0x00FF0000) >> 12) ); // MOV #<DestinationAddress23:16>, W4
			send_cmd(0x883954);	//MOV W4, NVMADR
			send_cmd(0x883965);	//MOV W5, NVMADRU

			/*  Set the NVMCON register to program two instruction words.*/
			send_cmd(0x24001A); // MOV #0x4001, W10
			send_nop();
			send_cmd(0x88394A); // MOV W10, NVMCON
			send_nop();
			send_nop();

			/* Initiate the write cycle */
			send_cmd(0x200551);	// MOV #0x55, W1
			send_cmd(0x883971);	// W1, NVMKEY
			send_cmd(0x200AA1);	// MOV #0xAA, W1
			send_cmd(0x883971);	// MOV W1, NVMKEY
			send_cmd(0xA8E729);	// BSET NVMCON, #WR
			send_nop();
			send_nop();
			send_nop();
			send_nop();
			send_nop();

			/* Generate clock pulses for program operation to complete until the WR bit is clear */

			do{
				send_nop();
				send_cmd(0x803940);
				send_nop();
				send_cmd(0x887C40);
				send_nop();
				nvmcon = read_data();
				send_nop();
				send_nop();
				send_nop();
				reset_pc();
				send_nop();
				send_nop();
				send_nop();
			} while((nvmcon & 0x8000) == 0x8000);
		} else if(flags.debug)
				fprintf(stderr,"\n - %s left unchanged", regname[i]);

		addr = addr+2;
	}

	if(flags.debug) cerr << endl;

	delay_us(100000);

	/***** VERIFY CODE MEMORY *****/
	if(!flags.noverify){
		if(!flags.debug) cerr << "[ 0%]";
		if(flags.client) fprintf(stdout, "@000");
		counter = 0;

		send_nop();
		send_nop();
		send_nop();
		reset_pc();
		send_nop();
		send_nop();
		send_nop();

		for(addr=0; addr < mem.code_memory_size; addr=addr+8) {

			skip=1;

			for(k=0; k<8; k+=2)
				if(mem.filled[addr+k])
					skip = 0;

			if(skip) continue;

			send_cmd(0x200000 | ((addr & 0x00FF0000) >> 12) );	// MOV #<DestAddress23:16>, W0
			send_cmd(0x8802A0);									// MOV W0, TBLPAG
			send_cmd(0x200006 | ((addr & 0x0000FFFF) << 4) );	// MOV #<DestAddress15:0>, W6

			/* Fetch the next four memory locations and put them to W0:W5 */
			send_cmd(0xEB0380);	// CLR W7
			send_nop();
			send_cmd(0xBA1B96);
			send_nop();
			send_nop();
			send_nop();
			send_nop();
			send_nop();
			send_cmd(0xBADBB6);
			send_nop();
			send_nop();
			send_nop();
			send_nop();
			send_nop();
			send_cmd(0xBADBD6);
			send_nop();
			send_nop();
			send_nop();
			send_nop();
			send_nop();
			send_cmd(0xBA1BB6);
			send_nop();
			send_nop();
			send_nop();
			send_nop();
			send_nop();
			send_cmd(0xBA1B96);
			send_nop();
			send_nop();
			send_nop();
			send_nop();
			send_nop();
			send_cmd(0xBADBB6);
			send_nop();
			send_nop();
			send_nop();
			send_nop();
			send_nop();
			send_cmd(0xBADBD6);
			send_nop();
			send_nop();
			send_nop();
			send_nop();
			send_nop();
			send_cmd(0xBA0BB6);
			send_nop();
			send_nop();
			send_nop();
			send_nop();
			send_nop();

			/* read six data words (16 bits each) */
			for(i=0; i<6; i++){
				send_cmd(0x887C40 + i);
				send_nop();
				raw_data[i] = read_data();
				send_nop();
			}

			send_nop();
			send_nop();
			send_nop();
			reset_pc();
			send_nop();
			send_nop();
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

			for(i=0; i<8; i++){
				if (flags.debug)
					fprintf(stderr, "\n addr = 0x%06X data = 0x%04X", (addr+i), data[i]);

				if(mem.filled[addr+i] && data[i] != mem.location[addr+i]){
					fprintf(stderr,"\n\n ERROR at address %06X: written %04X but %04X read!\n\n",
									addr+i, mem.location[addr+i], data[i]);
					exit(32);
				}

			}

			if(counter != addr*100/filled_locations){
				if(flags.client)
					fprintf(stdout,"@%03d", (addr*100/(filled_locations+0x100)));
				if(!flags.debug)
					fprintf(stderr,"\b\b\b\b\b[%2d%%]", addr*100/(filled_locations+0x100));
				counter = addr*100/filled_locations;
			}
		}

		/***** VERIFY CONFIGURATION WORDS *****/
		for(unsigned short i=0; i<8; i++)
		{
			send_nop();
			send_nop();
			send_nop();
			reset_pc();
			send_nop();
			send_nop();
			send_nop();

			send_cmd(0x200000 | ((config_addr[i] & 0x00FF0000) >> 12) );
			send_cmd(0x20F887);
			send_cmd(0x8802A0);
			send_cmd(0x200006 | ((0x0000FFFF & config_addr[i]) << 4));
			send_nop();

			send_cmd(0xBA8B96);
			send_nop();
			send_nop();
			send_nop();
			send_nop();
			send_nop();
			hbyte = read_data();

			send_cmd(0xBA0B96);
			send_nop();
			send_nop();
			send_nop();
			send_nop();
			send_nop();
			lbyte = read_data();

			config_data = (hbyte << 16) | lbyte;

			if(flags.debug)
				fprintf(stderr,"\n - %s: 0x%02x", regname[i], (hbyte << 16)|lbyte);

			if(mem.filled[config_addr[i]] && config_data != mem.location[config_addr[i]])
			{
				fprintf(stderr,"\n\n ERROR at config address %06X: written %04X but %04X read!\n\n",
								config_addr[i], mem.location[config_addr[i]], config_data);
				exit(33);
			}
		}

		if(!flags.debug) cerr << "\b\b\b\b\b";
		if(flags.client) fprintf(stdout, "@FIN");
	}
	else{
		if(flags.client) fprintf(stdout, "@FIN");
	}
}

/* write to screen the configuration registers, without saving them anywhere */
void dspic33epxxgs50x::dump_configuration_registers(void)
{
	const char *regname[] = {"FSEC","FBSLIM","FOSCSEL","FOSC","FWDT","FICD", "FDEVOPT", "FALTREG"};
	const int config_addr[] = {0x005780, 0x005790, 0x005798, 0x00579C, 0x0057A0, 0x0057A8, 0x0057AC, 0x0057B0};
	uint16_t hbyte = 0, lbyte = 0;

	cerr << endl << "Configuration registers:" << endl << endl;

	for(unsigned short i=0; i<8; i++)
	{

		send_nop();
		send_nop();
		send_nop();
		reset_pc();
		send_nop();
		send_nop();
		send_nop();

		send_cmd(0x200000 | ((config_addr[i] & 0x00FF0000) >> 12) );
		send_cmd(0x20F887);
		send_cmd(0x8802A0);
		send_cmd(0x200006 | ((0x0000FFFF & config_addr[i]) << 4));
		send_nop();

		send_cmd(0xBA8B96);
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		hbyte = read_data();

		send_cmd(0xBA0B96);
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		lbyte = read_data();

		fprintf(stderr," - %s: 0x%02x\n", regname[i], (hbyte << 16)|lbyte);
	}

	cerr << endl;
}
