#include "header/text/framebuffer.h"
#include "header/cpu/interrupt.h"
#include "header/cpu/portio.h"
#include "header/driver/keyboard.h"
#include "header/cpu/gdt.h"
#include "header/filesystem/fat32.h"
#include "header/process/process.h"
#include "header/scheduler/scheduler.h"
#include "header/driver/cmos_driver.h"
#include "header/stdlib/string.h"

#define MAX_DIR_STACK_SIZE 16
#define BLACK 0x00
#define DARK_BLUE 0x01
#define DARK_GREEN 0x2
#define DARK_AQUA 0x3
#define DARK_RED 0x4
#define DARK_PURPLE 0x5
#define GOLD 0x6
#define GRAY 0x7
#define DARK_GRAY 0x8
#define BLUE 0x09
#define GREEN 0x0A
#define AQUA 0x0B
#define RED 0x0C
#define LIGHT_PURPLE 0x0D
#define YELLOW 0x0E
#define WHITE 0x0F
#define MAX_INPUT_BUFFER 63
#define MAX_ARGS 0x3
#define MAX_QUEUE_SIZE 20

void printInt(int num)
{
    // Check if num is 0
    char input = '0';
    if (num == 0)
    {
        putchar((char *)&input, WHITE);
        return;
    }

    // Calculate the number of digits
    int temp = num;
    int numDigits = 0;
    while (temp > 0)
    {
        temp /= 10;
        numDigits++;
    }
    char result[4] = "\0\0\0\0";
    // Convert digits to characters
    int k = 0;
    for (int i = numDigits - 1; num > 0; i--)
    {
        input = (num % 10) + '0';
        result[k] = input;
        k += 1;
        // syscall_user(5, (uint32_t)&input, WHITE, 0);
        num /= 10;
    }
    k = 3;
    for (; k >= 0; k--)
    {
        putchar((char *)(result + k), WHITE);
    }
    return;
}
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
        *((int8_t *)frame.cpu.general.ecx) = process_destroy((uint32_t)frame.cpu.general.ebx);
        break;
    case 15:
        *(struct ProcessControlBlock *)frame.cpu.general.ebx = *(struct ProcessControlBlock *)process_get_current_running_pcb_pointer();
        break;
    case 16:;
        struct FAT32DriverRequest clockreq = {
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
        putTime((char *)frame.cpu.general.ebx, (char *)frame.cpu.general.ecx, (char *)frame.cpu.general.edx, ':');
        break;
    case 20:
        // ;uint8_t i = 0;
        // uint8_t maxPid = 0;
        // for (;i<16;i++){
        //     if (process_manager_state.list_of_process[i]){
        //         maxPid = i;
        //     }
        // }
        // (struct ProcessControlBlock *)frame.cpu.general.ebx = (struct ProcessControlBlock *) _process_list;

        for (int j = 0; j < 16; j++)
        {
            if (process_manager_state.list_of_process[j])
            {
                puts((char *)"PROCESS INFO: \n", 15, BLUE);
                puts((char *)"ID: ", 4, GREEN);
                printInt(_process_list[j].metadata.pid);
                putchar((char *)"\n", GREEN);
                puts((char *)"NAME: ", 7, GREEN);
                uint8_t i = 0;
                for (; i < PROCESS_NAME_LENGTH_MAX && _process_list[j].metadata.name[i] != 0; i++)
                {
                    putchar((char *)_process_list[j].metadata.name + i, WHITE);
                }
                putchar((char *)"\n", WHITE);
                puts((char *)"STATUS: ", 8, WHITE);
                if (_process_list[j].metadata.state == 0)
                {
                    puts((char *)"READY ", 6, YELLOW);
                }
                else if (_process_list[j].metadata.state == 1)
                {
                    puts((char *)"RUNNING ", 8, GREEN);
                }
                else if (_process_list[j].metadata.state == 2)
                {
                    puts((char *)"BLOCKED ", 8, RED);
                }
                putchar((char *)"\n", WHITE);
            }
        }
        break;
    case 21:

        ;
        struct ProcessControlBlock pcb;
        int8_t is_clock;
        int8_t retcode;
        char *arg = (char *)frame.cpu.general.ebx;

        int8_t piddd = 0;
        if (arg[1] != '\0')
        {
            piddd += (arg[0] - '0') * 10;
            piddd += arg[1] - '0';
        }
        else
        {
            piddd += (arg[0] - '0');
        }

        pcb = _process_list[piddd];
        if (strcmp(pcb.metadata.name, "clock") == 0)
        {
            is_clock = 1;
        }
        retcode = 0;
        if (!strcmp(pcb.metadata.name,"shell")==0){
            retcode = process_destroy(piddd);
            if (retcode)
            {
                puts((char *)"Proses Berhasil dihapus!\n", 25, GREEN);
                if (is_clock)
                {
                    putTime((char *)"  ", (char *)"  ", (char *)"  ", ' ');
                }
            }
            else
            {
                puts((char *)"Proses Tidak Berhasil dihapus!\n", 31, RED);
            }
        }else{
             puts((char *)"Proses Shell tidak Diizinkan untuk Dihapus!\n", 44, RED);
        }
       
        break;

    case 22:

        for (int8_t i = 0; i < 20; i++)
        {
            framebuffer_write(11, i + 30, ' ', WHITE, DARK_GRAY);
            framebuffer_write(12, i + 30, ' ', WHITE, DARK_GRAY);
            framebuffer_write(13, i + 30, ' ', WHITE, DARK_GRAY);

            for (int64_t j = 0; j < 10; j++)
            {
            }
        }
        for (int8_t i = 0; i < 20; i++)
        {
            framebuffer_write(11, i + 30, ' ', WHITE, YELLOW);
            framebuffer_write(12, i + 30, ' ', WHITE, YELLOW);
            framebuffer_write(13, i + 30, ' ', WHITE, YELLOW);

            for (int64_t j = 0; j < 10000000; j++)
            {
            }
        }
        framebuffer_clear();
        // Huruf H
        framebuffer_write(7, 30, ' ', WHITE, YELLOW);
        framebuffer_write(8, 30, ' ', WHITE, YELLOW);
        framebuffer_write(9, 30, ' ', WHITE, YELLOW);
        framebuffer_write(10, 30, ' ', WHITE, YELLOW);
        framebuffer_write(11, 30, ' ', WHITE, YELLOW);
        framebuffer_write(12, 30, ' ', WHITE, YELLOW);
        framebuffer_write(13, 30, ' ', WHITE, YELLOW);
        framebuffer_write(7, 31, ' ', WHITE, YELLOW);
        framebuffer_write(8, 31, ' ', WHITE, YELLOW);
        framebuffer_write(9, 31, ' ', WHITE, YELLOW);
        framebuffer_write(10, 31, ' ', WHITE, YELLOW);
        framebuffer_write(11, 31, ' ', WHITE, YELLOW);
        framebuffer_write(12, 31, ' ', WHITE, YELLOW);
        framebuffer_write(13, 31, ' ', WHITE, YELLOW);

        framebuffer_write(7, 32, ' ', WHITE, DARK_GREEN);
        framebuffer_write(8, 32, ' ', WHITE, DARK_GREEN);
        framebuffer_write(9, 32, ' ', WHITE, DARK_GREEN);
        framebuffer_write(11, 32, ' ', WHITE, DARK_GREEN);
        framebuffer_write(12, 32, ' ', WHITE, DARK_GREEN);
        framebuffer_write(13, 32, ' ', WHITE, DARK_GREEN);
        framebuffer_write(14, 32, ' ', WHITE, DARK_GREEN);
        framebuffer_write(14, 31, ' ', WHITE, DARK_GREEN);
        framebuffer_write(14, 30, ' ', WHITE, DARK_GREEN);

        framebuffer_write(10, 32, ' ', WHITE, YELLOW);
        framebuffer_write(10, 33, ' ', WHITE, YELLOW);
        framebuffer_write(10, 34, ' ', WHITE, YELLOW);
        framebuffer_write(10, 35, ' ', WHITE, YELLOW);
        framebuffer_write(10, 36, ' ', WHITE, YELLOW);

        framebuffer_write(11, 32, ' ', WHITE, DARK_GREEN);
        framebuffer_write(11, 33, ' ', WHITE, DARK_GREEN);
        framebuffer_write(11, 34, ' ', WHITE, DARK_GREEN);
        framebuffer_write(11, 35, ' ', WHITE, DARK_GREEN);
        framebuffer_write(11, 36, ' ', WHITE, DARK_GREEN);

        framebuffer_write(7, 35, ' ', WHITE, YELLOW);
        framebuffer_write(8, 35, ' ', WHITE, YELLOW);
        framebuffer_write(9, 35, ' ', WHITE, YELLOW);
        framebuffer_write(10, 35, ' ', WHITE, YELLOW);
        framebuffer_write(11, 35, ' ', WHITE, YELLOW);
        framebuffer_write(12, 35, ' ', WHITE, YELLOW);
        framebuffer_write(13, 35, ' ', WHITE, YELLOW);
        framebuffer_write(7, 36, ' ', WHITE, YELLOW);
        framebuffer_write(8, 36, ' ', WHITE, YELLOW);
        framebuffer_write(9, 36, ' ', WHITE, YELLOW);
        framebuffer_write(10, 36, ' ', WHITE, YELLOW);
        framebuffer_write(11, 36, ' ', WHITE, YELLOW);
        framebuffer_write(12, 36, ' ', WHITE, YELLOW);
        framebuffer_write(13, 36, ' ', WHITE, YELLOW);

        framebuffer_write(7, 37, ' ', WHITE, DARK_GREEN);
        framebuffer_write(8, 37, ' ', WHITE, DARK_GREEN);
        framebuffer_write(9, 37, ' ', WHITE, DARK_GREEN);
        framebuffer_write(10, 37, ' ', WHITE, DARK_GREEN);
        framebuffer_write(11, 37, ' ', WHITE, DARK_GREEN);
        framebuffer_write(12, 37, ' ', WHITE, DARK_GREEN);
        framebuffer_write(13, 37, ' ', WHITE, DARK_GREEN);
        framebuffer_write(14, 37, ' ', WHITE, DARK_GREEN);
        framebuffer_write(14, 36, ' ', WHITE, DARK_GREEN);
        framebuffer_write(14, 35, ' ', WHITE, DARK_GREEN);

        // huruf I
        framebuffer_write(7, 40, ' ', WHITE, YELLOW);
        framebuffer_write(8, 40, ' ', WHITE, YELLOW);
        framebuffer_write(9, 40, ' ', WHITE, YELLOW);
        framebuffer_write(10, 40, ' ', WHITE, YELLOW);
        framebuffer_write(11, 40, ' ', WHITE, YELLOW);
        framebuffer_write(12, 40, ' ', WHITE, YELLOW);
        framebuffer_write(13, 40, ' ', WHITE, YELLOW);
        framebuffer_write(7, 41, ' ', WHITE, YELLOW);
        framebuffer_write(8, 41, ' ', WHITE, YELLOW);
        framebuffer_write(9, 41, ' ', WHITE, YELLOW);
        framebuffer_write(10, 41, ' ', WHITE, YELLOW);
        framebuffer_write(11, 41, ' ', WHITE, YELLOW);
        framebuffer_write(12, 41, ' ', WHITE, YELLOW);
        framebuffer_write(13, 41, ' ', WHITE, YELLOW);

        framebuffer_write(7, 42, ' ', WHITE, DARK_GREEN);
        framebuffer_write(8, 42, ' ', WHITE, DARK_GREEN);
        framebuffer_write(9, 42, ' ', WHITE, DARK_GREEN);
        framebuffer_write(10, 42, ' ', WHITE, DARK_GREEN);
        framebuffer_write(11, 42, ' ', WHITE, DARK_GREEN);
        framebuffer_write(12, 42, ' ', WHITE, DARK_GREEN);
        framebuffer_write(13, 42, ' ', WHITE, DARK_GREEN);
        framebuffer_write(13, 42, ' ', WHITE, DARK_GREEN);
        framebuffer_write(14, 42, ' ', WHITE, DARK_GREEN);
        framebuffer_write(14, 41, ' ', WHITE, DARK_GREEN);
        framebuffer_write(14, 40, ' ', WHITE, DARK_GREEN);

        for (int8_t i = 0; i < 10; i++)
        {

            for (int64_t j = 0; j < 10000000; j++)
            {
            }
        }

        break;
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
