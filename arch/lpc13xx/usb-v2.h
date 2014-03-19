#ifndef __ARCH_LPC13XX_USB_V2_H
#define __ARCH_LPC13XX_USB_V2_H

#define USB_BASE		0x40080000
#define USB_SRAM		0x20004000
#define USB_SRAM_SIZE		0x800

#define USB_DEVCMDSTAT		(USB_BASE + 0x00)
#define  USB_DEVCMDSTAT_DEV_ADDR(x)	((x) & 0x7f)
#define  USB_DEVCMDSTAT_DEV_EN		(1 << 7)
#define  USB_DEVCMDSTAT_SETUP		(1 << 8)
#define  USB_DEVCMDSTAT_PLL_ON		(1 << 9)
#define  USB_DEVCMDSTAT_LPM_SUP		(1 << 11)
#define  USB_DEVCMDSTAT_INTONNAK_AO	(1 << 12)
#define  USB_DEVCMDSTAT_INTONNAK_AI	(1 << 13)
#define  USB_DEVCMDSTAT_INTONNAK_CO	(1 << 14)
#define  USB_DEVCMDSTAT_INTONNAK_CI	(1 << 15)
#define  USB_DEVCMDSTAT_DCON		(1 << 16)
#define  USB_DEVCMDSTAT_DSUS		(1 << 17)
#define  USB_DEVCMDSTAT_LPM_SUS		(1 << 19)
#define  USB_DEVCMDSTAT_LPM_REWP	(1 << 20)
#define  USB_DEVCMDSTAT_DCON_C		(1 << 24)
#define  USB_DEVCMDSTAT_DSUS_C		(1 << 25)
#define  USB_DEVCMDSTAT_DRES_C		(1 << 26)
#define  USB_DEVCMDSTAT_VBUSDEBOUNCED	(1 << 28)

#define USB_INFO		(USB_BASE + 0x04)
#define  USB_INFO_FRAME_NR(reg)		((reg) & 0x7ff)
#define  USB_INFO_ERR_CODE(reg)		(((reg) >> 11) & 0xf)

#define USB_EPLISTSTART		(USB_BASE + 0x08) /* must be 256 byte aligned */
#define USB_DATABUFSTART	(USB_BASE + 0x0C) /* musb be 0x400000 alignd */

#define USB_LPM			(USB_BASE + 0x10)

#define USB_EPSKIP		(USB_BASE + 0x14)
#define USB_EPINUSE		(USB_BASE + 0x18)
#define USB_EPBUFCFG		(USB_BASE + 0x1C)

#define USB_INTSTAT		(USB_BASE + 0x20)
#define USB_INTEN		(USB_BASE + 0x24)
#define USB_INTSETSTAT		(USB_BASE + 0x28)
#define USB_INTROUTING		(USB_BASE + 0x2C)
#define  USB_INT_EP0OUT			(1 << 0)
#define  USB_INT_EP0IN			(1 << 1)
#define  USB_INT_EP1OUT			(1 << 2)
#define  USB_INT_EP1IN			(1 << 3)
#define  USB_INT_EP2OUT			(1 << 4)
#define  USB_INT_EP2IN			(1 << 5)
#define  USB_INT_EP3OUT			(1 << 6)
#define  USB_INT_EP3IN			(1 << 7)
#define  USB_INT_EP4OUT			(1 << 8)
#define  USB_INT_EP4IN			(1 << 9)
#define  USB_INT_FRAME_INT		(1 << 30)
#define  USB_INT_DEV_INT		(1 << 31)

#define USB_EPTOGGLE		(USB_BASE + 0x34)

#define USB_EP_LIST_BUF_ADDR(addr)		(((unsigned)(addr) >> 6) & 0xffff)
#define USB_EP_LIST_BUF_SIZE(size)		(((size) & 0x3ff) << 16)
#define USB_EP_LIST_GET_BUF_SIZE(reg)		(((reg) >> 16) & 0x3ff)
#define USB_EP_LIST_TYPE			(1 << 26) /* Endpoint Type */
#define USB_EP_LIST_RF_TV			(1 << 27) /* Rate Feedback/Toggle value */
#define USB_EP_LIST_TR				(1 << 28) /* Toggle Reset */
#define USB_EP_LIST_STALL			(1 << 29) /* Stall */
#define USB_EP_LIST_DISABLED			(1 << 30) /* Disabled */
#define USB_EP_LIST_ACTIVE			(1 << 31) /* Active */

enum {
	EP_LIST_EP0_OUT = 0,
	EP_LIST_SETUP,
	EP_LIST_EP0_IN,
	EP_LIST_RES,
	EP_LIST_EP1_OUT0,
	EP_LIST_EP1_OUT1,
	EP_LIST_EP1_IN0,
	EP_LIST_EP1_IN1,
	EP_LIST_EP2_OUT0,
	EP_LIST_EP2_OUT1,
	EP_LIST_EP2_IN0,
	EP_LIST_EP2_IN1,
	EP_LIST_EP3_OUT0,
	EP_LIST_EP3_OUT1,
	EP_LIST_EP3_IN0,
	EP_LIST_EP3_IN1,
	EP_LIST_EP4_OUT0,
	EP_LIST_EP4_OUT1,
	EP_LIST_EP4_IN0,
	EP_LIST_EP4_IN1,
};

#endif
