/*

 USB CDC echo demo on pic18f24j50

 (C)hris2011 for dangerousprototypes.com

 */

// includes
#include "config.h"
#include <HardwareProfile.h>
#include <p18cxxx.h>
#include <GenericTypeDefs.h>
#include <usb_config.h>
#include <usb_stack.h>
#include <cdc.h>
#include <cdc_descriptors.h>
#include <delays.h>

#include "spi.h"
#include "ols.h"

// defines
static void cmd_version(unsigned char *args);
static void cmd_id(unsigned char *args);
static void cmd_write(unsigned char *args);
static void cmd_read(unsigned char *args);
static void cmd_erase(unsigned char *args);
static void cmd_status(unsigned char *args);
static void cmd_getselftest(unsigned char *args);
static void cmd_doselftest(unsigned char *args);
static void cmd_bootloader(unsigned char *args);
static void cmd_normal(unsigned char *args);
static void cmd_echo(unsigned char *args);

#pragma udata
static unsigned char selftestError = 0;
static unsigned char FLASH_algo = FLASH_ATMEL264;
static unsigned short FLASH_pagesize = 264;

#define ARY_SIZE(x) ((sizeof(x)/sizeof(x[0])))

struct {
	unsigned char id;
	void (*fnc)(unsigned char *);
} commands[] = {
	{ 0x00, cmd_version},
	{ 0x01, cmd_id},
	{ 0x02, cmd_write},
	{ 0x03, cmd_read},
	{ 0x04, cmd_erase},
	{ 0x05, cmd_status},
	{ 0x06, cmd_getselftest},
	{ 0x07, cmd_doselftest},
	{ '$', cmd_bootloader},
	{ 'a', cmd_echo},
	{ 0xff, cmd_normal},
};


// forward declarations
void init(void);
void delayms(unsigned int i);
unsigned char HardwareSelftest(void);

#pragma code

void main(void)
{
	unsigned char cmd[4];

	init();

	initCDC();
	usb_init(cdc_device_descriptor, cdc_config_descriptor, cdc_str_descs, USB_NUM_STRINGS); // TODO: Remove magic with macro
	usb_start();
#if defined (USB_INTERRUPTS)
	//EnableUsbInterrupt(USB_TRN + USB_SOF + USB_UERR + USB_URST);
	EnableUsbInterrupt(USB_STALL + USB_IDLE + USB_TRN + USB_ACTIV + USB_SOF + USB_UERR + USB_URST);
	INTCONbits.PEIE = 1;
	INTCONbits.GIE = 1;
	EnableUsbInterrupts();
#endif
	usbbufflush(); //flush USB input buffer system

	do {
#ifndef USB_INTERRUPTS
		//service USB tasks
		//Guaranteed one pass in polling mode
		//even when usb_device_state == CONFIGURED_STATE
		if (!TestUsbInterruptEnabled()) {
			USBDeviceTasks();
		}
#endif

		if ((usb_device_state < DEFAULT_STATE)) { // JTR2 no suspendControl available yet || (USBSuspendControl==1) ){

		} else if (usb_device_state < CONFIGURED_STATE) {

		} else if ((usb_device_state == CONFIGURED_STATE)) {

		}
	} while (usb_device_state < CONFIGURED_STATE);

	usbbufflush();


	if (PIN_UPDATE == 1) {
		selftestError = HardwareSelftest();
	}

	selftestError = 1; // TODO: remove when fpgaloop complete
	if ((selftestError != 0) || (PIN_UPDATE == 0)) {
		//return FPGA to reset
		PROG_B_LOW();

		PIN_LED = 1; //LED on in update mode
		setupROMSPI(); //setup SPI to program ROM
	} else {
		PIN_LED = 0; //LED off
		setupFPGASPImaster(); //setup SPI to FPGA
		//TODO:fpgaloop();
	}
	 
	while (1) {
#ifndef USB_INTERRUPTS
		if (!TestUsbInterruptEnabled())
			USBDeviceTasks();
#endif
		if ((usbbufavailable() >= 4) && usbbufgetarray(cmd, 4)) {
			int i;

			for (i = 0; i < ARY_SIZE(commands); i++) {
				if (commands[i].id == cmd[0]) {
					commands[i].fnc(&cmd[1]);
					break;
				}
			}
		}

	}
}

