#include <stdint.h>
#include "header/cpu/gdt.h"
#include "header/kernel-entrypoint.h"
#include <stdbool.h>

void kernel_setup(void)
{
    load_gdt(&_gdt_gdtr);
    while (true)
        ;
}