/* usb.c
 *
 * Copyright 2011 Brian Swetland <swetland@frotz.net>
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <fw/types.h>
#include <fw/lib.h>
#include <fw/io.h>

#include <arch/hardware.h>
#include <arch/cpu.h>
#include <protocol/usb.h>

#include "usb-v1.h"

#if CONFIG_USB_TRACE
#define P(x...) printx(x)
#else
#define P(x...)
#endif

#ifndef CONFIG_USB_USE_IRQS
#define enable_interrupts() do {} while (0)
#define disable_interrupts() do {} while (0)
#endif

static volatile unsigned msec_counter = 0;

void usb_handle_irq(void);

static u8 _dev00[] = {
	18,		/* size */
	DSC_DEVICE,
	0x10, 0x01,	/* version */
	0xFF,		/* class */
	0x00,		/* subclass */
	0x00,		/* protocol */
	0x40,		/* maxpacket0 */
	0xd1, 0x18,	/* VID */
	0x02, 0x65,	/* PID */
	0x10, 0x01,	/* version */
	0x00,		/* manufacturer string */
	0x00,		/* product string */
	0x00,		/* serialno string */
	0x01,		/* configurations */
};

static u8 _cfg00[] = {
	9,
	DSC_CONFIG,
#if CONFIG_USB_2ND_IFC
	0x37, 0x00,	/* total length */
	0x02,		/* ifc count */
#else
	0x20, 0x00,	/* total length */
	0x01,		/* ifc count */
#endif
	0x01,		/* configuration value */
	0x00,		/* configuration string */
	0x80,		/* attributes */
	50,		/* mA/2 */

	9,
	DSC_INTERFACE,
	0x00,		/* interface number */
	0x00,		/* alt setting */
	0x02,		/* ept count */
	0xFF,		/* class */
	0x00,		/* subclass */
	0x00,		/* protocol */
	0x00,		/* interface string */

	7,
	DSC_ENDPOINT,
	0x81,		/* address */
	0x02,		/* bulk */
	0x40, 0x00,	/* max packet size */
	0x00,		/* interval */

	7,
	DSC_ENDPOINT,
	0x01,		/* address */
	0x02,		/* bulk */
	0x40, 0x00,	/* max packet size */
	0x00,		/* interval */

#if CONFIG_USB_2ND_IFC
	9,
	DSC_INTERFACE,
	0x01,		/* interface number */
	0x00,		/* alt setting */
	0x02,		/* ept count */
	0xFF,		/* class */
	0x00,		/* subclass */
	0x00,		/* protocol */
	0x00,		/* interface string */

	7,
	DSC_ENDPOINT,
	0x82,		/* address */
	0x02,		/* bulk */
	0x40, 0x00,	/* max packet size */
	0x00,		/* interval */

	7,
	DSC_ENDPOINT,
	0x02,		/* address */
	0x02,		/* bulk */
	0x40, 0x00,	/* max packet size */
	0x00,		/* interval */
#endif
};

#if CONFIG_USB_STRINGS
const static u8 lang_id[] = {
	4,
	DSC_STRING,
	0x09, 0x04
};

static u16 mfg_string[24] = {
};

static u16 prod_string[24] = {
};
#endif

static void write_sie_cmd(unsigned cmd) {
	writel(USB_INT_CC_EMPTY, USB_INT_CLEAR);
	writel(cmd | USB_OP_COMMAND, USB_CMD_CODE);
	while (!(readl(USB_INT_STATUS) & USB_INT_CC_EMPTY)) ;
	writel(USB_INT_CC_EMPTY, USB_INT_CLEAR);
}

static void write_sie(unsigned cmd, unsigned data) {
	write_sie_cmd(cmd);
	writel((data << 16) | USB_OP_WRITE, USB_CMD_CODE);
	while (!(readl(USB_INT_STATUS) & USB_INT_CC_EMPTY)) ;
	writel(USB_INT_CC_EMPTY, USB_INT_CLEAR);
}

static unsigned read_sie(unsigned cmd) {
	unsigned n;
	write_sie_cmd(cmd);
	writel(cmd | USB_OP_READ, USB_CMD_CODE);
	while (!(readl(USB_INT_STATUS) & USB_INT_CD_FULL)) ;
	n = readl(USB_CMD_DATA);
	writel(USB_INT_CC_EMPTY | USB_INT_CD_FULL, USB_INT_CLEAR);
	return n;
}

