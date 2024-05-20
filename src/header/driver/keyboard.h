#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../cpu/interrupt.h"

#define EXT_SCANCODE_UP 0x48
#define EXT_SCANCODE_DOWN 0x50
#define EXT_SCANCODE_LEFT 0x4B
#define EXT_SCANCODE_RIGHT 0x4D

#define KEYBOARD_DATA_PORT 0x60
#define EXTENDED_SCANCODE_BYTE 0xE0

/**
 * keyboard_scancode_1_to_ascii_map[256], Convert scancode values that correspond to ASCII printables
 * How to use this array: ascii_char = k[scancode]
 *
 * By default, QEMU using scancode set 1 (from empirical testing)
 */
extern const char keyboard_scancode_1_to_ascii_map[256];

/**
 * KeyboardDriverState - Contain all driver states
 *
 * @param read_extended_mode Optional, can be used for signaling next read is extended scancode (ex. arrow keys)
 * @param keyboard_input_on  Indicate whether keyboard ISR is activated or not
 * @param keyboard_buffer    Storing keyboard input values in ASCII
 */
struct KeyboardDriverState
{
    uint8_t cursorRow;
    uint8_t cursorColumn;
    bool read_extended_mode;
    bool keyboard_input_on;
    char keyboard_buffer;
    uint8_t template_length;
} __attribute((packed));

/* -- Driver Interfaces -- */

// Activate keyboard ISR / start listen keyboard & save to buffer
void keyboard_state_activate(void);

// Deactivate keyboard ISR / stop listening keyboard interrupt
void keyboard_state_deactivate(void);

// Get keyboard buffer value and flush the buffer - @param buf Pointer to char buffer
void get_keyboard_buffer(char *buf);

/* -- Keyboard Interrupt Service Routine -- */

/**
 * Handling keyboard interrupt & process scancodes into ASCII character.
 * Will start listen and process keyboard scancode if keyboard_input_on.
 */
void keyboard_isr(void);

int get_keyboard_col(void);

int get_keyboard_row(void);

void puts(char *, uint8_t, uint8_t);

void putchar(char, uint8_t);

void change_keyboard_template_length(uint8_t);

void clear_screen();

void testing(char c);

void printDigits(uint8_t number, uint8_t colPrint);

void writeClock(uint8_t jam, uint8_t menit, uint8_t detik);

#endif