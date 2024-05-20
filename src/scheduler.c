#include "header/scheduler/scheduler.h"
#include "header/process/process.h"
#include "header/cpu/interrupt.h"

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
void scheduler_save_context_to_current_running_pcb(struct Context ctx){
    struct ProcessControlBlock *pcb = process_get_current_running_pcb_pointer();
    if(pcb != NULL){
        pcb->metadata.state = RUNNING;
        pcb->context = ctx;
    }
}

/**
 * Trigger the scheduler algorithm and context switch to new process
 */
__attribute__((noreturn)) void scheduler_switch_to_next_process(void){
    struct ProcessControlBlock *pcb = process_get_current_running_pcb_pointer();
    if (pcb == NULL){
        pcb = &_process_list[0];
    }

    uint32_t current_pid = pcb->metadata.pid;
    uint32_t next_pid = current_pid + 1;
    pcb->metadata.state = READY;
    if (next_pid >= PROCESS_COUNT_MAX) {
        next_pid = 0;
    }
    // Find next process to run
    while (process_manager_state.list_of_process[next_pid] == false) {
        next_pid++;
        if (next_pid >= PROCESS_COUNT_MAX){
            next_pid = 0;
        }
    }
    struct ProcessControlBlock *next_pcb = &_process_list[next_pid];
    next_pcb->metadata.state = RUNNING;
    paging_use_page_directory(next_pcb->context.page_directory_virtual_addr);
    process_context_switch(next_pcb->context);
}