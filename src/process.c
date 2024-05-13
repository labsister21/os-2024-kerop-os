#include "header/memory/paging.h"
#include "header/stdlib/string.h"
#include "header/cpu/gdt.h"
#include "header/process/process.h"

struct ProcessControlBlock _process_list[PROCESS_COUNT_MAX] = {0};

// Add the missing declaration of process_manager_state
struct ProcessManagerState process_manager_state = {
    .list_of_process = {false},
    .active_process_count = 0
};

uint32_t process_list_get_inactive_index(){
    for(int i = 0; i < PROCESS_COUNT_MAX; i++){
        if(!process_manager_state.list_of_process[i]){
            return i;
        }
    }
    return -1;
}

uint32_t ceil_div(uint32_t request, uint32_t target){
    float result = (float) request / target;
    return (result == (int) result) ? (int) result : (int) result + 1;
}

/**
 * Get currently running process PCB pointer
 * 
 * @return Will return NULL if there's no running process
 */
struct ProcessControlBlock* process_get_current_running_pcb_pointer(void){
    struct ProcessControlBlock *pcb = NULL;
    for (int i = 0; i < PROCESS_COUNT_MAX; i++)
    {
        if (_process_list[i].metadata.state == RUNNING)
        {
            pcb = &_process_list[i];
            break;
        }
    }
    return pcb;
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
    process_manager_state.list_of_process[p_index] = true;
    process_manager_state.active_process_count++;

    new_pcb->metadata.pid = p_index;

    // Membuatan virtual address space baru dengan page directory
    struct PageDirectory *new_page_dir = paging_create_new_page_directory();
    if (new_page_dir == NULL) {
        retcode = PROCESS_CREATE_FAIL_NOT_ENOUGH_MEMORY;
        goto exit_cleanup;
    }

    new_pcb->metadata.state = READY;
    new_pcb->memory.page_frame_used_count = 0;
    // new_pcb->memory.virtual_addr_used[new_pcb->memory.page_frame_used_count++] = (void*) request.buf;

    struct PageDirectory *prev_page_dir = paging_get_current_page_directory_addr();
    paging_use_page_directory(new_page_dir);

    paging_allocate_user_page_frame(new_page_dir,request.buf);
    paging_allocate_user_page_frame(new_page_dir,(void*) 0xBFFFFFFC);
    

    // Load executable to memory
    read(request);

    // Restore previous page directory
    paging_use_page_directory(prev_page_dir);

    // Set up process context
    new_pcb->context.eip = (uint32_t) request.buf;
    new_pcb->context.cpu.stack.esp = KERNEL_VIRTUAL_ADDRESS_BASE - 4;
    new_pcb->context.cpu.stack.ebp = KERNEL_VIRTUAL_ADDRESS_BASE - 4;
    new_pcb->context.cpu.segment.gs = GDT_USER_DATA_SEGMENT_SELECTOR | 0x3;
    new_pcb->context.cpu.segment.fs = GDT_USER_DATA_SEGMENT_SELECTOR | 0x3;
    new_pcb->context.cpu.segment.ds = GDT_USER_DATA_SEGMENT_SELECTOR | 0x3;
    new_pcb->context.cpu.segment.es = GDT_USER_DATA_SEGMENT_SELECTOR | 0x3;
    new_pcb->context.eflags |= CPU_EFLAGS_BASE_FLAG | CPU_EFLAGS_FLAG_INTERRUPT_ENABLE;
    new_pcb->context.page_directory_virtual_addr = new_page_dir;

    // Set up metadata
    new_pcb->metadata.state = READY;
    memset(new_pcb->metadata.name, 0, PROCESS_NAME_LENGTH_MAX);
    memcpy(new_pcb->metadata.name, request.name, 8);

exit_cleanup:
    return retcode;
}

/**
 * Destroy process then release page directory and process control block
 * 
 * @param pid Process ID to delete
 * @return    True if process destruction success
 */
bool process_destroy(uint32_t pid){
    if(pid >= PROCESS_COUNT_MAX){
        return false;
    }
    if(!process_manager_state.list_of_process[pid]){
        return false;
    }
    struct ProcessControlBlock *pcb = &_process_list[pid];
    struct PageDirectory *page_dir = pcb->context.page_directory_virtual_addr;
    paging_free_page_directory(page_dir);
    process_manager_state.list_of_process[pid] = false;
    process_manager_state.active_process_count--;
    return true;
}