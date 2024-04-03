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

void kernel_setup(void)
{
    load_gdt(&_gdt_gdtr);
    pic_remap();
    initialize_idt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);
    __asm__("int $0x4");
    while (true)
        ;
}
// void kernel_setup(void)
// {
//     load_gdt(&_gdt_gdtr);
//     pic_remap();
//     initialize_idt();
//     framebuffer_clear();
//     framebuffer_set_cursor(0, 0);

//     struct BlockBuffer b;
//     for (int i = 0; i < 512; i++)
//         b.buf[i] = i;
//     write_blocks(&b, 17, 1);
//     while (true)
//         ;
// }

//  void kernel_setup(void) {
//     load_gdt(&_gdt_gdtr);
//     pic_remap();
//     initialize_idt();
//     activate_keyboard_interrupt();
//     framebuffer_clear();
//     framebuffer_set_cursor(0, 0);
        
//     int col = 0;
//     keyboard_state_activate();
//     while (true) {
//          char c;
//          get_keyboard_buffer(&c);
//          if (c) framebuffer_write(0, col++, c, 0xF, 0);
//     }
// }
