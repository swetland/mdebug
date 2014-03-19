#ifndef _LPC13XX_HARDWARE_H_
#define _LPC13XX_HARDWARE_H_

#include <fw/io.h>

#include <arch/iocon.h>

#define IOCONSET(port, pin, func, flags) do {		\
	writel(IOCON_PIO ## port ## _ ## pin ## _FUNC_ ## func | (flags), \
	       IOCON_PIO ## port ## _ ## pin); \
} while (0)

#define SYS_CLK_CTRL		0x40048080
#define SYS_CLK_SYS		(1 << 0)
#define SYS_CLK_ROM		(1 << 1)
#define SYS_CLK_RAM		(1 << 2)
#define SYS_CLK_RAM0		(1 << 2)
#define SYS_CLK_FLASHREG	(1 << 3)
#define SYS_CLK_FLASHARRAY	(1 << 4)
#define SYS_CLK_I2C		(1 << 5)
#define SYS_CLK_GPIO		(1 << 6)
#define SYS_CLK_CT16B0		(1 << 7)
#define SYS_CLK_CT16B1		(1 << 8)
#define SYS_CLK_CT32B0		(1 << 9)
#define SYS_CLK_CT32B1		(1 << 10)
#define SYS_CLK_SSP0		(1 << 11)
#define SYS_CLK_UART		(1 << 12) /* MUST CONFIG IOCON FIRST */
#define SYS_CLK_ADC		(1 << 13)
#define SYS_CLK_USB_REG		(1 << 14)
#define SYS_CLK_WDT		(1 << 15)
#define SYS_CLK_IOCON		(1 << 16)
#define SYS_CLK_SSP1		(1 << 18)
#define SYS_CLK_PINT		(1 << 19)
#define SYS_CLK_GROUP0INT	(1 << 23)
#define SYS_CLK_GROUP1INT	(1 << 24)
#define SYS_CLK_RAM1		(1 << 26)
#define SYS_CLK_USBSRAM		(1 << 27)

#define SYS_DIV_AHB		0x40048078
#define SYS_DIV_SSP0		0x40048094 /* 0 = off, 1... = PCLK/n */
#define SYS_DIV_UART		0x40048098
#define SYS_DIV_SSP1		0x4004809C
#define SYS_DIV_TRACE		0x400480AC
#define SYS_DIV_SYSTICK		0x400480B0

#define CLKOUT_SELECT		0x400480E0
#define CLKOUT_IRC		0
#define CLKOUT_SYSTEM		1
#define CLKOUT_WATCHDOG		2
#define CLKOUT_MAIN		3
#define CLKOUT_UPDATE		0x400480E4 /* write 0, then 1 to update */

#define SCB_PINTSEL(n)		(0x40048178 + (n) * 4)
#define  SCB_PINTSEL_INTPIN(x)		((x) & 0x1f)
#define  SCB_PINTSEL_PORTSEL(x)		(((x) & 0x1) << 5)

#define PRESETCTRL		0x40048004
#define SSP0_RST_N		(1 << 0)
#define I2C_RST_N		(1 << 1)
#define SSP1_RST_N		(1 << 2)

#define SSP_CR0			0x00
#define SSP_CR1			0x04
#define SSP_DR			0x08 /* data */
#define SSP_SR			0x0C /* status */
#define SSP_CPSR		0x10 /* clock prescale 2..254 bit0=0 always */
#define SSP_IMSC		0x14 /* IRQ mask set/clear */
#define SSP_RIS			0x18 /* IRQ raw status */
#define SSP_MIS			0x1C /* IRQ masked status */
#define SSP_ICR			0x20 /* IRQ clear */

#define SSP0_CR0		0x40040000
#define SSP0_CR1		0x40040004
#define SSP0_DR			0x40040008 /* data */
#define SSP0_SR			0x4004000C /* status */
#define SSP0_CPSR		0x40040010 /* clock prescale 2..254 bit0=0 always */
#define SSP0_IMSC		0x40040014 /* IRQ mask set/clear */
#define SSP0_RIS		0x40040018 /* IRQ raw status */
#define SSP0_MIS		0x4004001C /* IRQ masked status */
#define SSP0_ICR		0x40040020 /* IRQ clear */

