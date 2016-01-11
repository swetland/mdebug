#ifndef _TI_ICEPICK_H_
#define _TI_ICEPICK_H_

#define IP_IR_WIDTH		6

#define IP_IR_BYPASS		0x3F // W = 1
#define IP_IR_ROUTER		0x02 // W = 32, only when CONNECTED
#define IP_IR_IDCODE		0x04 // W = 32
#define IP_IR_IPCODE		0x05 // W = 32
#define IP_IR_CONNECT		0x07 // W = 8
#define IP_IR_USERCODE		0x08 // W = 32

#define IP_CONNECT_RD		0x00
#define IP_CONNECT_WR_KEY	0x89

// scanout returns results from previous scan
// loading IR between scans may result in incorrect readback
#define IP_RTR_WR			(1 << 31)
#define IP_RTR_BLK(n)			(((n) & 7) << 28)
#define IP_RTR_REG(n)			(((n) & 15) << 24)
#define IP_RTR_VAL(n)			((n) & 0x00FFFFFF)

#define IP_RTR_OK			(1 << 31) // last write succeeded

#define IP_BLK_CONTROL			0
#define IP_BLK_TEST_TLCB		1 // TLCB = TAP Linking Control Block
#define IP_BLK_DEBUG_TLCB		2

// CONTROL block registers
#define IP_CTL_ZEROS			0
#define IP_CTL_CONTROL			1
#define  IP_CTL_BLOCK_SYS_RESET		(1 << 6)
#define  IP_CTL_SYSTEM_RESET		(1 << 0)

#define IP_CTL_LINKING_MODE		2
#define  IP_CTL_ALWAYS_FIRST		(0 << 1) // TAP always closest to TDI
#define  IP_CTL_DISAPPEAR_FOREVER	(3 << 1) // TAP vanishes until power-on-reset
#define  IP_CTL_ACTIVATE_MODE		(1 << 0) // Commit on next RunTestIdle

// DEBUG TLCB registers
#define IP_DBG_TAP0			0
#define  IP_DBG_INHIBIT_SLEEP		(1 << 20)
#define  IP_DBG_IN_RESET		(1 << 17) // RD
#define  IP_DBG_RELEASE_FROM_WIR	(1 << 17) // WR release from wait-in-reset
#define  IP_DBG_RESET_NORMAL		(0 << 14) // reset normally
#define  IP_DBG_RESET_WAIT		(1 << 14) // wait after reset
#define  IP_DBG_RESET_CANCEL		(4 << 14) // cancel command lockout
#define  IP_DBG_VISIBLE_TAP		(1 << 9)  // RD
#define  IP_DBG_SELECT_TAP		(1 << 8)  // RW 1 = Make Visible on RTI
#define  IP_DBG_FORCE_ACTIVE		(1 << 3)  // RW 1 = override app power/clock
#define  IP_DBG_TAP_ACCESSIBLE		(1 << 1)  // RO 0 = no TAP (security)
#define  IP_DBG_TAP_PRESENT		(1 << 0)  // RO 0 = no TAP (hw)

// ICE Melter wakes up JTAG after 8 TCKs -- must wait 200uS
#endif

