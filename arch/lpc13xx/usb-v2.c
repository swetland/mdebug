/* usb.c
 *
 * Copyright 2011 Brian Swetland <swetland@frotz.net>
 * Copyright 2013-2014 Erik Gillikg <konkers@konkers.net>
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

#include <fw/io.h>
#include <fw/lib.h>
#include <fw/string.h>
#include <fw/types.h>

#include <arch/cpu.h>
#include <arch/interrupts.h>
#include <arch/hardware.h>

#include <protocol/usb.h>

#include "usb-v2.h"

#undef DEBUG

#ifdef DEBUG
#define usb_debug(fmt, args...) do { \
		printx(fmt, ##args); \
	} while (0)

static inline void dump_bytes(void *addr, int len)
{

	int i;
	for (i = 0; i <len; i++) {
		if (i != 0)
			printx(" ");
		printx("%b", ((u8 *)addr)[i]);
	}
	printx("\n");
}

#else
#define usb_debug(fmt, args...) do { \
	} while (0)
static inline void dump_bytes(void *addr, int len)
{
}
#endif

static volatile unsigned msec_counter = 0;

void usb_handle_irq(void);

static u8 _dev00[] = {
	18,		/* size */
	DSC_DEVICE,
	0x00, 0x01,	/* version */
	0xFF,		/* class */
	0x00,		/* subclass */
	0x00,		/* protocol */
	0x40,		/* maxpacket0 */
	0xd1, 0x18,	/* VID */
	0x02, 0x65,	/* PID */
	0x00, 0x01,	/* version */
	0x00,		/* manufacturer string */
	0x00,		/* product string */
	0x00,		/* serialno string */
	0x01,		/* configurations */
};

static u8 _cfg00[] = {
	9,
	DSC_CONFIG,
	0x20, 0x00,	/* total length */
	0x01,		/* ifc count */
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
};

const static u8 lang_id[] = {
	4,
	DSC_STRING,
	0x09, 0x04
};

static u16 mfg_string[24] = {
};

static u16 prod_string[24] = {
};

static void *usb_sram_highwater = (void *) USB_SRAM;
static u32 *usb_ep_list;
static u8 *usb_setup_buf;

struct usb_ep {
	u8 addr;

	u16 in_size;
	u16 in_max_packet;
	i16 in_len;
	u16 in_pos;

	u16 out_size;
	u16 out_len;
	u16 out_pos;

	u8 *out_buf;
	u8 *in_buf;
	/* no pingpong buffers supported yet */
};

static struct usb_ep usb_ep_data[5];

enum {
	USB_STATE_OFFLINE = 0,
	USB_STATE_ONLINE,
};

static volatile unsigned usb_frames;
volatile int usb_state;

void (*usb_ep1_rx_full_cb)(void);
void (*usb_ep1_tx_empty_cb)(void);
void (*usb_ep2_rx_full_cb)(void);
void (*usb_ep2_tx_empty_cb)(void);
void (*usb_online_cb)(int online);


static void *usb_sram_alloc(int size, int align)
{
	void *highwater = usb_sram_highwater;
	highwater += (align - ((unsigned)usb_sram_highwater % align)) % align;
	if (highwater + size > (void *)(USB_SRAM + USB_SRAM_SIZE)) {
		printx("can't allocate 0x%x bytes of USB SRAM\n", size);
		return 0;
	}
	usb_sram_highwater = highwater + size;

	return highwater;
}

static void usb_setup_ep(struct usb_ep *ep, u8 addr, u16 in_buf_size, u16 in_max_packet, u16 out_buf_size)
{
	ep->addr = addr;

	ep->in_buf = usb_sram_alloc(in_buf_size, 64);
	ep->in_size = in_buf_size;
	ep->in_len = -1;
	ep->in_max_packet = in_max_packet;
	ep->out_buf = usb_sram_alloc(out_buf_size, 64);
	ep->out_size = out_buf_size;
}

