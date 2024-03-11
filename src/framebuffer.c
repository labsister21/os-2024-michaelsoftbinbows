#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "header/text/framebuffer.h"
#include "header/stdlib/string.h"
#include "header/cpu/portio.h"

// void framebuffer_set_cursor(uint8_t r, uint8_t c) {
    // TODO : Implement
// }

void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg) {
    FRAMEBUFFER_MEMORY_OFFSET[(160*row)+(2*col)] = c;
    FRAMEBUFFER_MEMORY_OFFSET[(160*row)+(2*col)+1] = ((fg & 0x0F) << 4) | (bg & 0x0F);
}

// void framebuffer_clear(void) {
    // TODO : Implement
// }
