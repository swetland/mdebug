#ifndef _LPC15XX_HARDWARE_H_
#define _LPC15XX_HARDWARE_H_

#include <fw/types.h>
#include <fw/io.h>

#define MAINCLKSELA		0x40074080
#define  SELA_IRC_OSC		0x0
#define  SELA_SYS_OSC		0x1
#define  SELA_WDG_OSC		0x2
#define  SELA_RESERVED		0x3

#define MAINCLKSELB		0x40074084
#define  SELB_MAINCLKSELA	0x0
#define  SELB_SYS_PLL_IN	0x1
#define  SELB_SYS_PLL_OUT	0x2
#define  SELB_RTC_OSC_32K	0x3

#define USBCLKSEL		0x40074088
#define  SELU_IRC_OSC		0x0
#define  SELU_SYS_OSC		0x1
#define  SELU_USB_PLL_OUT	0x2
#define  SELU_MAIN_CLOCK	0x3

#define ADCASYNCCLKSEL		0x4007408C
#define  SELAA_IRC_OSC		0x0
#define  SELAA_SYS_PLL_OUT	0x1
#define  SELAA_USB_PLL_OUT	0x2
#define  SELAA_SCT_PLL_OUT	0x3

#define CLKOUTSELA		0x40074094
#define  SELOA_IRC_OSC		0x0
#define  SELOA_SYS_OSC		0x1
#define  SELOA_WDG_OSC		0x2
#define  SELOA_MAIN_CLOCK	0x3

#define CLKOUTSELB		0x40074098
#define  SELOB_CLKOUTSELA	0x0
#define  SELOB_USB_PLL_OUT	0x1
#define  SELOB_SCT_PLL_OUT	0x2
#define  SELOB_RTC_OSC_32K	0x3

#define SYSPLLCLKSEL		0x400740A0
#define  SELSP_IRC_OSC		0x0
#define  SELSP_SYS_OSC		0x1

#define USBPLLCLKSEL		0x400740A4
#define  SELUP_IRC_OSC		0x0 /* will not operate correctly */
#define  SELUP_SYS_OSC		0x1

#define SCTPLLCLKSEL		0x400740A8
#define  SELTP_IRC_OSC		0x0
#define  SELTP_SYS_OSC		0x1

#define SYSAHBCLKDIV		0x400740C0
/* 0: disable system clock */
/* 1..255: divide-by-1, divide-by-2, ... divide-by-255 */

#define SYSAHBCLKCTRL0		0x400740C4
#define  AHBCC0_SYS		(1 << 0) /* RO, always 1 */
#define  AHBCC0_ROM		(1 << 1)
#define  AHBCC0_SRAM1		(1 << 3)
#define  AHBCC0_SRAM2		(1 << 4)
#define  AHBCC0_FLASH		(1 << 7)
#define  AHBCC0_EEPROM		(1 << 9)
#define  AHBCC0_MUX		(1 << 11)
#define  AHBCC0_SWM		(1 << 12)
#define  AHBCC0_IOCON		(1 << 13)
#define  AHBCC0_GPIO0		(1 << 14)
#define  AHBCC0_GPIO1		(1 << 15)
#define  AHBCC0_GPIO2		(1 << 16)
#define  AHBCC0_PINT		(1 << 18)
#define  AHBCC0_GINT		(1 << 19)
#define  AHBCC0_DMA		(1 << 20)
#define  AHBCC0_CRC		(1 << 21)
#define  AHBCC0_WWDT		(1 << 22)
#define  AHBCC0_RTC		(1 << 23)
#define  AHBCC0_ADC0		(1 << 27)
#define  AHBCC0_ADC1		(1 << 28)
#define  AHBCC0_DAC		(1 << 29)
#define  AHBCC0_ACMP		(1 << 30)

#define SYSAHBCLKCTRL1		0x400740C8
#define  AHBCC1_MRT		(1 << 0)
#define  AHBCC1_RIT		(1 << 1)
#define  AHBCC1_SCT0		(1 << 2)
#define  AHBCC1_SCT1		(1 << 3)
#define  AHBCC1_SCT2		(1 << 4)
#define  AHBCC1_SCT3		(1 << 5)
#define  AHBCC1_SCTIPU		(1 << 6)
#define  AHBCC1_CCAN		(1 << 7)
#define  AHBCC1_SPI0		(1 << 9)
#define  AHBCC1_SPI1		(1 << 10)
#define  AHBCC1_I2C0		(1 << 13)
#define  AHBCC1_UART0		(1 << 17)
#define  AHBCC1_UART1		(1 << 18)
#define  AHBCC1_UART2		(1 << 19)
#define  AHBCC1_QEI		(1 << 21)
#define  AHBCC1_USB		(1 << 23)