void usb_init(unsigned vid, unsigned pid, const char *mfg, const char *prod) {
	unsigned n;

	irq_enable(v_usb_irq);

	usb_state = USB_STATE_OFFLINE;

	_dev00[8] = vid;
	_dev00[9] = vid >> 8;
	_dev00[10] = pid;
	_dev00[11] = pid >> 8;

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

	/* SYSCLK to USB REG domain */
	writel(readl(SYS_CLK_CTRL) | SYS_CLK_USB_REG | SYS_CLK_USBSRAM, SYS_CLK_CTRL);

	writel(0, USB_DEVCMDSTAT);

	/* power on USB PHY and USB PLL */
	writel(readl(0x40048238) & (~(1 << 10)), 0x40048238);
	writel(readl(0x40048238) & (~(1 << 8)), 0x40048238);

	/* wait for power */
	for (n = 0; n < 10000; n++) asm("nop");

	usb_ep_list = usb_sram_alloc(0x50, 256);
	usb_setup_buf = usb_sram_alloc(9, 64);

	usb_setup_ep(&usb_ep_data[0], 0x0, 64, 64, 64);
	usb_setup_ep(&usb_ep_data[1], 0x1, 128, 64, 64);

	for (n = 0; n < 4; n++)
		usb_ep_list[n] = 0;
	for (n = 4; n < 20; n++)
		usb_ep_list[n] = USB_EP_LIST_DISABLED;
	for (n = 0; n < 9; n++)
		usb_setup_buf[n] = 0;

	usb_ep_list[EP_LIST_SETUP] = USB_EP_LIST_BUF_ADDR(usb_setup_buf);
	writel((unsigned)usb_ep_list, USB_EPLISTSTART);
	writel(USB_SRAM, USB_DATABUFSTART);


	writel(0, USB_INTROUTING);
	writel(USB_DEVCMDSTAT_DEV_ADDR(0) |
	       USB_DEVCMDSTAT_DEV_EN |
	       USB_DEVCMDSTAT_DCON |
	       USB_DEVCMDSTAT_DSUS,
	       USB_DEVCMDSTAT);

	writel(USB_INT_DEV_INT |
	       USB_INT_EP0OUT | USB_INT_EP0IN,
	       USB_INTEN);
}

static void usb_send_in(struct usb_ep *ep)
{
//	unsigned ep_out_cmd = usb_ep_list[ep->addr * 4];
	unsigned ep_cmd = usb_ep_list[ep->addr * 4 + 2];
	ep_cmd = USB_EP_LIST_BUF_ADDR(ep->in_buf + ep->in_pos) |
		USB_EP_LIST_BUF_SIZE(min(ep->in_len, ep->in_max_packet)) |
		USB_EP_LIST_ACTIVE;

//	if (ep->in_len >= ep->in_max_packet)
//		ep_cmd |= USB_EP_LIST_STALL;
//	ep_out_cmd = clr_set_bits(ep_out_cmd, USB_EP_LIST_ACTIVE, USB_EP_LIST_STALL);
//	usb_ep_list[ep->addr * 4] = ep_out_cmd;
	usb_ep_list[ep->addr * 4 + 2] = ep_cmd;

	/* clear error code */
	writel(0x0, USB_INFO);

//	if (len)
//		clr_set_reg(USB_DEVCMDSTAT, USB_DEVCMDSTAT_INTONNAK_CO, USB_DEVCMDSTAT_INTONNAK_CI);
}

static void usb_send_zlp(struct usb_ep *ep)
{
	ep->in_pos = 0;
	ep->in_len = 0;
	usb_send_in(ep);
}

static void usb_nak(struct usb_ep *ep)
{
	unsigned ep_out_cmd = usb_ep_list[ep->addr * 4];
	unsigned ep_in_cmd = usb_ep_list[ep->addr * 4 + 2];

	ep_out_cmd = clr_set_bits(ep_out_cmd, USB_EP_LIST_ACTIVE, USB_EP_LIST_STALL);
	ep_in_cmd = clr_set_bits(ep_in_cmd, USB_EP_LIST_ACTIVE, USB_EP_LIST_STALL);

	usb_ep_list[ep->addr * 4 + 2] = ep_in_cmd;
	usb_ep_list[ep->addr * 4] = ep_out_cmd;
}

