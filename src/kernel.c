#include <stdint.h>
#include "header/cpu/gdt.h"
#include "header/cpu/idt.h"
#include "header/cpu/interrupt.h"
#include "header/kernel-entrypoint.h"
#include "header/text/framebuffer.h"
#include "header/driver/keyboard.h"
#include "header/filesystem/disk.h"
#include "header/filesystem/fat32.h"
#include <stdbool.h>

void kernel_setup(void) {
    load_gdt(&_gdt_gdtr);
    pic_remap();
    //activate_keyboard_interrupt();
    initialize_idt();
    activate_keyboard_interrupt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);
        
    keyboard_state_activate();
    while (true) {
         char c;
         get_keyboard_buffer(&c);
         if (c) framebuffer_write(cursorRow, cursorColumn-1, c, 0xF, 0);
    }

}
