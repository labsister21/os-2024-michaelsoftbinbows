#include "../header/process/process.h"
#include "../header/paging/paging.h"
#include "../header/stdlib/string.h"
#include "../header/cpu/gdt.h"

struct ProcessManagerState process_manager_state = { 
    .active_process_count = 0,
};

struct ProcessControlBlock _process_list[PROCESS_COUNT_MAX];

int32_t current_pid = -1;

int32_t process_list_get_inactive_index() {
    return process_manager_state.active_process_count;
}

uint32_t process_generate_new_pid(){
    return process_manager_state.active_process_count++;
}

uint32_t ceil_div(uint32_t numerator, uint32_t denominator) {
    return (numerator + denominator - 1) / denominator;
}

int32_t process_create_user_process(struct FAT32DriverRequest request) {
    int32_t retcode = PROCESS_CREATE_SUCCESS; 
    if (process_manager_state.active_process_count >= PROCESS_COUNT_MAX) {
        retcode = PROCESS_CREATE_FAIL_MAX_PROCESS_EXCEEDED;
        goto exit_cleanup;
    }

    // Ensure entrypoint is not located at kernel's section at higher half
    if ((uint32_t) request.buf >= KERNEL_VIRTUAL_ADDRESS_BASE) {
        retcode = PROCESS_CREATE_FAIL_INVALID_ENTRYPOINT;
        goto exit_cleanup;
    }

    // Check whether memory is enough for the executable and additional frame for user stack
    uint32_t page_frame_count_needed = ceil_div(request.buffer_size + PAGE_FRAME_SIZE, PAGE_FRAME_SIZE);
    if (!paging_allocate_check(page_frame_count_needed) || page_frame_count_needed > PROCESS_PAGE_FRAME_COUNT_MAX) {
        retcode = PROCESS_CREATE_FAIL_NOT_ENOUGH_MEMORY;
        goto exit_cleanup;
    }

    int32_t p_index = process_list_get_inactive_index();
    struct PageDirectory *new_page = paging_create_new_page_directory();
    struct ProcessControlBlock *new_pcb = &(_process_list[p_index]);
    new_pcb->context.page_directory_virtual_addr = new_page;

    for (uint32_t i = 0; i < page_frame_count_needed; i++) {
        void* virtual_addr = (void*) (i * PAGE_FRAME_SIZE);
        paging_allocate_user_page_frame(new_page, virtual_addr);
        new_pcb->memory.virtual_addr_used[i] = virtual_addr;
    }
    struct PageDirectory *curr_dir = paging_get_current_page_directory_addr();
    paging_use_page_directory(new_page);
    read(request);
    paging_use_page_directory(curr_dir);
    
    new_pcb->memory.page_frame_used_count = page_frame_count_needed;
    new_pcb->context.eflags |= CPU_EFLAGS_BASE_FLAG | CPU_EFLAGS_FLAG_INTERRUPT_ENABLE;
    new_pcb->context.cpu.segment.ds = GDT_USER_DATA_SEGMENT_SELECTOR | 0x3;
    new_pcb->context.cpu.segment.es = GDT_USER_DATA_SEGMENT_SELECTOR | 0x3;
    new_pcb->context.cpu.segment.fs = GDT_USER_DATA_SEGMENT_SELECTOR | 0x3;
    new_pcb->context.cpu.segment.gs = GDT_USER_DATA_SEGMENT_SELECTOR | 0x3;
    new_pcb->context.cpu.stack.esp = PAGE_FRAME_SIZE;

    new_pcb->context.cpu.index.edi = 0;
    new_pcb->context.cpu.index.esi = 0;
    new_pcb->context.cpu.stack.ebp = 0;
    new_pcb->context.cpu.stack.esp = 0x400000 - 4;

    new_pcb->context.cpu.general.ebx = 0;
    new_pcb->context.cpu.general.edx = 0;
    new_pcb->context.cpu.general.ecx = 0;
    new_pcb->context.cpu.general.eax = 0;

    new_pcb->metadata.pid = process_generate_new_pid();
    new_pcb->metadata.state = Ready;
    memset(new_pcb->metadata.nama,0,8);
    memcpy(new_pcb->metadata.nama, request.name, 8);
    // process_manager_state.active_process_count++;

    exit_cleanup:
    return retcode;
}

/**
 * Get currently running process PCB pointer
 * 
 * @return Will return NULL if there's no running process
 */
struct ProcessControlBlock* process_get_current_running_pcb_pointer(void) {
    // for (int i=0; i<PROCESS_COUNT_MAX; i++) {
    //     if (_process_list[i].metadata.state == Running) {
    //         return &_process_list[i];
    //     }
    // }

    if (current_pid == -1) {
        return NULL;
    } else {
        return &_process_list[current_pid];
    }
}

/**
 * Destroy process then release page directory and process control block
 * 
 * @param pid Process ID to delete
 * @return    True if process destruction success
 */
bool process_destroy(uint32_t pid) {
    pid++;
    // struct ProcessControlBlock* cur_pcb = &_process_list[pid];
    // if (!paging_free_page_directory(cur_pcb->));
    return false;
}

// void process_context_initializer(){
//     _process_list[0].context.eflags |= CPU_EFLAGS_BASE_FLAG | CPU_EFLAGS_FLAG_INTERRUPT_ENABLE;
//     _process_list[0].context
// }