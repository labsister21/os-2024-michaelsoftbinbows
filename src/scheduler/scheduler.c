#include "../header/scheduler/scheduler.h"

/* --- Scheduler --- */
/**
 * Initialize scheduler before executing init process 
 */
void scheduler_init(void) {
    activate_timer_interrupt();
}

/**
 * Save context to current running process
 * 
 * @param ctx Context to save to current running process control block
 */
void scheduler_save_context_to_current_running_pcb(struct Context ctx) {
    struct ProcessControlBlock* cur_pcb = process_get_current_running_pcb_pointer();
    cur_pcb->context.cpu = ctx.cpu;
    cur_pcb->context.eflags = ctx.eflags;
    cur_pcb->context.eip = ctx.eip;
    cur_pcb->context.page_directory_virtual_addr = ctx.page_directory_virtual_addr;
    cur_pcb->metadata.state = Ready;
}

/**
 * Trigger the scheduler algorithm and context switch to new process
 */
__attribute__((noreturn)) void scheduler_switch_to_next_process(void) {
    struct Context next_context;
    int context_id, num_active_process;
    num_active_process = process_manager_state.active_process_count;

    context_id = current_pid+1; 
    while (context_id != current_pid) {
        if (context_id >= num_active_process) {
            context_id=0;
        }
        if (_process_list[context_id].metadata.state == Ready) {
            break;
        }
        context_id++;
    }

    next_context = _process_list[context_id].context;
    _process_list[context_id].metadata.state = Running;
    current_pid = context_id;
    //Virtual Address Space & Process Manipulation 
    paging_use_page_directory(next_context.page_directory_virtual_addr);

    // dummy variables
    // struct PageDirectory temp;
    // next_context.eip = 1; 
    // next_context.eflags = 2; 
    // next_context.page_directory_virtual_addr = &temp; 

    process_context_switch(next_context);
}
