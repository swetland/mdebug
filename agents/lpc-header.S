
.section .vectors
.syntax unified
.globl _start

_start:
	.long 0x42776166	// magic
	.long 0x00010000	// version
	.long 0x00000001	// flags
	.long _start
	.long _start + 0x400	// data addr
	.long 0x00001000	// data size
	.long CONFIG_FLASHADDR	// flash addr
	.long CONFIG_FLASHSIZE	// flash size
	.long 0
	.long 0
	.long 0
	.long 0
	.long flash_agent_setup + 1
	.long flash_agent_erase + 1
	.long flash_agent_write + 1
	.long flash_agent_ioctl + 1