static void cmd_echo(unsigned char *arg)
{
	unsigned short len;
	unsigned short i;
	unsigned short j;
	unsigned char ch;

	len = arg[2];

	while (len > 0) {
		j = len;
		if (j > 64)
			j = 64;

		ch = 'j';

		WaitInReady();

		for (i = 0; i < j; i++) {
			usbbufgetbyte_block(&ch);
			cdc_In_buffer[i] = ch;
		}

		len = len - j;

		putUnsignedCharArrayUsbUsart(cdc_In_buffer, j);

	}
}

static void cmd_version(unsigned char *arg)
{
	WaitInReady();

	cdc_In_buffer[0] = 'H';
	cdc_In_buffer[1] = HW_VER;
	cdc_In_buffer[2] = 'F';
	cdc_In_buffer[3] = FW_VER_H;
	cdc_In_buffer[4] = FW_VER_L;
	cdc_In_buffer[5] = 'B';
	//read 0x7fe for the bootloader version string
	TBLPTRU = 0;
	TBLPTRH = 0x7;
	TBLPTRL = 0xfe;
	_asm TBLRDPOSTINC _endasm; //read into TABLAT and increment
	cdc_In_buffer[6] = TABLAT;

	putUnsignedCharArrayUsbUsart(cdc_In_buffer, 7);
}

static void cmd_id(unsigned char *args)
{
	WaitInReady();

	PIN_FLASH_CS = 0; //CS low
	spi(0x9f); //get ID command
	cdc_In_buffer[0] = spi(0xff);
	cdc_In_buffer[1] = spi(0xff);
	cdc_In_buffer[2] = spi(0xff);
	cdc_In_buffer[3] = spi(0xff);
	PIN_FLASH_CS = 1;

	putUnsignedCharArrayUsbUsart(cdc_In_buffer, 4);
}

static void cmd_write(unsigned char *args)
{
	unsigned char crc = 0;
	unsigned char ch;
	unsigned short i;

	unsigned char status = CMD_ERROR;

	if (FLASH_algo == FLASH_ATMEL264) {
		PIN_FLASH_CS = 0; //CS low
		spi(0x84); //write page buffer 1 command
		spi(0); //send buffer address
		spi(0);
		spi(0);
	} else if (FLASH_algo == FLASH_WINBOND256) {
		PIN_FLASH_CS = 0; //CS low
		spi(0x06); //write enable
		PIN_FLASH_CS = 1; //CS high
		Nop();
		PIN_FLASH_CS = 0; //CS low
		spi(0x02); //write page command
		spi(args[0]); //send address
		spi(args[1]);
		spi(args[2]);
	}

	crc = 0;
	for (i = 0; i < FLASH_pagesize; i++) {
		usbbufgetbyte_block(&ch);
		crc -= ch;

		spi(ch);
	}
	PIN_FLASH_CS = 1;
	
	usbbufgetbyte_block(&ch);

	if (ch == crc) {
		if (FLASH_algo == FLASH_ATMEL264) {
			//slight delay
			PIN_FLASH_CS = 0;
			spi(0x83); //save buffer to memory command
			spi(args[0]); //send write page address
			spi(args[1]);
			spi(args[2]);
			PIN_FLASH_CS = 1; //disable CS

			while (1) {
				delayms(1);
				PIN_FLASH_CS = 0;
				spi(0xd7); //read status command
				i = spi(0xff);
				if ((i & 0b10000000) != 0) {
					status = CMD_SUCESS;
					break;
				}
				PIN_FLASH_CS = 1;
			}
		} else if (FLASH_algo == FLASH_WINBOND256) {
			while (1) {
				delayms(1);
				PIN_FLASH_CS = 0;
				spi(0x05); //read status command
				i = spi(0xff);
				if ((i & 0b1) == 0) {
					status = CMD_SUCESS;
					break;
				}
				PIN_FLASH_CS = 1;
			}
		}
	}
	PIN_FLASH_CS = 1;

	WaitInReady();
	cdc_In_buffer[0] = status;

	putUnsignedCharArrayUsbUsart(cdc_In_buffer, 1);
}