#define SSP_CR0_BITS(n)		((n) - 1) /* valid for n=4..16 */
#define SSP_CR0_FRAME_SPI	(0 << 4)
#define SSP_CR0_FRAME_TI	(1 << 4)
#define SSP_CR0_FRAME_MICROWIRE	(2 << 4)
#define SSP_CR0_CLK_LOW		(0 << 6) /* clock idle is low */
#define SSP_CR0_CLK_HIGH	(1 << 6) /* clock idle is high */
#define SSP_CR0_CPOL		(1 << 6)
#define SSP_CR0_PHASE0		(0 << 7) /* capture on clock change from idle */
#define SSP_CR0_PHASE1		(1 << 7) /* capture on clock change to idle */
#define SSP_CR0_CPHA		(1 << 7)
#define SSP_CR0_CLOCK_RATE(n)	(((n) - 1) << 8)

#define SSP_CR1_LOOPBACK	(1 << 0)
#define SSP_CR1_ENABLE		(1 << 1)
#define SSP_CR1_MASTER		(0 << 2)
#define SSP_CR1_SLAVE		(1 << 2)
#define SSP_CR1_OUTPUT_DISABLE	(1 << 3) /* only valid in SLAVE mode */

#define SSP_XMIT_EMPTY		(1 << 0)
#define SSP_XMIT_NOT_FULL	(1 << 1)
#define SSP_RECV_NOT_EMPTY	(1 << 2)
#define SSP_RECV_FULL		(1 << 3)
#define SSP_BUSY		(1 << 4)

#define SSP_SR_TFE		(1 << 0)
#define SSP_SR_TNF		(1 << 1)
#define SSP_SR_RNE		(1 << 2)
#define SSP_SR_RFF		(1 << 3)
#define SSP_SR_BSY		(1 << 4)

#define SSP_INT_RO		(1 << 0)
#define SSP_INT_RT		(1 << 1)
#define SSP_INT_RX		(1 << 2)
#define SSP_INT_TX		(1 << 3)

/* SSP bitrate = SYSCLK / SYS_DIV_SSPn / SSP_CR0_CLOCK_RATE */

#ifdef LPC13XX_V2
#define GPIO_ISEL		0x4004C000
#define GPIO_IENR		0x4004C004
#define GPIO_SIENR		0x4004C008
#define GPIO_CIENR		0x4004C00C
#define GPIO_IENF		0x4004C010
#define GPIO_SIENF		0x4004C014
#define GPIO_CIENF		0x4004C018
#define GPIO_RISE		0x4004C01C
#define GPIO_FALL		0x4004C020
#define GPIO_IST		0x4004C024

#define GPIO_BASE		0x50000000
#define GPIO_B(n)		(GPIO_BASE + (n))
#define GPIO_W(n)		(GPIO_BASE + 0x1000 + (n) * 4)
#define GPIO_DIR(n)		(GPIO_BASE + 0x2000 + (n) * 4)
#define GPIO_MASK(n)		(GPIO_BASE + 0x2080 + (n) * 4)
#define GPIO_PIN(n)		(GPIO_BASE + 0x2100 + (n) * 4)
#define GPIO_MPIN(n)		(GPIO_BASE + 0x2180 + (n) * 4)
#define GPIO_SET(n)		(GPIO_BASE + 0x2200 + (n) * 4)
#define GPIO_CLR(n)		(GPIO_BASE + 0x2280 + (n) * 4)
#define GPIO_NOT(n)		(GPIO_BASE + 0x2300 + (n) * 4)
#else
#define GPIOBASE(n)		(0x50000000 + ((n) << 16))
#define GPIODATA(n)		(GPIOBASE(n) + 0x3FFC)
#define GPIODIR(n)		(GPIOBASE(n) + 0x8000) /* 0 = input, 1 = output */
#define GPIOIER(n)		(GPIOBASE(n) + 0x8010) /* 1 = irq enabled */
#define GPIOLEVEL(n)		(GPIOBASE(n) + 0x8004) /* 0 = edge, 1 = level irq */
#define GPIOBOTHEDGES(n)	(GPIOBASE(n) + 0x8008) /* 1 = trigger on both edges */
#define GPIOPOLARITY(n)		(GPIOBASE(n) + 0x800C) /* 0 = low/falling, 1 = high/rising */
#define GPIORAWISR(n)		(GPIOBASE(n) + 0x8014) /* 1 if triggered */
#define GPIOISR(n)		(GPIOBASE(n) + 0x8018) /* 1 if triggered and enabled */
#define GPIOICR(n)		(GPIOBASE(n) + 0x801C) /* write 1 to clear, 2 clock delay */
#endif

/* these registers survive powerdown / warm reboot */
#define GPREG0			0x40038004
#define GPREG1			0x40038008
#define GPREG2			0x4003800C
#define GPREG3			0x40038010
#define GPREG4			0x40038014


