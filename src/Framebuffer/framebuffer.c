#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../header/text/framebuffer.h"
#include "../header/stdlib/string.h"
#include "../header/cpu/portio.h"

int framebuffer_position(uint8_t r, uint8_t c){
    return (160*r)+(2*c);
}

void framebuffer_set_cursor(uint8_t r, uint8_t c) {
        out(CURSOR_PORT_CMD, 0xE);
        out(CURSOR_PORT_DATA,((framebuffer_position(r,c) >> 8) & 0x00FF));
        out(CURSOR_PORT_CMD, 0xF);
        out(CURSOR_PORT_DATA,framebuffer_position(r,c) & 0x00FF);
}

void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg) {
    FRAMEBUFFER_MEMORY_OFFSET[framebuffer_position(row,col)] = c;
    FRAMEBUFFER_MEMORY_OFFSET[framebuffer_position(row,col)+1] = ((bg & 0x0F) << 4) | (fg & 0x0F);
}

void framebuffer_clear(void) {
    for(int i=0;i<25;i++){
        for(int j=0;j<80;j++){
            framebuffer_write(i,j,0x00,0x07,0x00);
        }
    }
}
