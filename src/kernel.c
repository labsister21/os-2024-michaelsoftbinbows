#include <stdint.h>
#include "header/cpu/gdt.h"
#include "header/cpu/idt.h"
#include "header/cpu/interrupt.h"
#include "header/kernel-entrypoint.h"
#include "header/text/framebuffer.h"
#include <stdbool.h>

void kernel_setup(void) {
    load_gdt(&_gdt_gdtr);
    pic_remap();
    initialize_idt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 11);
    __asm__("int $0x4");
    framebuffer_write(0,0,'H',10,0);
    framebuffer_write(0,1,'e',10,0);
    framebuffer_write(0,2,'l',10,0);
    framebuffer_write(0,3,'l',10,0);
    framebuffer_write(0,4,'o',10,0);
    framebuffer_write(0,5,' ',10,0);
    framebuffer_write(0,6,'w',10,0);
    framebuffer_write(0,7,'o',10,0);
    framebuffer_write(0,8,'r',10,0);
    framebuffer_write(0,9,'l',10,0);
    framebuffer_write(0,10,'d',10,0);
    framebuffer_write(0,11,'!',10,0);
    while (true);
}
