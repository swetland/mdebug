#ifndef __ARCH_LPC13XX_USB_V1_H
#define __ARCH_LPC13XX_USB_V1_H

#define USB_INT_STATUS		0x40020000
#define USB_INT_ENABLE		0x40020004
#define USB_INT_CLEAR		0x40020008
#define USB_INT_SET		0x4002000C
#define USB_CMD_CODE		0x40020010
#define USB_CMD_DATA		0x40020014
#define USB_RX_DATA		0x40020018
#define USB_TX_DATA		0x4002001C
#define USB_RX_PLEN		0x40020020
#define USB_TX_PLEN		0x40020024
#define USB_CTRL		0x40020028
#define USB_FIQ_SELECT		0x4002002C

#define USB_INT_FRAME		(1 << 0)
#define USB_INT_EP0		(1 << 1)
#define USB_INT_EP1		(1 << 2)
#define USB_INT_EP2		(1 << 3)
#define USB_INT_EP3		(1 << 4)
#define USB_INT_EP4		(1 << 5)
#define USB_INT_EP5		(1 << 6)
#define USB_INT_EP6		(1 << 7)
#define USB_INT_EP7		(1 << 8)
#define USB_INT_DEV_STAT	(1 << 9) /* RESET, SUSPEND, CONNECT */
#define USB_INT_CC_EMPTY	(1 << 10) /* can write CMD_CODE */
#define USB_INT_CD_FULL		(1 << 11) /* can read CMD_DATA */
#define USB_INT_RX_END		(1 << 12)
#define USB_INT_TX_END		(1 << 13)

#define USB_CTRL_RD_EN		(1 << 0)
#define USB_CTRL_WR_EN		(1 << 1)
#define USB_CTRL_EP_NUM(n)	(((n) & 0xF) << 2)

#define USB_OP_WRITE		0x0100
#define USB_OP_READ		0x0200
#define USB_OP_COMMAND		0x0500

#define USB_CC_SET_ADDR		0xD00000
#define USB_CC_CONFIG_DEV	0xD80000
#define USB_CC_SET_MODE		0xF30000
#define USB_CC_RD_INT_STATUS	0xF40000
#define USB_CC_RD_FRAME_NUM	0xF50000
#define USB_CC_RD_CHIP_ID	0xFD0000
#define USB_CC_SET_DEV_STATUS	0xFE0000
#define USB_CC_GET_DEV_STATUS	0xFE0000
#define USB_CC_GET_ERROR_CODE	0xFF0000
#define USB_CC_SEL_EPT(n)	(((n) & 0xF) << 16)
#define USB_CC_CLR_EPT(n)	((((n) & 0xF) | 0x40) << 16)
#define USB_CC_SET_EPT(n)	((((n) & 0xF) | 0x40) << 16)
#define USB_CC_CLR_BUFFER	0xF20000
#define USB_CC_VAL_BUFFER	0xFA0000

#endif
