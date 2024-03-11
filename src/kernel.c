#include <stdint.h>
#include "header/cpu/gdt.h"
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

void kernel_setup(void)
{
    load_gdt(&_gdt_gdtr);
    
    // Testing framebuffer
    framebuffer_clear();
    // framebuffer_write(0,0,'A',12,10);
    // framebuffer_write(0,1,'B',11,15);
    // framebuffer_write(1,0,'C',12,15);
    // framebuffer_write(1,1,'D',13,15);
    for(int i=0;i<25;i++){
        for(int j=0;j<79;j++){
            framebuffer_write(i,j,'Z',10,0);
            framebuffer_write(i,j+1,'A',10,0);
            framebuffer_set_cursor(i,j);
            delay(500);
        }   
    }
    while(true){;}
}