/* All *CLKDIV: */
/* 0: disable clock */
/* 1..255: divide-by-1, divide-by-2, ... divide-by-255 */
#define SYSTICKCLKDIV		0x400740CC
#define UARTCLKDIV		0x400740D0
#define IOCONCLKDIV		0x400740D4
#define TRACECLKDIV		0x400740D8
#define USBCLKDIV		0x400740EC
#define ADCASYNCCLKDIV		0x400740F0
#define CLKOUTDIV		0x400740F8

#define FLASHCFG		0x40074124
/* DO NOT MODIFY ANY BITS EXCEPT FLASHTIM_MASK */
#define  FLASHTIM_MASK		(3 << 12)
#define  FLASHTIM_1CYCLE	(0 << 12) /* SYSCLK <= 25MHz */
#define  FLASHTIM_2CYCLE	(1 << 12) /* SYSCLK <= 55MHz */
#define  FLASHTIM_3CYCLE	(2 << 12) /* SYSCLK <= 72MHz */

/* see UM10736 3.6.33 pp59-60 */
#define USARTCLKCTRL		0x40074128

#define USBCLKCTRL		0x4007412C
#define  USB_AUTO_CLK		(0 << 0)
#define  USB_FORCE_CLK		(1 << 0)
#define  USB_WAKE_FALLING	(0 << 1)
#define  USB_WAKE_RISING	(1 << 1)

#define USBCLKST		0x40074130
#define  USB_NEED_CLK		 1

#define SYSOSCCTRL		0x40074188
#define  SYSOSC_BYPASS		1 /* sys_osc_clk from xtalin */
#define  SYSOSC_FREQRANGE_LOW	(0 << 1) /* 1-20MHz */
#define  SYSOSC_FREQRANGE_HIGH	(1 << 1) /* 15-25MHz */

#define RTCOSCCTRL		0x40074190
#define  RTCOSC_EN		1

/* see UM10736 3.7.4 pp73-77 */
#define SYSPLLCTRL		0x40074198
#define SYSPLLSTAT		0x4007419C
#define USBPLLCTRL		0x400741A0
#define USBPLLSTAT		0x400741A4
#define SCTPLLCTRL		0x400741A8
#define SCTPLLSTAT		0x400741AC
#define  PLLCTRL_MSEL(m)	((m)-1) /* m = 1..64 */
#define  PLLCTRL_PSEL_1		(0 << 6)
#define  PLLCTRL_PSEL_2		(1 << 6)
#define  PLLCTRL_PSEL_4		(2 << 6)
#define  PLLCTRL_PSEL_8		(2 << 6)
#define  PLLSTAT_LOCKED		1

/* analog block power control */
/* PD_* bits *disable* the block in question (PowerDown) */
/* bits 2:0 must be written as 0s */
#define PDAWAKECFG		0x40074204
/* power config on wakeup from deep-sleep or power-down */
#define PDRUNCFG		0x40074208
/* power config while running (changes are immediate) */
#define  PD_IRCOUT		(1 << 3)
#define  PD_IRC			(1 << 4)
#define  PD_FLASH		(1 << 5)
#define  PD_EEPROM		(1 << 6)
#define  PD_BOD			(1 << 8)
#define  PD_USBPHY		(1 << 9)
#define  PD_ADC0		(1 << 10)
#define  PD_ADC1		(1 << 11)
#define  PD_DAC			(1 << 12)
#define  PD_ACMP0		(1 << 13)
#define  PD_ACMP1		(1 << 14)
#define  PD_ACMP2		(1 << 15)
#define  PD_ACMP3		(1 << 16)
#define  PD_IREF		(1 << 17)
#define  PD_TS			(1 << 18)
#define  PD_VDDADIV		(1 << 19)
#define  PD_WDTOSC		(1 << 20)
#define  PD_SYSOSC		(1 << 21)  /* requires 500uS delay */
#define  PD_SYSPLL		(1 << 22)
#define  PD_USBPLL		(1 << 23)
#define  PD_SCTPLL		(1 << 24)

#define JTAGIDCODE		0x400743F4
#define  JTAGID			0x19D6C02B

