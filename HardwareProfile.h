/*
 *
 *	Open Logic Sniffer firmware v1.1
 *	http://www.gadgetfactory.net
 *	http://dangerousprototypes.com
 *	License: GPL
 *	Copyright Ian Lesnet 2010
 *
 */
#ifndef HARDWARE_PROFILE_H
#define HARDWARE_PROFILE_H

#include <p18cxxx.h>

//the first batch of OLS has 20mhz crystal on the PIC
//use this to compile for that hardware
//#define XTAL_20MHZ

//hardware version string
#define HW_VER   1

//firmware version string
#define FW_VER_H 4
#define FW_VER_L 0

//Define to enable manufacturing hardware 
//self-test with jumper check on UART header.
//If there is a jumper between RX and TX
//the firmware will light the ACT LED and send 0x03 
//to the FPGA through the UART. THis triggers a test mode
//in special bitstream builds.
#define SELF_TEST	

//measure the voltage on PROG_B (AN3/RA3), should be 2.5volts
//#define VREG_TEST

//calculate the ADC value to test for
//#define ADC_2V5 ((25/33)*1024) //2.5volts/3.3volts * 1024 = ADC reading (775.75).
//#define ADC_2V5_LIMIT (ADC_2V5-(ADC_2V5/10)) //show error if not within 10% 775-78=697
#define ADC_2V5_LIMIT 697

//pin setup
/*
A0 PR0 flash_si
A1 PR1 flash_so
A2 flash_cs
A3 PROG_B (pull-down only) test high
A5 RP2 DONE (input only) test high, eventually
C1 RP12 update button (input/interrupt) test high
C2 RP13 ACT LED

B7 RP10 PGD
B6 RP9 PGC
B5 RP8 flash_sck
B4 RP7 FPGA_AUX0
B3 RP6 fpga_cs
B2 RP5 fpga_aux1
B1 RP4 FPGA_AUX2
B0 RP3 FPGA_AUX3
C7 RP18 RX1
C6 RP17 TX1
 */
#define PIN_PROG_B      PORTAbits.RA3
#define TRIS_PROG_B     TRISAbits.TRISA3
#define PROG_B_LOW()    PIN_PROG_B=0; TRIS_PROG_B=0 //ground,output
#define PROG_B_HIGH()   PIN_PROG_B=0; TRIS_PROG_B=1; TRIS_FLASH_CS=1

#define PIN_DONE        PORTAbits.RA5
#define PIN_UPDATE      PORTCbits.RC1
#define PIN_LED         PORTCbits.RC2

#define TRIS_UART1_TX   TRISCbits.TRISC6
#define PIN_UART1_TX    LATCbits.LATC6
#define TRIS_UART1_RX   TRISCbits.TRISC7
#define PIN_UART1_RX    PORTCbits.RC7

#define TRIS_UART2_TX   TRISBbits.TRISB1
#define PIN_UART2_TX    PORTBbits.RB1

#define TRIS_FLASH_SCK  TRISBbits.TRISB5
#define PIN_FLASH_SCK   PORTBbits.RB5
#define TRIS_FLASH_MOSI TRISAbits.TRISA0
#define PIN_FLASH_MOSI  PORTAbits.RA0

#define TRIS_FLASH_CS   TRISAbits.TRISA2
#define PIN_FLASH_CS    PORTAbits.RA2

#define BootloaderJump() _asm goto 0x16 _endasm


#if 0
#define TRIS_FPGA_AUX0      TRISBbits.TRISB4
#define PIN_FPGA_AUX0       LATBbits.LATB4
#define TRIS_FPGA_AUX1      TRISBbits.TRISB2
#define PIN_FPGA_AUX1       PORTBbits.RB2
#define TRIS_FPGA_MOSI      TRISBbits.TRISB3
#define PIN_FPGA_MOSI       LATBbits.LATB3

#define TRIS_FPGA_DATAREADY TRISBbits.TRISB1
#define PIN_FPGA_DATAREADY  PORTBbits.RB1

#define TRIS_FPGA_CS        TRISAbits.TRISA1
#define PIN_FPGA_CS         LATAbits.LATA1
#else
#define TRIS_FPGA_DATAREADY TRISBbits.TRISB2
#define PIN_FPGA_DATAREADY  PORTBbits.RB2

#define TRIS_FPGA_CS        TRISBbits.TRISB1
#define PIN_FPGA_CS         PORTBbits.RB1
#endif

#endif 



