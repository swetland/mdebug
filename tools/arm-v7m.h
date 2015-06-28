#ifndef _ARM_V7M_H_
#define _ARM_V7M_H_

// ---- debug registers -----------------
#define	DHCSR			0xE000EDF0 // Debug Halting Ctrl/Stat
#define DCRSR			0xE000EDF4 // Debug Core Reg Select
#define DCRDR			0xE000EDF8 // Debug Core Reg Data
#define DEMCR			0xE000EDFC // Debug Exception & Monitor Ctrl

#define DHCSR_DBGKEY		0xA05F0000
#define DHCSR_S_RESET_ST	(1 << 25) // CPU Reset Since Last Read (CoR)
#define DHCSR_S_RETIRE_ST	(1 << 24) // Inst Retired Since Last Read (CoR)
#define DHCSR_S_LOCKUP		(1 << 19) // CPU is locked up / unrecov excpt
#define DHCSR_S_SLEEP		(1 << 18) // CPU is Sleeping
#define DHCSR_S_HALT		(1 << 17) // CPU is Halted
#define DHCSR_S_REGRDY		(1 << 16) // 0:DCRSR Written 1:DCRSR TXN Done
#define DHCSR_C_SNAPSTALL	(1 << 5)  // Allow Imprecise Debug Entry
#define DHCSR_C_MASKINTS	(1 << 3)  // Mask PendSV, SysTick, and Ext IRQs
					  // C_HALT must already be and remain 1
#define DHCSR_C_STEP		(1 << 2)  // Enable Single Step
#define DHCSR_C_HALT		(1 << 1)  // Halt CPU
#define DHCSR_C_DEBUGEN		(1 << 0)  // Enable Halting Debug

#define DCRSR_REG_WR		(1 << 16) // Write

#define DEMCR_TRCENA		(1 << 24) // Enable DWT and ITM
#define DEMCR_MON_REQ		(1 << 19) // HW does not use, SW Sem Bit
#define DEMCR_MON_STEP		(1 << 18) // Set Step Request Pending (in Mon Mode)
#define DEMCR_MON_PEND		(1 << 17) // Set DebugMonitor Exception Pending
#define DEMCR_MON_EN		(1 << 16) // Enable DebugMonitor Exception

#define DEMCR_VC_HARDERR	(1 << 10) // Vector Catch: HardFault Exception
#define DEMCR_VC_INTERR		(1 << 9)  // Vector Catch: Exception Entry / Return
#define DEMCR_VC_BUSERR		(1 << 8)  // Vector Catch: BusFault Exception 
#define DEMCR_VC_STATERR	(1 << 7)  // Vector Catch: UsageFault State Error 
#define DEMCR_VC_CHKERR		(1 << 6)  // Vector Catch: UsageFault Check Error 
#define DEMCR_VC_NOCPERR	(1 << 5)  // Vector Catch: Coprocessor Access 
#define DEMCR_VC_MMERR		(1 << 4)  // Vector Catch: MemManage Exception
#define DEMCR_VC_CORERESET	(1 << 0)  // Vector Catch: Core Reset

// ---- fault status registers -----------
#define CFSR			0xE000ED28 // Configurable Fault Status Register
#define HFSR			0xE000ED2C // Hard Fault Status Register
#define DFSR			0xE000ED30 // Debug Fault Status Register
#define MMFAR			0xE000ED34 // MM Fault Address Register
#define BFAR			0xE000ED38 // Bus Fault Address Register
#define AFSR			0xE000ED3C // Aux Fault Status Register (Impl Defined)

#define CFSR_IACCVIOL		(1 << 0)  // Inst Access Violation
#define CFSR_DACCVIOL		(1 << 1)  // Data Access Violation
#define CFSR_MUNSTKERR		(1 << 3)  // Derived MM Fault on Exception Return
#define CFSR_MSTKERR		(1 << 4)  // Derived MM Fault on Exception Entry
#define CFSR_MLSPERR		(1 << 5)  // MM Fault During Lazy FP Save
#define CFSR_MMARVALID		(1 << 7)  // MMFAR has valid contents

#define CFSR_IBUSERR		(1 << 8)  // Bus Fault on Instruction Prefetch
#define CFSR_PRECISERR		(1 << 9)  // Precise Data Access Error, Addr in BFAR
#define CFSR_IMPRECISERR	(1 << 10) // Imprecise Data Access Error
#define CFSR_UNSTKERR		(1 << 11) // Derived Bus Fault on Exception Return
#define CFSR_STKERR		(1 << 12) // Derived Bus Fault on Exception Entry
#define CFSR_LSPERR		(1 << 13) // Bus Fault During Lazy FP Save
#define CFSR_BFARVALID		(1 << 15) // BFAR has valid contents

#define CFSR_UNDEFINSTR		(1 << 16) // Undefined Instruction Usage Fault
#define CFSR_INVSTATE		(1 << 17) // EPSR.T or ESPR.IT invalid
#define CFSR_INVPC		(1 << 18) // Integrity Check Error on EXC_RETURN
#define CFSR_NOCP		(1 << 19) // Coprocessor Error
#define CFSR_UNALIGNED		(1 << 24) // Unaligned Access Error
#define CFSR_DIVBYZERO		(1 << 25) // Divide by Zero (when CCR.DIV_0_TRP set)