#define DEVICE_ID0		0x400743F8
#define  ID_LPC1549		0x00001549
#define  ID_LPC1548		0x00001548
#define  ID_LPC1547		0x00001547
#define  ID_LPC1519		0x00001519
#define  ID_LPC1518		0x00001518
#define  ID_LPC1517		0x00001517

#define DEVICE_ID1		0x400743FC
/* boot rom and die revision */

/* pins        analog  glitch  digital  high-drive  i2c / true */
/*             func    fliter  filter   output      open drain */
/* 0_0:0_17    Y       Y       Y        N           N          */
/* 0_18:0_21   N       N       Y        N           N          */
/* 0_22:0_23   N       N       Y        N           Y          */
/* 0_24        N       N       Y        Y           N          */
/* 0_25:0_31   Y       Y       Y        N           N          */
/* 1_0:1_10    Y       Y       Y        N           N          */
/* 1_11:1_31   N       N       Y        N           N          */
/* 2_0:2_2     Y       Y       Y        N           N          */
/* 2_3:2_13    N       N       Y        N           N          */

#define PIO_IDX_NONE		0xFF
#define PIO_IDX(m,n)		(((m) << 5) + n)

#define IOCON_PIO(m,n)		(0x400F8000 + (PIO_IDX(m,n) << 2))

#define IOCON_MODE_INACTIVE	(0 << 3)  /* mode */
#define IOCON_MODE_PULL_DOWN	(1 << 3)
#define IOCON_MODE_PULL_UP	(2 << 3)
#define IOCON_MODE_REPEATER	(3 << 3)
#define IOCON_MODE_I2C          (0 << 3)  /* required for 0_22 & 0_23 */

#define IOCON_HYSTERESIS_DIS	(0 << 5)
#define IOCON_HYSTERESIS_ENA	(1 << 5)
#define IOCON_HYSTERESIS_I2C    (0 << 5)  /* required for 0_22 & 0_23 */

#define IOCON_INPUT_NORMAL	(0 << 6)  /* invert input */
#define IOCON_INPUT_INVERT	(1 << 6)

#define IOCON_GLITCH_FILTER_DIS	(0 << 8)  /* 10ns glitch filter */
#define IOCON_GLICTH_FILTER_ENA	(1 << 8)

#define IOCON_OPEN_DRAIN_DIS	(0 << 10) /* open drain mode */
#define IOCON_OPEN_DRAIN_ENA	(1 << 10) /* pseudo- except on 0_22 & 0_23 */

#define IOCON_FILTER_BYPASS	(0 << 11) /* digital filter */
#define IOCON_FILTER_1CLOCK	(1 << 11)
#define IOCON_FILTER_2CLOCK	(2 << 11)
#define IOCON_FILTER_3CLOCK	(3 << 11)
#define IOCON_FILTER_PCLK	(0 << 13)
#define IOCON_FILTER_PCLK_2	(1 << 13)
#define IOCON_FILTER_PCLK_4	(2 << 13)
#define IOCON_FILTER_PCLK_8	(3 << 13)
#define IOCON_FILTER_PCLK_16	(4 << 13)
#define IOCON_FILTER_PCLK_32	(5 << 13)
#define IOCON_FILTER_PCLK_64	(6 << 13)

#define IOCON_I2C_FAST_I2C	(0 << 8)  /* i2c only (0_22 & 0_23) */
#define IOCON_I2C_STANDARD_IO	(1 << 8)
#define IOCON_I2C_FAST_PLUS_I2C (2 << 8)

