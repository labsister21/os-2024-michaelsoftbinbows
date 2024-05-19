#include <stdint.h>
#include "header/filesystem/fat32.h"
#include "header/stdlib/string.h"
// #define from_bcd(val) ((val / 16) * 10 + (val & 0xf))

// void cmos_dump(uint16_t *values);

// void outportb(unsigned short _port, unsigned char _data);

// unsigned char inportb(unsigned short _port);

// /**
//  * Get the current time.
//  *
//  * @param hours   Pointer to a short to store the current hour (/24)
//  * @param minutes Pointer to a short to store the current minute
//  * @param seconds Pointer to a short to store the current second
//  */
// void get_time(uint16_t *hours, uint16_t *minutes, uint16_t *seconds);