static int usb_ep_write(struct usb_ep *ep, const void *data, int len)
{
	if (len > ep->in_size)
		return -EFAIL;

	if (ep->in_len >= 0)
		return -EBUSY;

	memcpy(ep->in_buf, data, len);
	ep->in_len = len;
	ep->in_pos = 0;
	usb_send_in(ep);

	return 0;
}

static void usb_send_desc(const void *desc, int len)
{
	if (usb_ep_data[0].in_size < len) {
		usb_debug("descriptor size (%x) larger than in buffer (%x)\n",
		       len, usb_ep_data[0].in_size);
		usb_nak(&usb_ep_data[0]);
		return;
	}
	usb_ep_write(&usb_ep_data[0], desc, len);
}


static void usb_handle_get_desc(struct usb_setup_req *req)
{
	switch (req->wValue) {
	case USB_DESC_VALUE(USB_DESC_DEVICE, 0):
		usb_send_desc(_dev00, min(sizeof(_dev00),req->wLength));
		break;

	case USB_DESC_VALUE(USB_DESC_CONFIG, 0):
		usb_send_desc(_cfg00, min(sizeof(_cfg00),req->wLength));
		break;

	case USB_DESC_VALUE(USB_DESC_STRING, 0):
		usb_send_desc(lang_id, min(sizeof(lang_id),req->wLength));
		break;

	case USB_DESC_VALUE(USB_DESC_STRING, 1):
		usb_send_desc(mfg_string, min(sizeof(mfg_string),req->wLength));
		break;

	case USB_DESC_VALUE(USB_DESC_STRING, 2):
		usb_send_desc(prod_string, min(sizeof(prod_string),req->wLength));
		break;

	default:
		usb_debug("unknown desc: %h\n", req->wValue);

		usb_nak(&usb_ep_data[0]);
		break;
	}
}

static void usb_config_ep(struct usb_ep *ep)
{
	/* out buf 0 */
//	usb_ep_list[ep->addr * 4] = USB_EP_LIST_TR;
	usb_ep_list[ep->addr * 4] =  USB_EP_LIST_BUF_ADDR(ep->out_buf) |
		USB_EP_LIST_BUF_SIZE(ep->out_size) |
		USB_EP_LIST_ACTIVE | /* USB_EP_LIST_STALL | */ USB_EP_LIST_TR;

	/* in buf 0 */
	usb_ep_list[ep->addr * 4 + 2] =  USB_EP_LIST_TR; // USB_EP_LIST_STALL;

	clr_set_reg(USB_INTEN, 0, 0x3 << (ep->addr * 2));
}

static void usb_handle_set_config(unsigned config)
{
        usb_debug("set_config(%d)\n", config);
	if (config != 1) {
		usb_nak(&usb_ep_data[0]);
		return;
	}
	usb_config_ep(&usb_ep_data[1]);
//	clr_set_reg(USB_DEVCMDSTAT, 0, USB_DEVCMDSTAT_INTONNAK_AI);

	usb_send_zlp(&usb_ep_data[0]);
	usb_state = USB_STATE_ONLINE;
	if (usb_online_cb)
		usb_online_cb(1);
        printx("state = %d\n", usb_state);
}
static void usb_handle_setup_req(struct usb_setup_req *req)
{
	switch (req->bRequest) {
	case USB_REQ_GET_DESCRIPTOR:
		usb_handle_get_desc(req);
		break;
	case USB_REQ_SET_ADDRESS:
		clr_set_reg(USB_DEVCMDSTAT, USB_DEVCMDSTAT_DEV_ADDR(~0),
			    USB_DEVCMDSTAT_DEV_ADDR(req->wValue));
		usb_send_zlp(&usb_ep_data[0]);
		break;
	case USB_REQ_GET_CONFIGURATION: {
                u8 config = usb_state == USB_STATE_ONLINE;
                usb_debug("get_config(%d)\n", config);
                usb_ep_write(&usb_ep_data[0], &config, 1);
                break;
        }

	case USB_REQ_SET_CONFIGURATION:
		usb_handle_set_config(req->wValue);
		break;

	default:
		usb_debug("unknown setup req:\n");
		usb_debug("  bmRequestType: %b\n", req->bmRequestType);
		usb_debug("     dir: %b\n", req->t.dir);
		usb_debug("     type: %b\n", req->t.type);
		usb_debug("     rcpt: %b\n", req->t.rcpt);
		usb_debug("  bRequest:      %b\n", req->bRequest);
		usb_debug("  wValue:        %h\n", req->wValue);
		usb_debug("  wIndex:        %h\n", req->wIndex);
		usb_debug("  wLength:       %h\n", req->wLength);

		usb_nak(&usb_ep_data[0]);
		break;
	}
}