static void cmd_read(unsigned char *args)
{
	unsigned int read = FLASH_pagesize;
	unsigned int i;
	unsigned int j;

	PIN_FLASH_CS = 0; //CS low
	spi(0x03); //read array command
	spi(args[0]); //send read address
	spi(args[1]);
	spi(args[2]);

	while (read) {
		j = read;
		if (j > 64)
			j = 64;

		WaitInReady();
		for (i = 0; i < j; i++) {
			cdc_In_buffer[i] = spi(0xff);
		}

		putUnsignedCharArrayUsbUsart(cdc_In_buffer, j);
		read -= j;
	}
	PIN_FLASH_CS = 1;
}

static void cmd_erase(unsigned char *args)
{
	unsigned int i;

	WaitInReady();
	cdc_In_buffer[0] = CMD_ERROR;

	if (FLASH_algo == FLASH_ATMEL264) {
		PIN_FLASH_CS = 0; //CS low
		spi(0xc7); //erase code, takes up to 12 seconds!
		spi(0x94); //erase code, takes up to 12 seconds!
		spi(0x80); //erase code, takes up to 12 seconds!
		spi(0x9a); //erase code, takes up to 12 seconds!
		PIN_FLASH_CS = 1; //CS high

		while (1) {
			delayms(1);
			PIN_FLASH_CS = 0;
			spi(0xd7); //read status command
			i = spi(0xff);
			if ((i & 0b10000000) != 0) {
				cdc_In_buffer[0] = CMD_SUCESS;
				break;
			}
			PIN_FLASH_CS = 1;

		}

	} else if (FLASH_algo == FLASH_WINBOND256) {
		PIN_FLASH_CS = 0; //CS low
		spi(0x06); //write enable
		PIN_FLASH_CS = 1; //CS high
		Nop();
		PIN_FLASH_CS = 0; //CS low
		spi(0xc7); //erase code
		PIN_FLASH_CS = 1; //CS high
		while (1) {
			delayms(1);
			PIN_FLASH_CS = 0;
			spi(0x05); //read status command
			i = spi(0xff);
			if ((i & 0b1) == 0) {
				cdc_In_buffer[0] = CMD_SUCESS;
				break;
			}
			PIN_FLASH_CS = 1;
		}
		PIN_FLASH_CS = 1;

	}
	PIN_FLASH_CS = 1;

	putUnsignedCharArrayUsbUsart(cdc_In_buffer, 1);
}

static void cmd_status(unsigned char *args)
{
	WaitInReady();

	PIN_FLASH_CS = 0;
	if (FLASH_algo == FLASH_ATMEL264) {
		spi(0xd7); //read status command
	} else if (FLASH_algo == FLASH_WINBOND256) {
		spi(0x05); //read status command
	}
	cdc_In_buffer[0] = spi(0xff);
	PIN_FLASH_CS = 1;

	putUnsignedCharArrayUsbUsart(cdc_In_buffer, 1);
}

static void cmd_getselftest(unsigned char *args)
{
	WaitInReady();
	cdc_In_buffer[0] = selftestError;
	putUnsignedCharArrayUsbUsart(cdc_In_buffer, 1);
}

static void cmd_doselftest(unsigned char *args)
{
	selftestError = HardwareSelftest();
	setupROMSPI(); //restore the ROM SPI setting after the self test

	cmd_getselftest(args);
}

static void cmd_bootloader(unsigned char *args)
{
	usb_deinit();
	
//FIXME: Y U no work?!
	BootloaderJump();
}

static void cmd_normal(unsigned char *args)
{
	PIN_LED = 0;
	teardownROMSPI();
	_asm RESET _endasm;
	while (1);
}

void init(void)
{
	// no analogue
	ANCON0 = 0xFF;
	ANCON1 = 0x1F;

	// everything is input for now
	TRISA = 0xFF;
	TRISB = 0xFF;
	TRISC = 0xFB; // led output

	PROG_B_LOW();

	TRIS_FPGA_CS = 0;
	PIN_FPGA_CS = 1;

	// wait for PLL lock
	//on 18f24j50 we must manually enable PLL and wait at least 2ms for a lock
	OSCTUNEbits.PLLEN = 1; //enable PLL
	delayms(5);
}

