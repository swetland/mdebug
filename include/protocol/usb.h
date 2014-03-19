#ifndef _PROTOCOL_USB_H_
#define _PROTOCOL_USB_H_

#define DSC_DEVICE		0x01
#define DSC_CONFIG		0x02
#define DSC_STRING		0x03
#define DSC_INTERFACE		0x04
#define DSC_ENDPOINT		0x05

#define GET_STATUS		0x0080
#define SET_ADDRESS		0x0500
#define GET_DESCRIPTOR		0x0680
#define SET_DESCRIPTOR		0x0700
#define GET_CONFIGURATION	0x0880
#define SET_CONFIGURATION	0x0900
#define GET_INTERFACE		0x0A81
#define SET_INTERFACE		0x0B01

struct usb_setup_req{
	union {
		u8 bmRequestType;
		struct {
			unsigned rcpt:4;
			unsigned type:3;
			unsigned dir:1;
		} __attribute__((packed)) t ;
	};
	u8 bRequest;
	u16 wValue;
	u16 wIndex;
	u16 wLength;
} __attribute__((packed));

#define USB_REQ_GET_STATUS		0
#define USB_REQ_CLEAR_FEATURE		1
#define USB_REQ_SET_FEATURE		3
#define USB_REQ_SET_ADDRESS		5
#define USB_REQ_GET_DESCRIPTOR		6
#define USB_REQ_SET_DESCRIPTOR		7
#define USB_REQ_GET_CONFIGURATION	8
#define USB_REQ_SET_CONFIGURATION	9
#define USB_REQ_GET_INTERFACE		10
#define USB_REQ_SET_INTERFACE		11
#define USB_REQ_SYNCH_FRAME		12
#define USB_REQ_SET_SEL			48
#define USB_REQ_SET_ISOCH_DELAY		49

#define USB_DESC_DEVICE			1
#define USB_DESC_CONFIG			2
#define USB_DESC_STRING			3
#define USB_DESC_INTERFACE		4
#define USB_DESC_ENDPOINT		5
#define USB_DESC_INTERFACE_POWER	8
#define USB_DESC_OTG			9
#define USB_DESC_DEBUG			10
#define USB_DESC_INTERFACE_ASSOCIATION	11
#define USB_DESC_BOS			15
#define USB_DESC_DEVICE_CAPABILITY	16
#define USB_DESC_SUPERSPEED_ENDPOINT	48

#define USB_DESC_VALUE(type, index)	(((type) << 8) | ((index) & 0xff))

#endif

