#include <stdint.h>
#include "header/cpu/gdt.h"
#include "header/cpu/idt.h"
#include "header/cpu/interrupt.h"
#include "header/kernel-entrypoint.h"
#include "header/text/framebuffer.h"
#include <stdbool.h>

// Yg delay ini boleh dihapus, cuma buat testing
void delay(int count) {
    volatile int i;
    volatile int j;
    for (i = 0; i < INT8_MAX*count; i++) {
        for(j=0;j<count;j++){

        }
    }
}

void kernel_setup(void) {
    load_gdt(&_gdt_gdtr);
    pic_remap();
    initialize_idt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);
    __asm__("int $0x4");
    while (true);
}