void usb_handle_dev_int(void)
{
	unsigned cmdstat = readl(USB_DEVCMDSTAT);
	int n;

	if (cmdstat & USB_DEVCMDSTAT_DCON_C) {
	}
	if (cmdstat & USB_DEVCMDSTAT_DSUS_C) {
	}
	if (cmdstat & USB_DEVCMDSTAT_DRES_C) {
		for (n = 4; n < 20; n++)
			usb_ep_list[n] = USB_EP_LIST_DISABLED;
	}
	writel(cmdstat, USB_DEVCMDSTAT);
	writel(USB_INT_DEV_INT, USB_INTSTAT);
}

void usb_handle_ep0out(void)
{
	writel(USB_INT_EP0OUT, USB_INTSTAT);
	unsigned cmdstat = readl(USB_DEVCMDSTAT);

	if (cmdstat & USB_DEVCMDSTAT_SETUP) {
		struct usb_setup_req *req;
		usb_ep_list[EP_LIST_EP0_OUT] &= ~(USB_EP_LIST_ACTIVE | USB_EP_LIST_STALL);
		usb_ep_list[EP_LIST_EP0_IN] &= ~(USB_EP_LIST_ACTIVE | USB_EP_LIST_STALL);

		writel(USB_INT_EP0IN, USB_INTSTAT);
		writel(cmdstat, USB_DEVCMDSTAT);

		req = (struct usb_setup_req *)usb_setup_buf;
		usb_handle_setup_req(req);
	} else {
		usb_ep_list[EP_LIST_EP0_OUT] &= ~(USB_EP_LIST_STALL);

	}
}

void usb_handle_ep0in(void)
{
	writel(USB_INT_EP0IN, USB_INTSTAT);
	unsigned ep_in_cmd = usb_ep_list[EP_LIST_EP0_IN];

	if (usb_ep_data[0].in_len >= 0) {
		unsigned ep_out_cmd = usb_ep_list[EP_LIST_EP0_OUT];

		ep_out_cmd = USB_EP_LIST_BUF_ADDR(usb_ep_data[0].out_buf) |
			USB_EP_LIST_BUF_SIZE(usb_ep_data[0].out_size) |
			USB_EP_LIST_STALL | USB_EP_LIST_ACTIVE ;


		usb_ep_list[EP_LIST_EP0_OUT] = ep_out_cmd;
	}

	ep_in_cmd = clr_set_bits(ep_in_cmd, USB_EP_LIST_ACTIVE, USB_EP_LIST_STALL);

	usb_ep_list[EP_LIST_EP0_IN] = ep_in_cmd;
	if (usb_ep_data[0].in_len >= 0) {
		writel(USB_INT_EP0OUT, USB_INTSTAT);
		usb_ep_data[0].in_len = -1;
	}
}

static void usb_handle_in(struct usb_ep *ep, unsigned int_mask)
{
	int pos = ep->in_pos;
	pos -= ep->in_max_packet;
//	printx("ep%b in %x %b\n", ep->addr, usb_ep_list[ep->addr * 4 + 2],
//	       USB_INFO_ERR_CODE(readl(USB_INFO)));

	writel(int_mask, USB_INTSTAT);

	if (pos < 0) {
		ep->in_pos = 0;
		ep->in_len = -1;
		if (ep->addr == 0x01 && usb_ep1_tx_empty_cb)
			usb_ep1_tx_empty_cb();
		if (ep->addr == 0x02 && usb_ep2_tx_empty_cb)
			usb_ep2_tx_empty_cb();
	} else if (pos == 0) {
		ep->in_pos = 0;
		usb_send_zlp(ep);
	} else {
		ep->in_pos = pos;
		usb_send_in(ep);
	}
}

