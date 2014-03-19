#ifndef _ARCH_LCP13XX_SSP_H_
#define _ARCH_LCP13XX_SSP_H_

struct spi_dev;

#define SPI_CPOL_0	0
#define SPI_CPOL_1	1

#define SPI_CPHA_0	0
#define SPI_CPHA_1	1

struct spi_dev *ssp_init(unsigned n, int bits, bool cpol, bool cpha,
			 int prescale, int clkdiv, int rate);
void spi_xmit(struct spi_dev *spi, void *out_buf, int out_len,
	      void *in_buf, int in_len,
	      void (*cb)(void *data), void *cb_data);


#endif /* _ARCH_LCP13XX_SSP_H_ */
