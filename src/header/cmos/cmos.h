#ifndef _CMOS_H
#define _CMOS_H
#define CURRENT_YEAR 2024 // Change this each year!

// int century_register = 0x00; // Set by ACPI table parsing code if possible
extern uint8_t secondc;
extern uint8_t minutec;
extern uint8_t hourc;

enum
{
    cmos_address = 0x70,
    cmos_data = 0x71
};

int get_update_in_progress_flag();

uint8_t get_RTC_register(int reg);

void read_rtc();

#endif