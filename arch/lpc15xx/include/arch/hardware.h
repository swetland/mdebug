#ifndef _LPC15XX_HARDWARE_H_
#define _LPC15XX_HARDWARE_H_

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

#define PARTID			0x400743F8
#define  ID_LPC1549		0x00001549
#define  ID_LPC1548		0x00001548
#define  ID_LPC1547		0x00001547
#define  ID_LPC1519		0x00001519
#define  ID_LPC1518		0x00001518
#define  ID_LPC1517		0x00001517

#define REVID			0x400743FC

#endif