#define CFSR_ALL		0x030FBFBD // all fault bits

// Write 1 to Clear Status Bits
#define DFSR_HALTED		(1 << 0)  // DHCSR C_HALT or C_STEP Request
#define DFSR_BKPT		(1 << 1)  // BKPT instruction or FPB match
#define DFSR_DWTTRAP		(1 << 2)  // DWT Debug Event
#define DFSR_VCATCH		(1 << 3)  // Vector Catch Event
#define DFSR_EXTERNAL		(1 << 4)  // External Debug Event

#define DFSR_ALL		0x0000001F // all fault bits


#define HFSR_VECTTBL		(1 << 1)  // Vector Table Read Fault
#define HFSR_FORCED		(1 << 30) // Configurable-Priority Exception Escalated
#define HFSR_DEBUGEVT		(1 << 31) // Debug Event Occurred (and halting debug off)

#define HFSR_ALL		0xC0000002 // all fault bits

// ---- Data Watchpoint and Trace --------
#define DWT_CTRL		0xE0001000 // Control Register
#define DWT_CYCCNT		0xE0001004 // Cycle Count Register
#define DWT_CPICNT		0xE0001008 // CPI Count Register
#define DWT_EXCCNT		0xE000100C // Exception Overhead Count Register
#define DWT_SLEEPCNT		0xE0001010 // Sleep Count Register
#define DWT_LSUCNT		0xE0001014 // LSU Count Register
#define DWT_FOLDCNT		0xE0001018 // Folded Instruction Count Register
#define DWT_PCSR		0xE000101C // Program Counter Sample Register
#define DWT_COMP(n)		(0xE0001020 + (n) * 0x10)
#define DWT_MASK(n)		(0xE0001024 + (n) * 0x10)
#define DWT_FUNC(n)		(0xE0001028 + (n) * 0x10)

// ---- DWT_CTRL bits ---------
#define DWT_CYCCNTENA		(1 << 0)  // Enable Cycle Counter
#define DWT_POSTPRESET(n)	(((n) & 15) << 1)
#define DWT_POSTINIT(n)		(((n) & 15) << 5)
#define DWT_CYCTAP		(1 << 9)  // 0: POSTCNT tap at CYCCNT[6], 1: at [10]
#define DWT_SYNCTAP_DISABLE	(0 << 10)
#define DWT_SYNCTAP_BIT24	(1 << 10)
#define DWT_SYNCTAP_BIT26	(2 << 10)
#define DWT_SYNCTAP_BIT28	(3 << 10)
#define DWT_PCSAMPLENA		(1 << 12) // Enable POSTCNT as timer for PC Sample Pkts
#define DWT_EXCTRCENA		(1 << 16) // Enable Exception Trace
#define DWT_CPIEVTENA		(1 << 17) // Enable CPI Counter Overflow Events
#define DWT_EXCEVTENA		(1 << 18) // Enable Exception Overhead Counter Ovf Evt
#define DWT_SLEEPEVTENA		(1 << 19) // Enable Sleep Counter Ovf Evt
#define DWT_LSUEVTENA		(1 << 20) // Enable LSU Counter Ovf Evt
#define DWT_FOLDEVTENA		(1 << 21) // Enable Folded Instruction Counter Ovf Evt
#define DWT_CYCEVTENA		(1 << 22) // Enable POSTCNT Undererflow Packets
#define DWT_NOPRFCNT		(1 << 24) // 1: No Profiling Counters
#define DWT_NOCYCCNT		(1 << 25) // 1: No Cycle Counter
#define DWT_NOEXTTRIG		(1 << 26) // 1: No External Match Signals
#define DWT_NOTRCPKT		(1 << 27) // 1: No Trace Sampling and Exception Tracing
#define DWT_NUMCOMP(v)		((v) >> 28)

// ---- DWT_FUNC(n) bits ------

#define DWT_FN_DISABLED		0x0

#define DWT_FN_WATCH_PC		0x4
#define DWT_FN_WATCH_RD		0x5
#define DWT_FN_WATCH_WR		0x6
#define DWT_FN_WATCH_RW		0x7

#define DWT_EMITRANGE		(1 << 5)  // 1: Enable Data Trace Address Packet Gen
#define DWT_CYCMATCH		(1 << 7)  // 1: Cycle Count Comparison (only for COMP0)
#define DWT_DATAVMATCH		(1 << 8)  // 0: Address Comparison  1: Value Comparison
#define DWT_LNK1ENA		(1 << 9)  // RO: 1: Linked Comparator Supported
#define DWT_DATAVSIZE_BYTE	(0 << 10)
#define DWT_DATAVSIZE_HALF	(1 << 10)
#define DWT_DATAVSIZE_WORD	(2 << 10)
#define DWT_DATAVADDR0(n)	(((n) & 15) << 12)
#define DWT_DATAVADDR1(n)	(((n) & 15) << 16)
#define DWT_MATCHED		(1 << 24) // Matched since last read. Cleared on read.

#endif

