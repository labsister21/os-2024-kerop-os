#include "header/process/process.h"
#include "header/memory/paging.h"
#include "header/stdlib/string.h"
#include "header/cpu/gdt.h"

// Add the missing declaration of process_manager_state
static struct  ProcessManagerState process_manager_state = {
    .active_process_count = 0
};

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

    new_pcb->metadata.pid = process_generate_new_pid();

    // Membuatan virtual address space baru dengan page directory
    struct PageDirectory *new_page_dir = paging_create_new_page_directory();
    if (new_page_dir == NULL) {
        retcode = PROCESS_CREATE_FAIL_NOT_ENOUGH_MEMORY;
        goto exit_cleanup;
    }

    new_pcb->metadata.state = READY;
    new_pcb->memory.page_frame_used_count = 0;
    new_pcb->memory.virtual_addr_used[new_pcb->memory.page_frame_used_count++] = (void*) request.buf;

    // Setelah virtual memory untuk process telah dialokasikan, untuk sementara ganti page directory ke virtual address space baru dan lakukan pembacaan executable dari file system ke memory. Kembalikan virtual address space ke sebelum proses load executable setelah operasi file system selesai.

    struct PageDirectory *prev_page_dir = paging_get_current_page_directory_addr();
    paging_switch_page_directory(new_page_dir);

    // Load executable to memory
    read(request);

    // Restore previous page directory
    paging_switch_page_directory(prev_page_dir);

    // Set up process context
    new_pcb->context.eip = (uint32_t) request.buf;
    new_pcb->context.cpu.stack.esp = 0xBFFFFFFC;
    new_pcb->context.cpu.stack.ebp = 0xBFFFFFFC;
    new_pcb->context.cpu.segment.gs = GDT_USER_DATA_SEGMENT_SELECTOR | 0x3;
    new_pcb->context.cpu.segment.fs = GDT_USER_DATA_SEGMENT_SELECTOR | 0x3;
    new_pcb->context.cpu.segment.ds = GDT_USER_DATA_SEGMENT_SELECTOR | 0x3;
    new_pcb->context.cpu.segment.es = GDT_USER_DATA_SEGMENT_SELECTOR | 0x3;
    new_pcb->context.eflags |= CPU_EFLAGS_BASE_FLAG | CPU_EFLAGS_FLAG_INTERRUPT_ENABLE;
    new_pcb->context.page_directory_virtual_addr = new_page_dir;

    // Set up metadata
    new_pcb->metadata.state = READY;
    strcpy(new_pcb->metadata.name, request.name);

    process_manager_state.active_process_count++;

exit_cleanup:
    return retcode;
}
