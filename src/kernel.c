#include <stdint.h>
#include "header/cpu/gdt.h"
#include "header/cpu/idt.h"
#include "header/cpu/interrupt.h"
#include "header/kernel-entrypoint.h"
#include "header/text/framebuffer.h"
#include "header/filesystem/disk.h"
#include "header/filesystem/fat32.h"
#include <stdbool.h>

void kernel_setup(void) {
    load_gdt(&_gdt_gdtr);
    pic_remap();
    //activate_keyboard_interrupt();
    initialize_idt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);

    initialize_filesystem_fat32();

    struct FAT32DirectoryTable u = {};

    create_directory_table(&u, "tes\0\0\0\0", 2, 3);
    
    struct FAT32DriverRequest k = {};

    struct ClusterBuffer b = {};
    memset(b.buf, 0, BLOCK_SIZE);
    b.buf[0] = 'A';
    b.buf[1] = 'B';
    b.buf[2] = 'C';

    k.buf = &b;
    k.buffer_size = 3;
    memcpy(k.name, "Feel\0\0\0", 8);
    memcpy(k.ext, "AA", 3);
    k.parent_cluster_number = 3;


    struct BlockBuffer comm = {};

    comm.buf[0] = write(k);

    memset(b.buf, 0, BLOCK_SIZE);
    b.buf[0] = 'D';
    b.buf[1] = 'E';

    //write_blocks(&comm, 1024, 1);
    memcpy(k.ext, "BB", 3);
    
    comm.buf[1] = write(k);
    write_blocks(&comm, 1024, 1);

    struct FAT32DirectoryTable ftbl = {};

    struct FAT32DriverRequest drq = {};

    drq.buf = &ftbl;
    memcpy(drq.name, "tes\0\0\0\0", 8);
    drq.parent_cluster_number = 2;
    drq.buffer_size = sizeof(struct FAT32DirectoryTable);

    comm.buf[0] = read_directory(drq);

    memcpy(ftbl.table[0].name, "UWU\0\0\0\0", 8);

    write_clusters(&ftbl, 1024, 1);

    write_blocks(comm.buf, 512, 1);

    end_filesystem_fat32();

    while (true);

}