static void usb_ep0_send(const void *_data, int len, int rlen) {
	const u32 *data = _data;

	if (len > rlen)
		len = rlen;
	writel(USB_CTRL_WR_EN | USB_CTRL_EP_NUM(0), USB_CTRL);
	writel(len, USB_TX_PLEN);
	while (len > 0) {
		writel(*data++, USB_TX_DATA);
		len -= 4;
	}
	writel(0, USB_CTRL);
	write_sie_cmd(USB_CC_SEL_EPT(1));
	write_sie_cmd(USB_CC_VAL_BUFFER);
}

static void usb_ep0_send0(void) {
	writel(USB_CTRL_WR_EN | USB_CTRL_EP_NUM(0), USB_CTRL);
	writel(0, USB_TX_PLEN);
	writel(0, USB_CTRL);
	write_sie_cmd(USB_CC_SEL_EPT(1));
	write_sie_cmd(USB_CC_VAL_BUFFER);
}

#define EP0_TX_ACK_ADDR	0 /* sending ACK, then changing address */
#define EP0_TX_ACK	1 /* sending ACK */
#define EP0_RX_ACK	2 /* receiving ACK */
#define EP0_TX		3 /* sending data */
#define EP0_RX		4 /* receiving data */
#define EP0_IDLE	5 /* waiting for SETUP */

static u8 ep0state;
static u8 newaddr;

static volatile int _usb_online = 0;

static void usb_ep0_rx_setup(void) {
	unsigned c,n;
	u16 req, val, idx, len;

	do {
		writel(USB_CTRL_RD_EN | USB_CTRL_EP_NUM(0), USB_CTRL);
		/* to ensure PLEN is valid */
		asm("nop"); asm("nop"); asm("nop");
		asm("nop"); asm("nop"); asm("nop");
		c = readl(USB_RX_PLEN) & 0x1FF;
		n = readl(USB_RX_DATA);
		req = n;
		val = n >> 16;
		n = readl(USB_RX_DATA);
		idx = n;
		len = n >> 16;
		writel(0, USB_CTRL);
		/* bit 1 will be set if another SETUP arrived... */
		n = read_sie(USB_CC_CLR_BUFFER);
	} while (n & 1);

	switch (req) {
	case GET_STATUS: {
		u16 status = 0;
		usb_ep0_send(&status, sizeof(status), len);
		ep0state = EP0_RX;
		break;
	}
	case GET_DESCRIPTOR:
		if (val == 0x0100) {
			usb_ep0_send(_dev00, sizeof(_dev00), len);
		} else if (val == 0x0200) {
			usb_ep0_send(_cfg00, sizeof(_cfg00), len);
#if CONFIG_USB_STRINGS
		} else if (val == 0x0300) {
			usb_ep0_send(lang_id, sizeof(lang_id), len);
		} else if (val == 0x0301) {
			usb_ep0_send(mfg_string, mfg_string[0] & 0xff, len);
		} else if (val == 0x0302) {
			usb_ep0_send(prod_string, prod_string[0] & 0xff, len);
#endif
		} else {
			goto stall;
		}
		ep0state = EP0_RX;
		break;
	case SET_ADDRESS:
		usb_ep0_send0();
		ep0state = EP0_TX_ACK_ADDR;
		newaddr = val & 0x7F;
		break;
	case SET_CONFIGURATION:
		write_sie(USB_CC_CONFIG_DEV, (val == 1) ? 1 : 0);
		_usb_online = (val == 1);
		usb_ep0_send0();
		if (usb_online_cb)
			usb_online_cb(_usb_online);
		ep0state = EP0_TX_ACK;
		break;
	default:
		goto stall;
	}

	return;

stall:
	/* stall */
	write_sie(USB_CC_SET_EPT(0), 0x80);
	//P("? %h %h %h %h\n", req, val, idx, len);
}

static void usb_ep0_rx(void) {
	unsigned n;
	n = read_sie(USB_CC_CLR_EPT(0));
	if (n & 1) {
		usb_ep0_rx_setup();
	} else {
	}
}

void usb_ep0_tx(void) {
	unsigned n;
	n = read_sie(USB_CC_CLR_EPT(1));
	if (ep0state == EP0_TX_ACK_ADDR) {
		write_sie(USB_CC_SET_ADDR, 0x80 | newaddr);
		write_sie(USB_CC_SET_ADDR, 0x80 | newaddr);
	}
	ep0state = EP0_IDLE;
}

int usb_recv(void *_data, int count) {
	return usb_recv_timeout(_data, count, 0);
}

int usb_online(void) {
	usb_handle_irq();
	return _usb_online;
}

