#include <stdint.h>
#include "header/cpu/gdt.h"
#include "header/cpu/idt.h"
#include "header/cpu/interrupt.h"
#include "header/kernel-entrypoint.h"
#include "header/text/framebuffer.h"
#include "header/driver/keyboard.h"
#include "header/filesystem/disk.h"
#include "header/filesystem/fat32.h"
#include "header/stdlib/string.h"
#include "header/paging/paging.h"
#include "header/process/process.h"
#include <stdbool.h>

void kernel_setup(void) {
    load_gdt(&_gdt_gdtr);
    pic_remap();
    initialize_idt();
    activate_keyboard_interrupt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);
    initialize_filesystem_fat32();
    gdt_install_tss();
    set_tss_register();

    // Shell request
    struct FAT32DriverRequest request = {
        .buf                   = (uint8_t*) 0,
        .name                  = "shell",
        .ext                   = "\0\0\0",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size           = 0x100000,
    };

    // Set TSS.esp0 for interprivilege interrupt
    set_tss_kernel_current_stack();

    // Create & execute process 0
    process_create_user_process(request);
    paging_use_page_directory(_process_list[0].context.page_directory_virtual_addr);
    kernel_execute_user_program((void*) 0x0);
}
