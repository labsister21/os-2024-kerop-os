#include <stdint.h>
#include "header/cpu/gdt.h"
#include "header/kernel-entrypoint.h"
#include "header/cpu/portio.h"
#include "header/text/framebuffer.h"
#include "header/stdlib/string.h"
#include <stdbool.h>

void kernel_setup(void)
{
    uint32_t a;
    uint32_t volatile b = 0x0000BABE;
    load_gdt(&_gdt_gdtr);
    __asm__("mov $0xCAFE0000, %0" : "=r"(a));
    framebuffer_clear();
    framebuffer_write(3, 8, 'H', 0, 0xF);
    framebuffer_write(3, 9, 'a', 0, 0xF);
    framebuffer_write(3, 10, 'i', 0, 0xF);
    framebuffer_write(3, 11, '!', 0, 0xF);
    framebuffer_set_cursor(3, 10);
    while (true)
        ;
    b += 1;
}
