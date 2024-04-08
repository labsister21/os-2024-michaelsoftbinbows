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

    initialize_filesystem_fat32();
    /*
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
    b.buf[2] = 'F';
    b.buf[3] = 'O';

    //write_blocks(&comm, 1024, 1);
    memcpy(k.ext, "BB", 3);

    k.buffer_size = 4;
    
    comm.buf[1] = write(k);


    struct ClusterBuffer res = {};

    k.buf = &res;

    k.buffer_size = CLUSTER_SIZE;

    comm.buf[2] = read(k);

    res.buf[3] = 'A';

    write_clusters(res.buf, 1024, 1);

    write_blocks(&comm, 512, 1);*/

    //char folderName[50] = "GG";

    struct ClusterBuffer res = {};

    uint8_t y = 0, breakNow = 0;

    while (true) {
         char c;
         get_keyboard_buffer(&c);
         if (c && c == 49){
            struct FAT32DriverRequest k = {};
            struct ClusterBuffer b = {};
            memset(b.buf, 0, BLOCK_SIZE);

            k.buf = &b;
            k.buffer_size = 0;
            memcpy(k.name, "GG\0\0\0\0\0", 8);
            memcpy(k.ext, "\0\0", 3);
            k.parent_cluster_number = 2;

            write(k);
            breakNow = 1;
         }
         if(c){
            //strcat(folderName, &c);
            res.buf[y] = c;
            y++;
            write_clusters(res.buf, 512, 1);
            framebuffer_write(get_keyboard_row(), get_keyboard_col()-1, c, 0xF, 0);
         }
         if(breakNow) break;
    }

    end_filesystem_fat32();
}
