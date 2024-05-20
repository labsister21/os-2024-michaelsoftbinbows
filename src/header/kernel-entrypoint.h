#ifndef _KERNEL_ENTRYPOINT
#define _KERNEL_ENTRYPOINT

#include "header/cpu/gdt.h"

/**
 * Load GDT from gdtr and complete init for protected mode. This procedure implemented in asm.
 * Note: This procedure will not activate CPU interrupt flag
 * 
 * @param gdtr Pointer to already defined & initialized GDTR
 * @warning Invalid address / definition of GDT will cause bootloop after calling this procedure.
 */
extern void load_gdt(struct GDTR *gdtr);

// Optional linker variable : Pointing to kernel start & end address
// Note : Use & operator, example : a = (uint32_t) &_linker_kernel_stack_top;
extern uint32_t _linker_kernel_virtual_addr_start;
extern uint32_t _linker_kernel_virtual_addr_end;
extern uint32_t _linker_kernel_physical_addr_start;
extern uint32_t _linker_kernel_physical_addr_end;
extern uint32_t _linker_kernel_stack_top;

/**
 * Execute user program from kernel, one way jump. This function is defined in asm source code.
 * 
 * @param virtual_addr Pointer into user program that already in memory
 * @warning            Assuming pointed memory is properly loaded with instruction
 */
extern void kernel_execute_user_program(void *virtual_addr);

/**
 * Set the tss register pointing to GDT_TSS_SELECTOR with ring 0
 */
extern void set_tss_register(void);  // Implemented in kernel-entrypoint.s

#endif