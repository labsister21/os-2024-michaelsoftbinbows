ENTRY(loader)                /* the name of the entry label */
/* Relocation address 0xC0010000, but load address (physical location) 0x100000 */
SECTIONS {
    . = 0xC0100000;          /* use relocation address at 0xC0100000 */
    /* Linker variable that can be used in kernel */
    _linker_kernel_virtual_addr_start  = .; 
    _linker_kernel_physical_addr_start = . - 0xC0000000; 
    .multiboot ALIGN (0x1000) : AT (ADDR (.multiboot) - 0xC0000000) {
        *(.multiboot)        /* put GRUB multiboot header at front */
    }

    .setup.text ALIGN (0x1000) : AT (ADDR (.setup.text) - 0xC0000000) {
        *(.setup.text)       /* initial setup code */
    }

    .text ALIGN (0x1000) : AT (ADDR (.text) - 0xC0000000) {
        *(.text)             /* all text sections from all files */
    }

    .rodata ALIGN (0x1000) : AT (ADDR (.rodata) - 0xC0000000) {
        *(.rodata*)          /* all read-only data sections from all files */
    }

    .data ALIGN (0x1000) : AT (ADDR (.data) - 0xC0000000) {
        *(.data)             /* all data sections from all files */
    }

    .bss ALIGN (0x1000) : AT (ADDR (.bss) - 0xC0000000) {
        *(COMMON)            /* all COMMON sections from all files */
        *(.bss)              /* all bss sections from all files */
        [a-z\/]*kernel-entrypoint.o(.bss)
        _linker_kernel_stack_top = .;
    }
    /* Linker variable that can be used in kernel */
    _linker_kernel_virtual_addr_end  = .;
    _linker_kernel_physical_addr_end = . - 0xC0000000;
}