unsigned char testROMtype(void)
{
	unsigned char buf[4];

	setupROMSPI(); //setup SPI to program ROM

	//read out JEDEC ID
	PIN_FLASH_CS = 0; //CS low
	spi(0x9f); //get ID command
	buf[0] = spi(0xff);
	buf[1] = spi(0xff);
	buf[2] = spi(0xff);
	buf[3] = spi(0xff);
	PIN_FLASH_CS = 1;

	teardownROMSPI();

	switch (buf[0]) {
	case 0x1f://ATMEL JEDEC
		FLASH_algo = FLASH_ATMEL264;
		FLASH_pagesize = 264;
		break;
	case 0xef:
		FLASH_algo = FLASH_WINBOND256;
		FLASH_pagesize = 256;
		break;
	default:
		return ERROR_ROMID;
	}
	return 0;

}

unsigned char HardwareSelftest(void)
{
	unsigned long timer;
#define longDelay(x) timer=x; while(timer--)
	unsigned char err = 0, i;

	//ensure FPGA in reset
	PROG_B_LOW();

	err |= testROMtype();

	//release PROG_B and wait
	//this lets the FPGA start loading
	//and allows 2.5volts onto AN3 for measurement
	PROG_B_HIGH();
	longDelay(0x20000); //delay for pull-up resistor to do its thing

	//check update button, should be high
	if (PIN_UPDATE == 0) {
		err |= ERROR_UPDATE;
	}

	//PROG_B, FPGA reset pin, should be high
	if (PIN_PROG_B == 0) {
		err |= ERROR_PROGB;
	}

#ifdef VREG_TEST
	//this test measures the 2.5volt power supply on AN3
	//AN3 is used to pull PROG_B low to put the FPGA in programmng mode
	//PROG_B is held high by 2.5volts through R5

	//these are for debug
	//enable them to ground PROG_B
	//tests that the ADC detects 0volts and catches the error
	//PIN_PROG_B=0;
	//TRIS_PROG_B=0;

	ANCON0 &= (~0b1000); //clear AN3 to make analog
	ADCON0 = 0b00001100; //setup ADC to no reference, AN3, off (pg 341)
	ADCON1 = 0b10111110; //setup ADC for max time, max divider (pg 342)
	ADCON0 |= 0b1; //turn on ADC

	longDelay(0x20000);

	ADCON0 |= 0b10; //start conversion
	while (ADCON0 & 0b10); //wait for conversion to finish

	ANCON0 = 0xff; //all pins back to digital
	ADCON0 = 0x00; //disable ADC

	if (ADRES < ADC_2V5_LIMIT) { //is votlage within 10%
		err |= ERROR_2V5;
	}
#endif

	//FPGA signals that it is loaded and ready
	// by letting DONE go high (pullup resistor R6 to 3v3)
	//Wait for DONE to go high
	//Blink LED
	//If DONE doesn't go high after 255 blinks,
	// jump to upgrade mode (ROM not programmed or FPGA problem)
	i = 0;
	while (PIN_DONE == 0) {

		//this is a delay, but it helps keep the USB connection alive
		longDelay(0x3fff);
		PIN_LED ^= 1;
		i++;
		if (i == 20) { //shortened delay
			err |= ERROR_DONE;
			break;
		}
	}

	PIN_LED = 1; //LED back on at the endof the self-test

	return err;

}

void delayms(unsigned int i)
{
	while (i) {
		Delay100TCYx(120);
		i--;
	}
}

#pragma interruptlow InterruptHandlerLow nosave= PROD, PCLATH, PCLATU, TBLPTR, TBLPTRU, TABLAT, section (".tmpdata"), section("MATH_DATA")

void InterruptHandlerLow(void)
{
	usb_handler();
	ClearGlobalUsbInterruptFlag();
}

#pragma interrupt InterruptHandlerHigh nosave= PROD, PCLATH, PCLATU, TBLPTR, TBLPTRU, TABLAT, section (".tmpdata"), section("MATH_DATA")

void InterruptHandlerHigh(void)
{
	usb_handler();
	ClearGlobalUsbInterruptFlag();
}

extern void _startup(void);
#pragma code REMAPPED_RESET_VECTOR = 0x800

void _reset(void)
{
	_asm goto _startup _endasm
}

#pragma code REMAPPED_HIGH_INTERRUPT_VECTOR = 0x808

void Remapped_High_ISR(void)
{
	_asm goto InterruptHandlerHigh _endasm
}

#pragma code REMAPPED_LOW_INTERRUPT_VECTOR = 0x818

void Remapped_Low_ISR(void)
{
	_asm goto InterruptHandlerLow _endasm
}
