#include <stdint.h>
#include "header/cpu/gdt.h"
#include "header/kernel-entrypoint.h"
#include "header/text/framebuffer.h"
#include <stdbool.h>

void kernel_setup(void)
{
    load_gdt(&_gdt_gdtr);
    framebuffer_write(0,0,'A',0,15);
    framebuffer_write(0,1,'B',0,15);
    framebuffer_write(1,0,'C',0,15);
    framebuffer_write(1,1,'D',0,15);
    while (true)
        ;
}