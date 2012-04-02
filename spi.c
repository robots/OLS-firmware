
#include "HardwareProfile.h"

#include "spi.h"

unsigned char spi(unsigned char c)
{
	SSP2BUF = c;
	while (SSP2STATbits.BF == 0);
	c = SSP2BUF;
	return c;
}

//setup the SPI connection to the FPGA
//does not yet account for protocol, etc
//no teardown needed because only exit is on hardware reset

void setupFPGASPImaster(void)
{
	//disable FLASH while we are master and FPGA is slave
	PIN_FLASH_CS = 1; //CS high
	TRIS_FLASH_CS = 0; //CS output

	//FPGA CS disabled
	PIN_FPGA_CS = 1; //CS high
	TRIS_FPGA_CS = 0; //CS output

#ifndef NEWSPI
	//this is the old SPI route between the PIC and FPGA
	//this uses the AUX pins and leaves the shared PIC->ROM->FPGA route unused
	//this is compatible with bitstream 2.12 and earlier
	//setup SPI to FPGA
	//SS AUX2 RB1/RP4
	//MISO AUX1 RB2/RP5
	//MOSI CS RB3/RP6
	//SCLK AUX0 RB4/RP7

	//set MISO input pin (master)
	TRIS_FPGA_MISO = 1; //direction input
	RPINR21 = 5; // Set SDI2 (MISO) to RP5

	//set MOSI output pin (master)
	TRIS_FPGA_MOSI = 0;
	PIN_FPGA_MOSI = 0;
	RPOR6 = 9; //PPS output

	//set SCK output pin (master)
	TRIS_FPGA_SCK = 0;
	PIN_FPGA_SCK = 0;
	RPOR7 = 10; //PPS output
#else
	//this is the 'new' SPI connection
	//it uses traces shared by the PIC, ROM, and FPGA
	//there is less configuration changes in the PIC
	//but most importantly it emulates a new project so they can share a bitstream
	//setup SPI to FPGA
	//TRIS_FLASH_MISO=1;
	RPINR21 = 1; //PPS input

	TRIS_FLASH_MOSI = 0;
	PIN_FLASH_MOSI = 0;
	RPOR0 = 9; //PPS output

	TRIS_FLASH_SCK = 0;
	PIN_FLASH_SCK = 0;
	RPOR8 = 10; //PPS output
#endif

	//settings for master mode
	//SSP2CON1=0b00100010; //SSPEN/ FOSC/64
	//SSP2CON1=0b00100001; //SSPEN/ FOSC/16
	SSP2CON1 = 0b00100000; //SSPEN/ FOSC/4
	SSP2STAT = 0b01000000; //cke=1

	//set DATAREADY input pin (master)
	TRIS_FPGA_DATAREADY = 1; //direction input
}

void setupROMSPI(void)
{
	//setup SPI for ROM
	//!!!leave unconfigured (input) except when PROG_B is held low!!!
	PROG_B_LOW();
	//A0 PR0 flash_si
	//A1 PR1 flash_so
	//B5 RP8 flash_sck
	//A2 flash_cs

	//CS disabled
	PIN_FLASH_CS = 1; //CS high
	TRIS_FLASH_CS = 0; //CS output

	//TRIS_FLASH_MISO=1;
	RPINR21 = 1; //PPS input

	TRIS_FLASH_MOSI = 0;
	PIN_FLASH_MOSI = 0;
	RPOR0 = 9; //PPS output

	TRIS_FLASH_SCK = 0;
	PIN_FLASH_SCK = 0;
	RPOR8 = 10; //PPS output

	SSP2CON1 = 0b00100000; //SSPEN/ FOSC/4 CP=0
	SSP2STAT = 0b01000000; //cke=1
}

void teardownROMSPI(void)
{
	SSP2CON1 = 0; //disable SPI
	//A0 PR0 flash_si
	//A1 PR1 flash_so
	//B5 RP8 flash_sck
	//A2 flash_cs
	//TRIS_FLASH_MISO=1;
	RPINR21 = 0b11111; //move PPS to nothing

	TRIS_FLASH_MOSI = 1;
	RPOR0 = 0;

	TRIS_FLASH_SCK = 1;
	RPOR8 = 0;

	//let FPGA control CS
	TRIS_FLASH_CS = 1; //CS input
	PIN_FLASH_CS = 0; //CS low
}