static void usb_handle_out(struct usb_ep *ep, unsigned int_mask)
{
	unsigned cmd = usb_ep_list[ep->addr * 4];

//	usb_debug("usb_out ep%b %x\n", ep->addr, usb_ep_list[ep->addr * 4]);
	ep->out_len = ep->out_size - USB_EP_LIST_GET_BUF_SIZE(cmd);
	ep->out_pos = 0;

	if (usb_ep1_rx_full_cb)
		usb_ep1_rx_full_cb();

	writel(int_mask, USB_INTSTAT);
}

static int usb_ep_read(struct usb_ep *ep, void *data, int max)
{
	int len = min(ep->out_len - ep->out_pos, max);

	if (len == 0)
		return -EBUSY;

	memcpy(data, ep->out_buf + ep->out_pos, len);

	/* XXX: not rentrant with the irq handler!*/
	ep->out_pos += len;

	if (ep->out_pos == ep->out_len) {
		usb_ep_list[ep->addr * 4] =  USB_EP_LIST_BUF_ADDR(ep->out_buf) |
			USB_EP_LIST_BUF_SIZE(ep->out_size) |
			USB_EP_LIST_ACTIVE;// | USB_EP_LIST_STALL;
	}

	return len;
}


void handle_irq_usb_irq(void) {
	unsigned status = readl(USB_INTSTAT);

        status &= readl(USB_INTEN);

	if (status & USB_INT_FRAME_INT) {
		usb_frames++;
		writel(USB_INT_FRAME_INT, USB_INTEN);
	}


	if (status & USB_INT_DEV_INT)
		usb_handle_dev_int();
	if (status & USB_INT_EP0OUT)
		usb_handle_ep0out();
	if (status & USB_INT_EP0IN)
		usb_handle_ep0in();
	if (status & USB_INT_EP1OUT)
		usb_handle_out(&usb_ep_data[1], USB_INT_EP1OUT);
	if (status & USB_INT_EP1IN)
		usb_handle_in(&usb_ep_data[1], USB_INT_EP1IN);


}

void usb_handle_irq(void) {
	handle_irq_usb_irq();
}


/* USB API */
int usb_ep1_read(void *data, int max)
{
	return usb_ep_read(&usb_ep_data[1], data, max);
}


int usb_ep1_write(void *data, int len)
{
	return usb_ep_write(&usb_ep_data[1], data, len);
}

int usb_ep2_read(void *data, int max)
{
	return -1; // XXX real error
}

int usb_ep2_write(void *data, int len)
{
	return -1; // XXX real error
}

void usb_mask_ep1_rx_full(void)
{

}

void usb_unmask_ep1_rx_full(void)
{

}

void usb_mask_ep1_tx_empty(void)
{

}

void usb_unmask_ep1_tx_empty(void)
{

}

void usb_mask_ep2_rx_full(void)
{

}

void usb_unmask_ep2_rx_full(void)
{

}

void usb_mask_ep2_tx_empty(void)
{

}

void usb_unmask_ep2_tx_empty(void)
{

}


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

int usb_recv_timeout(void *_data, int count, unsigned timeout) {
	int r, rx;
	u8 *data;

	data = _data;
	rx = 0;
	usb_frames = 0;

	/* if offline, wait for us to go online */
	while (usb_state == USB_STATE_OFFLINE)
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
			if (timeout && (usb_frames > timeout))
				return -ETIMEOUT;
			usb_handle_irq();
		} else {
			return r;
		}
	}
	return rx;
}

int usb_recv(void *_data, int count) {
	return usb_recv_timeout(_data, count, 0);
}

int usb_online(void) {
	return usb_state == USB_STATE_ONLINE;
}

void usb_stop(void) {
	/* disable dev */
	writel(0, USB_DEVCMDSTAT);

	/* power off USB PHY and USB PLL */
	clr_set_reg(0x40048238, (1 << 10) | (1 << 8), 0);

	/* turn of SYSCLK to USB REG domain */
	clr_set_reg(SYS_CLK_CTRL, SYS_CLK_USB_REG | SYS_CLK_USBSRAM, 0);
}
