#include "./clock.h"

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx)
{
    __asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));
    // Note : gcc usually use %eax as intermediate register,
    //        so it need to be the last one to mov
    __asm__ volatile("int $0x30");
}

// void outportb(unsigned short _port, unsigned char _data)
// {
//     asm volatile("outb %1, %0" : : "dN"(_port), "a"(_data));
// }

// unsigned char inportb(unsigned short _port)
// {
//     unsigned char rv;
//     asm volatile("inb %1, %0" : "=a"(rv) : "dN"(_port));
//     return rv;
// }

// void cmos_dump(
//     uint16_t *values)
// {
//     uint16_t index;
//     for (index = 0; index < 128; ++index)
//     {
//         outportb(0x70, index);
//         values[index] = inportb(0x71);
//     }
// }

// void get_time(
//     uint16_t *hours,
//     uint16_t *minutes,
//     uint16_t *seconds)
// {
//     uint16_t values[128]; /* CMOS dump */
//     cmos_dump(values);

//     *hours = from_bcd(values[4]);
//     *minutes = from_bcd(values[2]);
//     *seconds = from_bcd(values[0]);
// }

int main()
{
    while (true)
    {
        syscall(78, 0, 0, 0);
    }
    return 0;
}