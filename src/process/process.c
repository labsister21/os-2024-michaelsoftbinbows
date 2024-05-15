#include "../header/process/process.h"
#include "../header/paging/paging.h"
#include "../header/stdlib/string.h"
#include "../header/cpu/gdt.h"

typedef struct {
    int active_process_count;
} ProcessManagerState;

static ProcessManagerState process_manager_state = { .active_process_count = 0 };

struct ProcessControlBlock _process_list[PROCESS_COUNT_MAX];

static uint32_t current_pid = 0;

int32_t process_list_get_inactive_index() {
    for (int32_t i = 0; i < PROCESS_COUNT_MAX; i++) {
        if (_process_list[i].metadata.state != Running) {
            return i;
        }
    }
    return -1;
}

uint32_t process_generate_new_pid(){
    return ++current_pid;
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

    // Process PCB 
    int32_t p_index = process_list_get_inactive_index();
    struct ProcessControlBlock *new_pcb = &(_process_list[p_index]);

    new_pcb->metadata.pid = process_generate_new_pid();
    process_manager_state.active_process_count++;

exit_cleanup:
    return retcode;
}


// void process_context_initializer(){
//     _process_list[0].context.eflags |= CPU_EFLAGS_BASE_FLAG | CPU_EFLAGS_FLAG_INTERRUPT_ENABLE;
//     _process_list[0].context
// }