#define FUNC_UART0_TXD		0
#define FUNC_UART0_RXD		1
#define FUNC_UART0_RTS		2
#define FUNC_UART0_CTS		3
#define FUNC_UART0_SCLK		4
#define FUNC_UART1_TXD		5
#define FUNC_UART1_RXD		6
#define FUNC_UART1_RTS		7
#define FUNC_UART1_CTS		8
#define FUNC_UART1_SCLK		9
#define FUNC_UART2_TXD		10
#define FUNC_UART2_RXD		11
#define FUNC_UART2_SCLK		12
#define FUNC_SPI0_SCK		13
#define FUNC_SPI0_MOSI		14
#define FUNC_SPI0_MISO		15
#define FUNC_SPI0_SSEL0		16
#define FUNC_SPI0_SSEL1		17
#define FUNC_SPI0_SSEL2		18
#define FUNC_SPI0_SSEL3		19
#define FUNC_SPI1_SCK		20
#define FUNC_SPI1_MOSI		21
#define FUNC_SPI1_MISO		22
#define FUNC_SPI1_SSEL0		23
#define FUNC_SPI1_SSEL1		24
#define FUNC_CAN0_TD		25
#define FUNC_CAN0_RD		26
#define FUNC_USB_VBUS		28
#define FUNC_SCT0_OUT0		29
#define FUNC_SCT0_OUT1		30
#define FUNC_SCT0_OUT2		31
#define FUNC_SCT1_OUT0		32
#define FUNC_SCT1_OUT1		33
#define FUNC_SCT1_OUT2		34
#define FUNC_SCT2_OUT0		35
#define FUNC_SCT2_OUT1		36
#define FUNC_SCT2_OUT2		37
#define FUNC_SCT3_OUT0		38
#define FUNC_SCT3_OUT1		39
#define FUNC_SCT3_OUT2		40
#define FUNC_SCT_ABORT0		41
#define FUNC_SCT_ABORT1		42
#define FUNC_ADC0_PINTRIG0	43
#define FUNC_ADC0_PINTRIG1	44
#define FUNC_ADC1_PINTRIG0	45
#define FUNC_ADC1_PINTRIG1	46
#define FUNC_DAC_PINTRIG	47
#define FUNC_DAC_SHUTOFF	48
#define FUNC_ACMP0_O		49
#define FUNC_ACMP1_O		50
#define FUNC_ACMP2_O		51
#define FUNC_ACMP3_O		52
#define FUNC_CLKOUT		53
#define FUNC_ROSC		54
#define FUNC_ROSC_RESET		55
#define FUNC_USB_FTOGGLE	56
#define FUNC_QEI_PHA		57
#define FUNC_QEI_PHB		58
#define FUNC_QEI_IDX		59
#define FUNC_GPIO_INT_BMAT	60
#define FUNC_SWO		61

#define PINASSIGN(func)		(0x4003800+((func)/4))
#define  PA_SHIFT(func)		(((func) & 3) * 8)
#define  PA_MASK(func)		(~(0xFF << PA_SHIFT(func)))

static inline void pin_assign(u32 func, u32 pio_idx) {
	u32 r = PINASSIGN(func);
	u32 v = readl(v);
	writel((v & PA_MASK(func)) | (pio_idx << PA_SHIFT(func)), r);
}

/* fixed-functions are enabled by *clearing* their bit */
/* enabled fixed-functions override gpio or output matrix */
#define PINENABLE0		0x400381C0
#define  PE0_ADC0_0_ON_0_8	(1 << 0)
#define  PE0_ADC0_1_ON_0_7	(1 << 1)
#define  PE0_ADC0_2_ON_0_6	(1 << 2)
#define  PE0_ADC0_3_ON_0_5	(1 << 3)
#define  PE0_ADC0_4_ON_0_4	(1 << 4)
#define  PE0_ADC0_5_ON_0_3	(1 << 5)
#define  PE0_ADC0_6_ON_0_2	(1 << 6)
#define  PE0_ADC0_7_ON_0_1	(1 << 7)
#define  PE0_ADC0_8_ON_1_0	(1 << 8)
#define  PE0_ADC0_9_ON_0_31	(1 << 9)
#define  PE0_ADC0_10_ON_0_0	(1 << 10)
#define  PE0_ADC0_11_ON_0_30	(1 << 11)
#define  PE0_ADC1_0_ON_1_1	(1 << 12)
#define  PE0_ADC1_1_ON_0_9	(1 << 13)
#define  PE0_ADC1_2_ON_0_10	(1 << 14)
#define  PE0_ADC1_3_ON_0_11	(1 << 15)
#define  PE0_ADC1_4_ON_1_2	(1 << 16)
#define  PE0_ADC1_5_ON_1_3	(1 << 17)
#define  PE0_ADC1_6_ON_0_13	(1 << 18)
#define  PE0_ADC1_7_ON_0_14	(1 << 19)
#define  PE0_ADC1_8_ON_0_15	(1 << 20)
#define  PE0_ADC1_9_ON_0_16	(1 << 21)
#define  PE0_ADC1_10_ON_1_4	(1 << 22)
#define  PE0_ADC1_11_ON_1_5	(1 << 23)
#define  PE0_DAC_OUT_ON_0_12	(1 << 24)
#define  PE0_ACMP_I1_ON_0_27	(1 << 25)
#define  PE0_ACMP_I2_ON_1_6	(1 << 26)
#define  PE0_ACMP0_I3_ON_0_26	(1 << 27)
#define  PE0_ACMP0_I4_ON_0_25	(1 << 28)
#define  PE0_ACMP1_I3_ON_0_28	(1 << 29)
#define  PE0_ACMP1_I4_ON_1_10	(1 << 30)
#define  PE0_ACMP2_I3_ON_0_29	(1 << 31)

