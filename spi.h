#ifndef SPI_H_
#define SPI_H_

unsigned char spi(unsigned char c);
void setupFPGASPImaster(void);
void setupROMSPI(void);
void teardownROMSPI(void);


#endif