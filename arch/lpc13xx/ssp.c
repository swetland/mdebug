#include <fw/io.h>
#include <fw/lib.h>
#include <fw/types.h>

#include <arch/cpu.h>
#include <arch/hardware.h>
#include <arch/interrupts.h>
#include <arch/ssp.h>

void printu(const char *fmt, ...);

struct spi_cfg {
	unsigned addr;
	unsigned rst;
	unsigned clk;
	unsigned div;
	unsigned irq;
};

struct spi_dev {
	const struct spi_cfg *cfg;

	u8	bits;

	void	*in_buf;
	u16	in_len;
	u16	in_pos;

	void	*out_buf;
	u16	out_len;
	u16	out_pos;

	void (*cb)(void *data);
	void	*cb_data;
};

const struct spi_cfg spi_cfgs[] = {
	[0] = {
		.addr = 0x40040000,
		.rst = SSP0_RST_N,
		.clk = SYS_CLK_SSP0,
		.div = SYS_DIV_SSP0,
		.irq = v_ssp0,
	},
	[1] = {
		.addr = 0x40058000,
		.rst = SSP1_RST_N,
		.clk = SYS_CLK_SSP1,
		.div = SYS_DIV_SSP1,
		.irq = v_ssp1,
	},
};

struct spi_dev spi_devs[] = {
	[0] = {.cfg = &spi_cfgs[0]},
	[1] = {.cfg = &spi_cfgs[1]},
};

static inline unsigned spi_readl(struct spi_dev *spi, unsigned reg)
{
	return readl(spi->cfg->addr + reg);
}

static inline void spi_writel(struct spi_dev *spi, unsigned val, unsigned reg)
{
	writel(val, spi->cfg->addr + reg);
}

static inline void spi_clr_set_reg(struct spi_dev *spi, unsigned reg,
				   unsigned clr, unsigned set)
{
	clr_set_reg(spi->cfg->addr + reg, clr, set);
}

void handle_spi_xmit(struct spi_dev *spi)
{
	u16 data;

	/*
	 * Drain the rx fifo. Make sure that we read as many words as we wrote.
	 * If the in_buf len is less that out_len, throw away the word.
	 */
	while(spi->in_pos < spi->out_len &&
	      (spi_readl(spi, SSP_SR) & SSP_SR_RNE)) {
		data = spi_readl(spi, SSP_DR);

		if (spi->in_pos < spi->in_len) {
			if (spi->bits == 8)
				((u8 *)spi->in_buf)[spi->in_pos] = data;
			else
				((u16 *)spi->in_buf)[spi->in_pos] = data;
		}
		spi->in_pos++;
	}

	if (spi->in_pos < spi->out_len)
		spi_clr_set_reg(spi, SSP_IMSC, 0, SSP_INT_RT | SSP_INT_RX);
	else
		spi_clr_set_reg(spi, SSP_IMSC, SSP_INT_RT | SSP_INT_RX, 0);

	/* fill the TX fifo */
	while(spi->out_pos < spi->out_len &&
	      (spi_readl(spi, SSP_SR) & SSP_SR_TNF)) {
		if (spi->bits == 8)
			data = ((u8 *)spi->out_buf)[spi->out_pos];
		else
			data = ((u16 *)spi->out_buf)[spi->out_pos];
		spi_writel(spi, data, SSP_DR);
		spi->out_pos++;
	}

	if (spi->in_pos < spi->out_len)
		spi_clr_set_reg(spi, SSP_IMSC, 0, SSP_INT_TX);
	else
		spi_clr_set_reg(spi, SSP_IMSC, SSP_INT_TX, 0);

	if (spi->in_pos < spi->out_len && spi->cb)
		spi->cb(spi->cb_data);
}

void handle_spi_irq(struct spi_dev *spi)
{
	unsigned status = spi_readl(spi, SSP_RIS);

	handle_spi_xmit(spi);

	spi_writel(spi, status & 0x3, SSP_RIS);
}

void handle_irq_ssp0(void)
{
	handle_spi_irq(&spi_devs[0]);
}

void handle_irq_ssp1(void)
{
	handle_spi_irq(&spi_devs[1]);
}

struct spi_dev *ssp_init(unsigned n, int bits, bool cpol, bool cpha,
				 int prescale, int clkdiv, int rate)
{
	struct spi_dev *spi;
	const struct spi_cfg *cfg;
	if (n > ARRAY_SIZE(spi_devs))
		return NULL;

	spi = &spi_devs[n];
	cfg = spi->cfg;
	spi->bits = bits;

	irq_enable(cfg->irq);

	/* assert reset, disable clock */
	writel(readl(PRESETCTRL) & ~cfg->rst, PRESETCTRL);
	writel(0, cfg->div);

	/* enable SSP0 clock */
	writel(readl(SYS_CLK_CTRL) | cfg->clk, SYS_CLK_CTRL);

	/* SSP0 PCLK = SYSCLK / 3 (16MHz) */
	writel(clkdiv, cfg->div);

	/* deassert reset */
	writel(readl(PRESETCTRL) | cfg->rst, PRESETCTRL);

	spi_writel(spi, prescale, SSP_CPSR); /* prescale = PCLK/2 */
	spi_writel(spi, SSP_CR0_BITS(bits) | SSP_CR0_FRAME_SPI |
		   (cpol ? SSP_CR0_CPOL : 0) |
		   (cpha ? SSP_CR0_CPHA : 0) |
		   SSP_CR0_CLOCK_RATE(rate),
		   SSP_CR0);
        spi_writel(spi, SSP_CR1_ENABLE | SSP_CR1_MASTER, SSP_CR1);

	return spi;
}

void spi_xmit(struct spi_dev *spi, void *out_buf, int out_len,
	      void *in_buf, int in_len,
	      void (*cb)(void *data), void *cb_data)
{
	/* wait for spi to idle */
	while (spi_readl(spi, SSP_SR) & SSP_SR_BSY) {
	}

	spi->in_buf = in_buf;
	spi->in_len = in_len;
	spi->in_pos = 0;

	spi->out_buf = out_buf;
	spi->out_len = out_len;
	spi->out_pos = 0;

	spi->cb = cb;
	spi->cb_data = cb_data;

	disable_interrupts();
	handle_spi_xmit(spi);
	enable_interrupts();
}
