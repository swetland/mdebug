
/* ROM plus RAM binary layout */

SECTIONS {
	.text : {
		. = ALIGN(4);
		KEEP (*(.vectors))
		*(.text)
		*(.text.*)
		*(.rodata)
		*(.rodata.*)
		. = ALIGN(4);
		__data_init__ = . ;
	} >FLASH
	.data : AT(__data_init__) {
		. = ALIGN(4);
		__data_start__ = . ;
		. = ALIGN(4);
		*(.data)
		*(.data.*)
		. = ALIGN(4);
		__data_end__ = . ;
	} >RAM
	.bss : {
		. = ALIGN(4);
		__bss_start__ = . ;
		*(.bss)
		*(.bss.*)
		*(COMMON)
		. = ALIGN(4);
		__bss_end__ = . ;
	} >RAM
}