static int usb_epN_read(unsigned ep, void *_data, int len) {
	unsigned n;
	int sz;
	u32 *data;

	if (len > 64)
		return -EFAIL;
	if (!_usb_online)
		return -ENODEV;

	data = _data;

	disable_interrupts();

	n = read_sie(USB_CC_CLR_EPT(ep << 1));
	if (!(n & 1)) {
		enable_interrupts();
		return -EBUSY;
	}
	
	writel(USB_CTRL_RD_EN | USB_CTRL_EP_NUM(ep), USB_CTRL);
	/* to ensure PLEN is valid */
	asm("nop"); asm("nop"); asm("nop");
	asm("nop"); asm("nop"); asm("nop");
	sz = readl(USB_RX_PLEN) & 0x1FF;

	if (sz > len)
		sz = len;

	while (len > 0) {
		*data++ = readl(USB_RX_DATA);
		len -= 4;
	}

	writel(0, USB_CTRL);
	n = read_sie(USB_CC_CLR_BUFFER);

	enable_interrupts();

	return sz;
}

int usb_ep1_read(void *_data, int len) {
	return usb_epN_read(1, _data, len);
}

#if CONFIG_USB_2ND_IFC
int usb_ep2_read(void *_data, int len) {
	return usb_epN_read(2, _data, len);
}
#endif

int usb_recv_timeout(void *_data, int count, unsigned timeout) {
	int r, rx;
	u8 *data;

	data = _data;
	rx = 0;
	msec_counter = 0;

	/* if offline, wait for us to go online */
	while (!_usb_online)
		usb_handle_irq();

	while (count > 0) {
		r = usb_ep1_read(data, (count > 64) ? 64 : count);

		if (r >= 0) {
			rx += r;
			data += r;
			count -= r;
			/* terminate on short packet */
			if (r != 64)
				break;
		} else if (r == -EBUSY) {
			if (timeout && (msec_counter > timeout))
				return -ETIMEOUT;
			usb_handle_irq();
		} else {
			return r;
		}
	}
	return rx;
}

static int usb_epN_write(unsigned ep, void *_data, int len) {
	unsigned n;
	u32 *data;

	if (len > 64)
		return -EFAIL;
	if (!_usb_online)
		return -ENODEV;

	disable_interrupts();

	data = _data;
	n = read_sie(USB_CC_CLR_EPT((ep << 1) + 1));
	if (n & 1) {
		enable_interrupts();
		return -EBUSY;
	}

	writel(USB_CTRL_WR_EN | USB_CTRL_EP_NUM(ep), USB_CTRL);
	writel(len, USB_TX_PLEN);
	while (len > 0) {
		writel(*data++, USB_TX_DATA);
		len -= 4;
	}
	writel(0, USB_CTRL);

	n = read_sie(USB_CC_SEL_EPT((ep << 1) + 1));
	n = read_sie(USB_CC_VAL_BUFFER);

	enable_interrupts();

	return 0;
}

int usb_ep1_write(void *_data, int len) {
	return usb_epN_write(1, _data, len);
}

#if CONFIG_USB_2ND_IFC
int usb_ep2_write(void *_data, int len) {
	return usb_epN_write(2, _data, len);
}
#endif

int usb_xmit(void *_data, int len) {
	int r, tx, xfer;
	u8 *data;

	data = _data;
	tx = 0;

	while (len > 0) {
		xfer = (len > 64) ? 64 : len;
		r = usb_ep1_write(data, xfer);
		if (r < 0) {
			if (r == -EBUSY) {
				usb_handle_irq();
				continue;
			}
			return r;
		}
		tx += xfer;
		len -= xfer;
		data += xfer;
	}
	return tx;
}

void usb_init(unsigned vid, unsigned pid, const char *mfg, const char *prod) {
	unsigned n;

	ep0state = EP0_IDLE;

	_dev00[8] = vid;
	_dev00[9] = vid >> 8;
	_dev00[10] = pid;
	_dev00[11] = pid >> 8;

#if CONFIG_USB_STRINGS
	if (mfg) {
		int i;
		for (i = 0; mfg[i] != 0 && i < ((sizeof(mfg_string) / 2) - 1); i++) {
			mfg_string[i+1] = mfg[i];
		}
		mfg_string[0] = (DSC_STRING << 8) | ((i * 2) + 2);

		_dev00[14] = 1;
	}
	if (prod) {
		int i;
		for (i = 0; prod[i] != 0 && i < ((sizeof(prod_string) / 2) - 1); i++) {
			prod_string[i+1] = prod[i];
		}
		prod_string[0] = (DSC_STRING << 8) | ((i * 2) + 2);

		_dev00[15] = 2;
	}
#endif

	/* SYSCLK to USB REG domain */
	writel(readl(SYS_CLK_CTRL) | SYS_CLK_USB_REG, SYS_CLK_CTRL);

	/* power on USB PHY and USB PLL */
	writel(readl(0x40048238) & (~(1 << 10)), 0x40048238);
	writel(readl(0x40048238) & (~(1 << 8)), 0x40048238);

	/* wait for power */
	for (n = 0; n < 10000; n++) asm("nop");

	/* configure external IO mapping */
	writel(IOCON_FUNC_1 | IOCON_DIGITAL, IOCON_PIO0_3); /* USB_VBUS */
	writel(IOCON_FUNC_1 | IOCON_DIGITAL, IOCON_PIO0_6); /* USB_CONNECTn */

	write_sie(USB_CC_SET_ADDR, 0x80); /* ADDR=0, EN=1 */
	write_sie(USB_CC_SET_DEV_STATUS, 0x01); /* CONNECT */

	writel(USB_INT_EP0 | USB_INT_EP1, USB_INT_ENABLE);
}

