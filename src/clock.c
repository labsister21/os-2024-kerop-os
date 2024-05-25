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
        uint8_t hour;
        uint8_t min;
        uint8_t sec;
        syscall_user(17, (uint32_t)&flag, 0, 0);
        syscall_user(18, (uint32_t)&hour, (uint32_t)&min, (uint32_t)&sec);
        hour = (hour+7)%24;
        char hourc[2] = "\0\0";
        char minc[2] = "\0\0";
        char secc[2] = "\0\0";
        hourc[0] = (hour/10) + '0';
        hourc[1] = (hour%10) + '0';
        minc[0] = (min/10) + '0';
        minc[1] = (min%10) + '0';
        secc[0] = (sec/10) + '0';
        secc[1] = (sec%10) + '0';
        syscall_user(19,(uint32_t)hourc,(uint32_t)minc,(uint32_t)secc);
        // syscall_user(6, (uint32_t)seconds, 2, 0x0F);
    }
    // syscall_user(6, (uint32_t) "Ini clock", 9, 0x0C);

    return 0;
}