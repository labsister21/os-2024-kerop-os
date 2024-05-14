#include "header/text/framebuffer.h"
#include "header/cpu/interrupt.h"
#include "header/cpu/portio.h"
#include "header/driver/keyboard.h"
#include "header/cpu/gdt.h"
#include "header/filesystem/fat32.h"
#include "header/process/process.h"
#include "header/scheduler/scheduler.h"
#include "header/driver/cmos_driver.h"

struct TSSEntry _interrupt_tss_entry = {
    .ss0 = GDT_KERNEL_DATA_SEGMENT_SELECTOR,
};

void set_tss_kernel_current_stack(void)
{
    uint32_t stack_ptr;
    // Reading base stack frame instead esp
    __asm__ volatile("mov %%ebp, %0" : "=r"(stack_ptr) : /* <Empty> */);
    // Add 8 because 4 for ret address and other 4 is for stack_ptr variable
    _interrupt_tss_entry.esp0 = stack_ptr + 8;
}

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
void syscall(struct InterruptFrame frame)
{
    switch (frame.cpu.general.eax)
    {
    case 0:
        // read
        *((int8_t *)frame.cpu.general.ecx) = read(
            *(struct FAT32DriverRequest *)frame.cpu.general.ebx);
        // read_clusters((struct ClusterBuffer*)((struct FAT32DriverRequest *)frame.cpu.general.ebx)->buf,(uint32_t)frame.cpu.general.edx,1);
        break;
    case 1:
        // read_directory
        *((int8_t *)frame.cpu.general.ecx) = read_directory(
            *(struct FAT32DriverRequest *)frame.cpu.general.ebx);
        break;
    case 2:
        // Write file
        *((int8_t *)frame.cpu.general.ecx) = write(
            *(struct FAT32DriverRequest *)frame.cpu.general.ebx);
        break;
    case 3:
        // Delete file
        *((int8_t *)frame.cpu.general.ecx) = delete (
            *(struct FAT32DriverRequest *)frame.cpu.general.ebx);
        break;
    case 4:
        get_keyboard_buffer((char *)frame.cpu.general.ebx + (int)frame.cpu.general.ecx);
        break;
    case 5:
        putchar((char *)frame.cpu.general.ebx, frame.cpu.general.ecx);
        break;
    case 6:
        puts(
            (char *)frame.cpu.general.ebx,
            frame.cpu.general.ecx,
            frame.cpu.general.edx);
        break;
    case 7:
        keyboard_state_activate();
        break;

    case 8:
        keyboard_state_deactivate();
        break;
    case 9:
        framebuffer_clear();
        framebuffer_set_cursor(0, 0);
        break;
    case 10:
        read_clusters((struct FAT32DirectoryTable *)frame.cpu.general.ebx, (uint32_t)frame.cpu.general.ecx, 1);
        break;

    case 11:
        write_clusters((struct FAT32DirectoryTable *)frame.cpu.general.ebx, (uint32_t)frame.cpu.general.ecx, 1);
        break;
    case 12:
        *((int8_t *)frame.cpu.general.ecx) = delete (
            *(struct FAT32DriverRequest *)frame.cpu.general.ebx);
        break;
    case 13:
        *((int8_t *)frame.cpu.general.ecx) =
            process_create_user_process(*(struct FAT32DriverRequest *)frame.cpu.general.ebx);
        break;

    case 14:;
        struct ProcessControlBlock pcb = *(struct ProcessControlBlock *)process_get_current_running_pcb_pointer();
        *((int8_t *)frame.cpu.general.ecx) = process_destroy((uint32_t)pcb.metadata.pid);
        break;
    case 15:
        *(struct ProcessControlBlock *)frame.cpu.general.ebx = *(struct ProcessControlBlock *)process_get_current_running_pcb_pointer();
        break;
    case 16:
        ;struct FAT32DriverRequest clockreq = {
            .buf = (uint8_t *)0,
            .name = "clock",
            .ext = "\0\0\0",
            .parent_cluster_number = ROOT_CLUSTER_NUMBER,
            .buffer_size = 0x800};
        process_create_user_process(clockreq);
        break;
    case 17:
        *(int8_t *)frame.cpu.general.ebx = get_update_in_process_flag();
        break;
    case 18:
        read_CMOS(); // init
        struct clock currTime = get_clock();
        *(uint8_t *)frame.cpu.general.edx = currTime.second;
        *(uint8_t *)frame.cpu.general.ecx = currTime.minute;
        *(uint8_t *)frame.cpu.general.ebx = currTime.hour;
        break;
    case 19:
        putTime((char*)frame.cpu.general.ebx,(char*)frame.cpu.general.ecx,(char*)frame.cpu.general.edx);
    }
}

void main_interrupt_handler(struct InterruptFrame frame)
{
    switch (frame.int_number)
    {
    case (PIC1_OFFSET + IRQ_TIMER):;
        struct PageDirectory *current = paging_get_current_page_directory_addr();
        struct Context ctx = {
            .cpu = frame.cpu,
            .eflags = frame.int_stack.eflags,
            .eip = frame.int_stack.eip,
            .page_directory_virtual_addr = current,
        };
        scheduler_save_context_to_current_running_pcb(ctx);

        pic_ack(IRQ_TIMER);

        scheduler_switch_to_next_process();

        break;
    case IRQ_KEYBOARD + PIC1_OFFSET:
        keyboard_isr();
        break;
    case 0x30:
        syscall(frame);
        break;
    }
}

void activate_keyboard_interrupt(void)
{
    out(PIC1_DATA, in(PIC1_DATA) & ~(1 << IRQ_KEYBOARD));
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
