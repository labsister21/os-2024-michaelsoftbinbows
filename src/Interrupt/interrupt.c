#include "../header/cpu/interrupt.h"
#include "../header/cpu/portio.h"
#include "../header/driver/keyboard.h"
#include "../header/cpu/gdt.h"
#include "../header/filesystem/fat32.h"
#include "../header/scheduler/scheduler.h"
#include "../header/cmos/cmos.h"

void io_wait(void)
{
    out(0x80, 0);
}

void pic_ack(uint8_t irq)
{
    if (irq >= 8)
        out(PIC2_COMMAND, PIC_ACK);
    out(PIC1_COMMAND, PIC_ACK);
}

void pic_remap(void)
{
    // Starts the initialization sequence in cascade mode
    out(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    out(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    out(PIC1_DATA, PIC1_OFFSET); // ICW2: Master PIC vector offset
    io_wait();
    out(PIC2_DATA, PIC2_OFFSET); // ICW2: Slave PIC vector offset
    io_wait();
    out(PIC1_DATA, 0b0100); // ICW3: tell Master PIC, slave PIC at IRQ2 (0000 0100)
    io_wait();
    out(PIC2_DATA, 0b0010); // ICW3: tell Slave PIC its cascade identity (0000 0010)
    io_wait();

    out(PIC1_DATA, ICW4_8086);
    io_wait();
    out(PIC2_DATA, ICW4_8086);
    io_wait();

    // Disable all interrupts
    out(PIC1_DATA, PIC_DISABLE_ALL_MASK);
    out(PIC2_DATA, PIC_DISABLE_ALL_MASK);
}

void main_interrupt_handler(struct InterruptFrame frame)
{
    switch (frame.int_number)
    {
    case (0x20):
        struct Context cur_context;
        cur_context.cpu = frame.cpu;
        cur_context.eflags = frame.int_stack.eflags;
        cur_context.eip = frame.int_stack.eip;
        cur_context.page_directory_virtual_addr = paging_get_current_page_directory_addr();
        scheduler_save_context_to_current_running_pcb(cur_context);

        pic_ack(IRQ_TIMER);
        scheduler_switch_to_next_process();
        break;
    case (0x21):
        keyboard_isr();
        break;
    case (0x30):
        syscall(frame);
        break;
    case (0xe):
        int ALERT = 5;
        ALERT++;
        break;
    }
}

void activate_keyboard_interrupt(void)
{
    out(PIC1_DATA, in(PIC1_DATA) & ~(1 << IRQ_KEYBOARD));
}

struct TSSEntry _interrupt_tss_entry = {
    .ss0 = GDT_KERNEL_DATA_SEGMENT_SELECTOR};

void set_tss_kernel_current_stack(void)
{
    uint32_t stack_ptr;
    // Reading base stack frame instead esp
    __asm__ volatile("mov %%ebp, %0" : "=r"(stack_ptr) : /* <Empty> */);
    // Add 8 because 4 for ret address and other 4 is for stack_ptr variable
    _interrupt_tss_entry.esp0 = stack_ptr + 8;
}

void syscall(struct InterruptFrame frame) {
    switch (frame.cpu.general.eax) {
        case 0:
            *((int8_t*) frame.cpu.general.ecx) = read(*(struct FAT32DriverRequest*) frame.cpu.general.ebx);
            break;
        case 1:
            *((int8_t*) frame.cpu.general.ecx) = read_directory(*(struct FAT32DriverRequest*) frame.cpu.general.ebx);
            break;
        case 2:
            *((int8_t*) frame.cpu.general.ecx) = write(*(struct FAT32DriverRequest*) frame.cpu.general.ebx);
            break;
        case 3:
            *((int8_t*) frame.cpu.general.ecx) = delete(*(struct FAT32DriverRequest*) frame.cpu.general.ebx);
            break;
        case 4:
            get_keyboard_buffer((char*) frame.cpu.general.ebx);
            break;
        case 5:
            putchar(*((char*)frame.cpu.general.ebx), frame.cpu.general.ecx);
            break;
        case 6:
            puts(
                (char*) frame.cpu.general.ebx, 
                frame.cpu.general.ecx, 
                frame.cpu.general.edx
            ); // Assuming puts() exist in kernel
            break;
        case 7: 
            keyboard_state_activate();
            break;
        case 8:
            *((int8_t*) frame.cpu.general.ecx) = process_create_user_process(*(struct FAT32DriverRequest*) frame.cpu.general.ebx);
            break;
        case 9:
            for (int i=0; i < PROCESS_COUNT_MAX; i++) {
                if (_process_list[i].metadata.state != Inactive) {
                    char disp = _process_list[i].metadata.pid + '0';
                    puts(_process_list[i].metadata.nama, (uint8_t) strlen(_process_list[i].metadata.nama), (uint8_t) 0xF);
                    putchar('-', (uint8_t) 0xF);
                    putchar(disp, (uint8_t) 0xF);
                    putchar('\n', (uint8_t) 0xF);
                }  
            }
            break;
        case 10: // terminasi proses
            // exit(0);
            break;
        case 11:
            char buf[2];
            memset(buf,0,2);
            memcpy(buf,(char*) frame.cpu.general.ebx,2);

            int pid;
            if (strlen(buf) == 1) {
                pid = (buf[0] - '0');
            } else {
                pid = (buf[0] - '0');
                int puluhan = (buf[1] - '0');
                pid += puluhan*10;
            }
            
            uint8_t retcode;
            if (process_destroy((uint32_t) pid)) {
                retcode = 0;
            } else {
                retcode = 1;
            }

            *((int8_t*) frame.cpu.general.ecx) = retcode;
            break;
        case 19:
            change_keyboard_template_length(*(uint8_t*)frame.cpu.general.ebx);
            break;
        case 69:
            clear_screen();
            break;
        case 78:
            read_rtc();
            writeClock((hourc + 7)%24, minutec, secondc);
            break;
        case 420:
            testing(*((char*)frame.cpu.general.ebx));
            break;
        case 666:
            struct FAT32DriverRequest request = {
            .buf                   = (uint8_t*) 0,
            .name                  = "testing",
            .ext                   = "\0\0\0",
            .parent_cluster_number = ROOT_CLUSTER_NUMBER,
            .buffer_size = 0x100000,
        };
        process_create_user_process(request);
        break;
    }
}

void activate_timer_interrupt(void)
{
    __asm__ volatile("cli");
    // Setup how often PIT fire
    uint32_t pit_timer_counter_to_fire = PIT_TIMER_COUNTER;
    out(PIT_COMMAND_REGISTER_PIO, PIT_COMMAND_VALUE);
    out(PIT_CHANNEL_0_DATA_PIO, (uint8_t)(pit_timer_counter_to_fire & 0xFF));
    out(PIT_CHANNEL_0_DATA_PIO, (uint8_t)((pit_timer_counter_to_fire >> 8) & 0xFF));

    // Activate the interrupt
    out(PIC1_DATA, in(PIC1_DATA) & ~(1 << IRQ_TIMER));
}
