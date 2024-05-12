#include "header/scheduler/scheduler.h"
#include "header/process/process.h"
#include "header/cpu/interrupt.h"

/* --- Scheduler --- */
/**
 * Initialize scheduler before executing init process
 */
void scheduler_init(void) {

}

/**
 * Save context to current running process
 * 
 * @param ctx Context to save to current running process control block
 */
void scheduler_save_context_to_current_running_pcb(struct Context ctx){
    struct ProcessControlBlock *pcb = process_get_current_running_pcb_pointer();
    pcb->context = ctx;
}


/**
 * Trigger the scheduler algorithm and context switch to new process
 */
__attribute__((noreturn)) void scheduler_switch_to_next_process(void){
    struct ProcessControlBlock *pcb = process_get_current_running_pcb_pointer();
    uint32_t current_pid = pcb->metadata.pid;
    uint32_t next_pid = current_pid + 1;
    if(next_pid >= PROCESS_COUNT_MAX){
        next_pid = 0;
    }
    while(_process_list[next_pid].metadata.state != READY){
        next_pid++;
        if(next_pid >= PROCESS_COUNT_MAX){
            next_pid = 0;
        }
    }
    pcb->metadata.state = READY;
    _process_list[next_pid].metadata.state = RUNNING;
    process_context_switch(_process_list[next_pid].context);
    while(1);
}