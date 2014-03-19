/* Based on Documentation/usbmon.txt in the Linux Kernel */

#ifndef _USBMON_H_
#define _USBMON_H_

struct usbmon_packet {
	u64 id;			/* URB ID */
	unsigned char type;	/* 'S'ubmit 'C'allback 'E'rror */
	unsigned char xfer;	/* ISO=0 INT=1 CTRL=2 BULK=3 */
	unsigned char epnum;
	unsigned char devnum;
	u16 busnum;
	char flag_setup;
	char flag_data;
	s64 ts_sec;
	s32 ts_usec;
	int status;
	unsigned int length;
	unsigned int len_cap;
	union {
		unsigned char setup[8];
		struct iso_rec {
			int error_count;
			int numdesc;
		} iso;
	} s;
	int interval;
	int start_frame;
	unsigned int xfer_flags;
	unsigned int ndesc;
};

struct usbmon_get {
	struct usbmon_packet *hdr;
	void *data;
	size_t alloc;
};

#define MON_IOC_MAGIC	0x92

#define MON_IOCX_GET	_IOW(MON_IOC_MAGIC, 6, struct usbmon_get)
#define MON_IOCX_GETX	_IOW(MON_IOC_MAGIC, 10, struct usbmon_get)

#endif