/* fixed-functions are enabled by *clearing* their bit */
/* enabled fixed-functions override gpio or output matrix */
#define PINENABLE1		0x400381C4
#define  PE1_ACMP2_I4_ON_1_9	(1 << 0)
#define  PE1_ACMP3_I3_ON_1_8	(1 << 1)
#define  PE1_ACMP3_I4_ON_1_7	(1 << 2)
#define  PE1_I2C0_SDA_ON_0_23	(1 << 3)
#define  PE1_I2C0_SCL_ON_0_24	(1 << 4)
#define  PE1_SCT0_OUT3_ON_0_0	(1 << 5)
#define  PE1_SCT0_OUT4_ON_0_1	(1 << 6)
#define  PE1_SCT0_OUT5_ON_0_18	(1 << 7)
#define  PE1_SCT0_OUT6_ON_0_24	(1 << 8)
#define  PE1_SCT0_OUT7_ON_1_14	(1 << 9)
#define  PE1_SCT1_OUT3_ON_0_2	(1 << 10)
#define  PE1_SCT1_OUT4_ON_0_3	(1 << 11)
#define  PE1_SCT1_OUT5_ON_0_14	(1 << 12)
#define  PE1_SCT1_OUT6_ON_0_20	(1 << 13)
#define  PE1_SCT1_OUT7_ON_1_17	(1 << 14)
#define  PE1_SCT2_OUT3_ON_0_6	(1 << 15)
#define  PE1_SCT2_OUT4_ON_0_29	(1 << 16)
#define  PE1_SCT2_OUT5_ON_1_20	(1 << 17)
#define  PE1_SCT3_OUT3_ON_0_26	(1 << 18)
#define  PE1_SCT3_OUT4_ON_1_8	(1 << 19)
#define  PE1_SCT3_OUT5_ON_1_24	(1 << 20)
#define  PE1_RESETN_ON_0_21	(1 << 21)
#define  PE1_SWCLK_ON_0_19	(1 << 22)
#define  PE1_SWDIO_ON_0_20	(1 << 23)

/* GPIO */

/* Transform PIOm_n to GPIO index number */
#define GPIO_IDX(m, n)		(((m) * 32) + (n))

/* Reads as 0x00 or 0x01 (gpio is 0 or 1) */
/* Write 0 to clear 1 to set gpio, bits 1:7 ignored */
#define GPIO_BYTE(idx)		(0x1C000000 + (idx))

/* Reads as 0x00000000 or 0xFFFFFFFF (gpio is 0 or 1) */
/* Write 0 to clear, nonzero to set gpio */
#define GPIO_WORD(idx)		(0x1C001000 + ((idx) * 4))

/* bit 0..31 are direction of io 0..31 for that port */
/* 0 is input, 1 is output */
#define GPIO0_DIR		0x1C002000
#define GPIO1_DIR		0x1C002004
#define GPIO2_DIR		0x1C002008

/* determine which ports are visible via MPORT regs (0=vis 1=masked) */
#define GPIO0_MASK		0x1C002080
#define GPIO1_MASK		0x1C002084
#define GPIO2_MASK		0x1C002088

/* raw access, read returns gpio status, write sets status */
#define GPIO0_PORT		0x1C002100
#define GPIO1_PORT		0x1C002104
#define GPIO2_PORT		0x1C002108

/* masked access */
#define GPIO0_MPORT		0x1C002180
#define GPIO1_MPORT		0x1C002184
#define GPIO2_MPORT		0x1C002188

/* set/clear/toggle registers - write 1 to take action, 0 is no-op */
#define GPIO0_SET		0x1C002200
#define GPIO1_SET		0x1C002204
#define GPIO2_SET		0x1C002208
#define GPIO0_CLR		0x1C002280
#define GPIO1_CLR		0x1C002284
#define GPIO2_CLR		0x1C002288
#define GPIO0_TGL		0x1C002300
#define GPIO1_TGL		0x1C002304
#define GPIO2_TGL		0x1C002308

#endif
