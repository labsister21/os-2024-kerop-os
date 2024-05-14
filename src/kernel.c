#include <stdint.h>
#include "header/cpu/gdt.h"
#include "header/kernel-entrypoint.h"
#include "header/cpu/portio.h"
#include "header/text/framebuffer.h"
#include "header/stdlib/string.h"
#include "header/cpu/idt.h"
#include "header/cpu/interrupt.h"
#include "header/filesystem/fat32.h"
#include "header/driver/disk.h"
#include "header/driver/keyboard.h"
#include "header/memory/paging.h"
#include "header/process/process.h"
#include <stdbool.h>

// void kernel_setup(void)
// {

//     uint32_t volatile b = 0x0000BABE;
//     load_gdt(&_gdt_gdtr);

//     framebuffer_clear();
//     framebuffer_write(3, 8, 'H', 0, 0xF);
//     framebuffer_write(3, 9, 'a', 0, 0xF);
//     framebuffer_write(3, 10, 'i', 0, 0xF);
//     framebuffer_write(3, 11, '!', 0, 0xF);
//     framebuffer_set_cursor(3, 10);
//     while (true)
//         ;
//     b += 1;
// }

// void kernel_setup(void)
// {
//     load_gdt(&_gdt_gdtr);
//     pic_remap();
//     initialize_idt();
//     framebuffer_clear();
//     framebuffer_set_cursor(0, 0);
//     __asm__("int $0x4");
//     while (true)
//         ;
// }
// void kernel_setup(void)
// {
//     load_gdt(&_gdt_gdtr);
//     pic_remap();
//     initialize_idt();
//     framebuffer_clear();
//     framebuffer_set_cursor(0, 0);
//     activate_keyboard_interrupt();

//     struct FAT32DriverRequest req;
//     req.parent_cluster_number = 2;
//     req.buffer_size =  1;
//     memcpy(req.name,"kano\0\0\0\0",8);

//     keyboard_state_activate();

//     // struct BlockBuffer b;
//     // for (int i = 0; i < 512; i++)
//     //     b.buf[i] = i;
//     // write_blocks(&b, 17, 1);
//     do{
//         int8_t i = read_directory(req);
//         if (i==0){
//             framebuffer_write(0,2,'y',0xF,0);
//         }else if (i==1){
//             framebuffer_write(0,2,'n',0xF,0);
//         }else if (i==2){
//             framebuffer_write(0,2,'f',0xF,0);
//         }else{
//             framebuffer_write(0,2,'x',0xF,0);
//         }
//     }while(true);
// }

// void kernel_setup(void)
// {
//     load_gdt(&_gdt_gdtr);
//     pic_remap();
//     initialize_idt();
//     activate_keyboard_interrupt();
//     framebuffer_clear();
//     framebuffer_set_cursor(0, 0);

//     int col = 0;
//     keyboard_state_activate();
//     while (true)
//     {
//         char c;
//         get_keyboard_buffer(&c);
//         if (c)
//             framebuffer_write(0, col++, c, 0xF, 0);
//     }
// }

/* void kernel_setup(void)
{
    load_gdt(&_gdt_gdtr);
    pic_remap();
    initialize_idt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);
    initialize_filesystem_fat32();
    activate_keyboard_interrupt();
    keyboard_state_activate();

    struct FAT32DriverRequest req =
        {
            .buf = "wawaaw",
            .name = "hihuha",
            .parent_cluster_number = 2,
            .buffer_size = sizeof(req.buf)};
    delete(req);
    while (true)
        ;
} */

// void kernel_setup(void) {
//     load_gdt(&_gdt_gdtr);
//     pic_remap();
//     initialize_idt();
//     activate_keyboard_interrupt();
//     framebuffer_clear();
//     framebuffer_set_cursor(0, 0);
//     initialize_filesystem_fat32();
//     gdt_install_tss();
//     set_tss_register();

//     // Allocate first 4 MiB virtual memory
//     paging_allocate_user_page_frame(&_paging_kernel_page_directory, (uint8_t*) 0);

//     // Write shell into memory
//     struct FAT32DriverRequest request = {
//         .buf                   = (uint8_t*) 0,
//         .name                  = "shell",
//         .ext                   = "\0\0\0",
//         .parent_cluster_number = ROOT_CLUSTER_NUMBER,
//         .buffer_size           = 0x100000,
//     };
//     read(request);

//     // Set TSS $esp pointer and jump into shell
//     set_tss_kernel_current_stack();
//     kernel_execute_user_program((uint8_t*) 0);

//     while (true);
// }

// void kernel_setup(void)
// {
//     load_gdt(&_gdt_gdtr);
//     pic_remap();
//     initialize_idt();
//     activate_keyboard_interrupt();
//     framebuffer_clear();
//     framebuffer_set_cursor(0, 0);
//     initialize_filesystem_fat32();
//     gdt_install_tss();
//     set_tss_register();

//     // Allocate first 4 MiB virtual memory
//     paging_allocate_user_page_frame(&_paging_kernel_page_directory, (uint8_t *)0);

//     // Write shell into memory
//     struct FAT32DriverRequest request = {
//         .buf = (uint8_t *)0,
//         .name = "shell",
//         .ext = "\0\0\0",
//         .parent_cluster_number = ROOT_CLUSTER_NUMBER,
//         .buffer_size = sizeof(request.buf),
//     };

//     read(request);
//     if (read(request) == 0)
//     {
//         framebuffer_write(3, 9, '0', 0, 0xF);
//     }
//     else if (read(request) == 1)
//     {
//         framebuffer_write(3, 9, '1', 0, 0xF);
//     }
//     else if (read(request) == 2)
//     {
//         framebuffer_write(3, 9, '2', 0, 0xF);
//     }
//     else if (read(request) == 3)
//     {
//         framebuffer_write(3, 9, '3', 0, 0xF);
//     }
//     else
//     {
//         framebuffer_write(3, 9, '-', 0, 0xF);
//     }

//     // Set TSS $esp pointer and jump into shell
//     set_tss_kernel_current_stack();
//     kernel_execute_user_program((uint8_t *)0);

//     while (true)
//         ;
// }

void kernel_setup(void) {
    load_gdt(&_gdt_gdtr);
    pic_remap();
    initialize_idt();
    activate_keyboard_interrupt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);
    initialize_filesystem_fat32();
    gdt_install_tss();
    set_tss_register();

    // Shell request
    struct FAT32DriverRequest request = {
        .buf                   = (uint8_t*) 0,
        .name                  = "shell",
        .ext                   = "\0\0\0",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size           = 0x100000,
    };

    // Set TSS.esp0 for interprivilege interrupt
    set_tss_kernel_current_stack();

    // Create & execute process 0
    process_create_user_process(request);
    paging_use_page_directory(_process_list[0].context.page_directory_virtual_addr);
    kernel_execute_user_program((void*) 0x0);
}
