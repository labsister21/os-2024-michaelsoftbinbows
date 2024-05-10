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

    // Allocate first 4 MiB virtual memory
    paging_allocate_user_page_frame(&_paging_kernel_page_directory, (uint8_t*) 0);

    // Write shell into memory
    struct FAT32DriverRequest request = {
        .buf                   = (uint8_t*) 0,
        .name                  = "shell",
        .ext                   = "\0\0\0",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size           = 0x100000,
    };

    struct ClusterBuffer clb = {0};

    char* uwu = "AAAAA\nAAAAA\nAAAA";

    memset(clb.buf, 0, CLUSTER_SIZE);
    memcpy(clb.buf, uwu, 17);

    struct FAT32DriverRequest write_req = {
        .buf = clb.buf,
        .name = "aaa\0\0\0\0",
        .ext  = "\0\0\0",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size = sizeof(struct ClusterBuffer)
    };
    memcpy(write_req.ext, "txt", 3);
    write(write_req);
    uint8_t ret = read(request);
    struct BlockBuffer b;
    b.buf[0] = ret;
    b.buf[1] = 'A';
    write_blocks(b.buf, 0x100, 1);
    // Set TSS $esp pointer and jump into shell 
    set_tss_kernel_current_stack();
    kernel_execute_user_program((uint8_t*) 0);
    int a = 1;
    a++;

    while (true);
}