#define TM32B0IR		0x40014000
#define TM32B0TCR		0x40014004
#define TM32B0TC		0x40014008 /* increments every PR PCLKs */
#define TM32B0PR		0x4001400C
#define TM32B0PC		0x40014010 /* increments every PCLK */
#define TM32B0MCR		0x40014014
#define TM32B0MR0		0x40014018
#define TM32B0MR1		0x4001401C
#define TM32B0MR2		0x40014020
#define TM32B0MR3		0x40014024
#define TM32B0CCR		0x40014028
#define TM32B0CR0		0x4001402C
#define TM32B0EMR		0x4001403C

#define TM32B1IR		0x40018000
#define TM32B1TCR		0x40018004
#define TM32B1TC		0x40018008 /* increments every PR PCLKs */
#define TM32B1PR		0x4001800C
#define TM32B1PC		0x40018010 /* increments every PCLK */
#define TM32B1MCR		0x40018014
#define TM32B1MR0		0x40018018
#define TM32B1MR1		0x4001801C
#define TM32B1MR2		0x40018020
#define TM32B1MR3		0x40018024
#define TM32B1CCR		0x40018028
#define TM32B1CR0		0x4001802C
#define TM32B1EMR		0x4001803C

#define TM32TCR_ENABLE		1
#define TM32TCR_RESET		2

#define TM32_IR_MRINT(n)	(1 << (n))
#define TM32_IR_MR0INT		TM32_IR_MRINT(0)
#define TM32_IR_MR1INT		TM32_IR_MRINT(1)
#define TM32_IR_MR2INT		TM32_IR_MRINT(2)
#define TM32_IR_MR3INT		TM32_IR_MRINT(3)
#define TM32_IR_CR0INT		(1 << 4)
#define TM32_IR_CR1INT		(1 << 5)


/* Match Control Register (MCR) actions for each Match Register */
#define TM32_M_IRQ(n)		(1 << (((n) * 3) + 0))
#define TM32_M_RESET(n)		(1 << (((n) * 3) + 1))
#define TM32_M_STOP(n)		(1 << (((n) * 3) + 2))

#define TM32_M0_IRQ		(TM32_M_IRQ(0))
#define TM32_M0_RESET		(TM32_M_RESET(0))
#define TM32_M0_STOP		(TM32_M_STOP(0))

#define TM32_M1_IRQ		(TM32_M_IRQ(1))
#define TM32_M1_RESET		(TM32_M_RESET(1))
#define TM32_M1_STOP		(TM32_M_STOP(1))

#define TM32_M2_IRQ		(TM32_M_IRQ(2))
#define TM32_M2_RESET		(TM32_M_RESET(2))
#define TM32_M2_STOP		(TM32_M_STOP(2))

#define TM32_M3_IRQ		(TM32_M_IRQ(3))
#define TM32_M3_RESET		(TM32_M_RESET(3))
#define TM32_M3_STOP		(TM32_M_STOP(3))

/* External Match Control (EMC) actions for each Match Register */
#define TM32_EMC0_CLEAR		(1 << 4)
#define TM32_EMC0_SET		(2 << 4)
#define TM32_EMC0_TOGGLE	(3 << 4)
#define TM32_EMC1_CLEAR		(1 << 6)
#define TM32_EMC1_SET		(2 << 6)
#define TM32_EMC1_TOGGLE	(3 << 6)
#define TM32_EMC2_CLEAR		(1 << 8)
#define TM32_EMC2_SET		(2 << 8)
#define TM32_EMC2_TOGGLE	(3 << 8)
#define TM32_EMC3_CLEAR		(1 << 10)
#define TM32_EMC3_SET		(2 << 10)
#define TM32_EMC3_TOGGLE	(3 << 10)

#ifdef LPC13XX_V2
#define MKGPIO(bank,num)	(((bank) << 16) | (num))
#define GPIO_PORT(gpio)		((gpio) >> 16)
#define GPIO_NUM(gpio)		((gpio) & 0xffff)
#else
#define MKGPIO(bank,num)	(((bank) << 16) | (1 << (num)))
#define GPIO_PORT(gpio)		((gpio) >> 16)
#define GPIO_NUM(gpio)		((gpio) & 0xffff)
#endif

/* serial */

void core_48mhz_init(void);
void ssp0_init(void);
unsigned ssp0_set_clock(unsigned khz);
void serial_init(unsigned sysclk_mhz, unsigned baud);
void serial_start_async_rx(void (*async_cb)(char c));

#endif
