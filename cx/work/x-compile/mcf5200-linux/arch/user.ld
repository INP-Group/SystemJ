MEMORY {
	flatmem : ORIGIN = 0x0, LENGTH = 0x400000
}

SECTIONS {
        .text : {
		_stext = ALIGN(0x4) ;
        	*(.text)
		_etext = ALIGN(0x4) ;
        } > flatmem
        .data : {
		_sdata = ALIGN(0x4) ;
		*(.rodata)
        	*(.data)
		_edata = ALIGN(0x4) ;
        } > flatmem
        .bss : {
		_sbss = ALIGN(0x4) ;
		*(.bss)
		*(COMMON)
		_ebss = ALIGN(0x4) ;
	} > flatmem
}
