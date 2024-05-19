#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "../header/cmos/cmos.h"
#include "../header/stdlib/string.h"
#include "../header/cpu/portio.h"

uint8_t secondc;
uint8_t minutec;
uint8_t hourc;

int get_update_in_progress_flag()
{
    out(cmos_address, 0x0A);
    return (in(cmos_data) & 0x80);
}

uint8_t get_RTC_register(int reg)
{
    out(cmos_address, reg);
    return in(cmos_data);
}

void read_rtc()
{
    uint8_t last_second;
    uint8_t last_minute;
    uint8_t last_hour;
    uint8_t registerB;

    while (get_update_in_progress_flag())
        ;
    secondc = get_RTC_register(0x00);
    minutec = get_RTC_register(0x02);
    hourc = get_RTC_register(0x04);
    do
    {
        last_second = secondc;
        last_minute = minutec;
        last_hour = hourc;
        while (get_update_in_progress_flag())
            ;
        secondc = get_RTC_register(0x00);
        minutec = get_RTC_register(0x02);
        hourc = get_RTC_register(0x04);
    } while ((last_second != secondc) || (last_minute != minutec) || (last_hour != hourc));
    registerB = get_RTC_register(0x0B);

    // Convert BCD to binary values if necessary

    if (!(registerB & 0x04))
    {
        secondc = (secondc & 0x0F) + ((secondc / 16) * 10);
        minutec = (minutec & 0x0F) + ((minutec / 16) * 10);
        hourc = ((hourc & 0x0F) + (((hourc & 0x70) / 16) * 10)) | (hourc & 0x80);
    }

    // Convert 12 hour clock to 24 hour clock if necessary

    if (!(registerB & 0x02) && (hourc & 0x80))
    {
        hourc = ((hourc & 0x7F) + 12) % 24;
    }
}