void usb_stop(void) {
	write_sie(USB_CC_SET_ADDR, 0);
	write_sie(USB_CC_SET_DEV_STATUS, 0);
}

void usb_mask_ep1_rx_full(void) {
	disable_interrupts();
	writel(readl(USB_INT_ENABLE) & (~USB_INT_EP2), USB_INT_ENABLE);
	enable_interrupts();
}
void usb_unmask_ep1_rx_full(void) {
	disable_interrupts();
	writel(readl(USB_INT_ENABLE) | USB_INT_EP2, USB_INT_ENABLE);
	enable_interrupts();
}
 
void usb_mask_ep1_tx_empty(void) {
	disable_interrupts();
	writel(readl(USB_INT_ENABLE) & (~USB_INT_EP3), USB_INT_ENABLE);
	enable_interrupts();
}
void usb_unmask_ep1_tx_empty(void) {
	disable_interrupts();
	writel(readl(USB_INT_ENABLE) | USB_INT_EP3, USB_INT_ENABLE);
	enable_interrupts();
}

#if CONFIG_USB_2ND_IFC
void usb_mask_ep2_rx_full(void) {
	disable_interrupts();
	writel(readl(USB_INT_ENABLE) & (~USB_INT_EP4), USB_INT_ENABLE);
	enable_interrupts();
}
void usb_unmask_ep2_rx_full(void) {
	disable_interrupts();
	writel(readl(USB_INT_ENABLE) | USB_INT_EP4, USB_INT_ENABLE);
	enable_interrupts();
}
 
void usb_mask_ep2_tx_empty(void) {
	disable_interrupts();
	writel(readl(USB_INT_ENABLE) & (~USB_INT_EP5), USB_INT_ENABLE);
	enable_interrupts();
}
void usb_unmask_ep2_tx_empty(void) {
	disable_interrupts();
	writel(readl(USB_INT_ENABLE) | USB_INT_EP5, USB_INT_ENABLE);
	enable_interrupts();
}
#endif

void (*usb_ep1_rx_full_cb)(void);
void (*usb_ep1_tx_empty_cb)(void);
#if CONFIG_USB_2ND_IFC
void (*usb_ep2_rx_full_cb)(void);
void (*usb_ep2_tx_empty_cb)(void);
#endif
void (*usb_online_cb)(int online);

volatile int USB_IRQ_COUNT = 0;

void usb_handle_irq(void) {
	unsigned n;

	USB_IRQ_COUNT++;

//	P("usb %x\n", USB_IRQ_COUNT);

	n = readl(USB_INT_STATUS);
	//writel(n & (USB_INT_EP0 | USB_INT_EP1), USB_INT_CLEAR);
	writel(n, USB_INT_CLEAR);
	if (n & USB_INT_FRAME)
		msec_counter++;
	if (n & USB_INT_EP0)
		usb_ep0_rx();
	if (n & USB_INT_EP1)
		usb_ep0_tx();

	/* ignore masked interrupts */
	n &= readl(USB_INT_ENABLE);

	if (n & ~(USB_INT_FRAME)) {
//		P("usb n 0x%x\n", n);
	}

	if ((n & USB_INT_EP2) && usb_ep1_rx_full_cb)
		usb_ep1_rx_full_cb();
	if ((n & USB_INT_EP3) && usb_ep1_tx_empty_cb)
		usb_ep1_tx_empty_cb();
#if CONFIG_USB_2ND_IFC
	if ((n & USB_INT_EP4) && usb_ep2_rx_full_cb)
		usb_ep2_rx_full_cb();
	if ((n & USB_INT_EP5) && usb_ep2_tx_empty_cb)
		usb_ep2_tx_empty_cb();
#endif
}

#if CONFIG_USB_USE_IRQS
void handle_irq_usb_irq(void) {
	usb_handle_irq(void) {
}
#endif
