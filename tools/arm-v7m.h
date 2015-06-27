#ifndef _ARM_V7M_H_
#define _ARM_V7M_H_

// ---- debug registers -----------------
#define	DHCSR			0xE00EDF0 // Debug Halting Ctrl/Stat
#define DCRSR			0xE00EDF4 // Debug Core Reg Select
#define DCRDR			0xE00EDF8 // Debug Core Reg Data
#define DEMCR			0xE00EDFC // Debug Exception & Monitor Ctrl

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

#endif

