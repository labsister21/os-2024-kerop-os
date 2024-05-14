#include <stdint.h>
#include "header/cpu/interrupt.h"
#include "header/filesystem/fat32.h"
#include "header/stdlib/string.h"
#include "header/driver/disk.h"
#include "header/driver/keyboard.h"
#include "header/process/process.h"

void syscall_user(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx)
{
    __asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));
    // Note : gcc usually use %eax as intermediate register,
    //        so it need to be the last one to mov
    __asm__ volatile("int $0x30");
}

int main(void)
{
    int8_t flag;
    while (true)
    {
        syscall_user(17, (uint32_t)&flag, 0, 0);
        char hour[2];
        char minute[2];
        char seconds[2];
        syscall_user(18, (uint32_t)&seconds, (uint32_t)&minute, (uint32_t)&hour);
        syscall_user(6, (uint32_t)seconds, 2, 0x0F);
    }
    // syscall_user(6, (uint32_t) "Ini clock", 9, 0x0C);

    return 0;
}