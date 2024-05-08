#include <stdint.h>
#include "header/cpu/interrupt.h"
#include "header/filesystem/fat32.h"
#include "header/stdlib/string.h"
#include "header/driver/disk.h"
#include "header/driver/keyboard.h"
uint32_t DIR_NUMBER_STACK[256] = {2};
char DIR_NAME_STACK[256][9] = {"ROOT\0\0\0\0\0"};
uint8_t DIR_STACK_LENGTH = 1;

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
#define KEYBOARD_BUFFER_SIZE 256

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

void change_directory(char *new_dir)
{
    struct FAT32DirectoryTable req_table;
    struct FAT32DriverRequest request = {
        .buf = &req_table,
        .parent_cluster_number = DIR_NUMBER_STACK[DIR_STACK_LENGTH - 1],
        .ext = "\0\0\0",
        .buffer_size = 0,
    };

    if (memcmp(new_dir, "..", 2) == 0)
    {
        if (DIR_STACK_LENGTH <= 1)
        {
            return;
        }
        else
        {
            DIR_STACK_LENGTH--;
        }
    }
    else if (memcmp(new_dir, ".", 1) == 0)
    {
        return;
    }
    else
    {
        for (int i = 0; i < 8; i++)
        {
            request.name[i] = new_dir[i];
        }

        int8_t retcode;
        syscall_user(1, (uint32_t)&request, (uint32_t)&retcode, 0);

        if (retcode != 0)
        {
            syscall_user(5, (uint32_t) "INVALID DIRECTORY\n", 18, DARK_GREEN);
            return;
        }

        for (int j = 0; j < 8; j++)
        {
            DIR_NAME_STACK[DIR_STACK_LENGTH][j] = request.name[j];
        }
        DIR_NAME_STACK[DIR_STACK_LENGTH][8] = '\0';
        DIR_NUMBER_STACK[DIR_STACK_LENGTH] =
            req_table.table[0].cluster_high << 16 | req_table.table[0].cluster_low;
        DIR_STACK_LENGTH++;
    }
}
void list_current_directory()
{
    struct FAT32DirectoryTable current_table;
    struct FAT32DriverRequest request = {
        .buf = &current_table,
        .ext = "\0\0\0",
        .buffer_size = 0,
    };

    for (int i = 0; i < 8; i++)
    {
        request.name[i] = DIR_NAME_STACK[DIR_STACK_LENGTH - 1][i];
    }

    if (DIR_STACK_LENGTH <= 1)
    {
        request.parent_cluster_number = ROOT_CLUSTER_NUMBER;
    }
    else
    {
        request.parent_cluster_number = DIR_NUMBER_STACK[DIR_STACK_LENGTH - 2];
    }

    int8_t retcode;
    syscall_user(1, (uint32_t)&request, (uint32_t)&retcode, 0);

    if (retcode != 0)
    {
        syscall_user(5, (uint32_t) "SHELL ERROR\n", 6, DARK_GREEN);
    }

    for (uint32_t i = 1; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry);
         i++)
    {
        if (current_table.table[i].user_attribute == UATTR_NOT_EMPTY)
        {
            char filename[9];
            for (int j = 0; j < 8; j++)
            {
                filename[j] = current_table.table[i].name[j];
            }
            filename[8] = '\0';

            if (current_table.table[i].attribute == ATTR_ARCHIVE)
            {
                syscall_user(5, (uint32_t)filename, 8, WHITE);

                if (current_table.table[i].ext[0] != '\0')
                {
                    char ext_name[4];
                    for (int j = 0; j < 3; j++)
                    {
                        ext_name[j] = current_table.table[i].ext[j];
                    }
                    ext_name[3] = '\0';

                    syscall_user(5, (uint32_t) ".", 1, WHITE);
                    syscall_user(5, (uint32_t)ext_name, 3, WHITE);
                }
            }
            else
            {
                syscall_user(5, (uint32_t)filename, 8, AQUA);
            }

            syscall_user(5, (uint32_t) " ", 1, WHITE);
        }
    }
}
int main(void)
{
    struct ClusterBuffer cl[2] = {0};
    struct FAT32DriverRequest request = {
        .buf = &cl,
        .name = "shell",
        .ext = "\0\0\0",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size = CLUSTER_SIZE,
    };
    int32_t retcode;
    syscall_user(0, (uint32_t)&request, (uint32_t)&retcode, 0);
    if (retcode == 0)
        syscall_user(6, (uint32_t) "owo\n", 4, 0xF);

    char buf;
    keyboard_state_activate();
    while (true)
    {
        syscall_user(4, (uint32_t)&buf, 0, 0);
        syscall_user(5, (uint32_t)&buf, 0xF, 0);
    }

    return 0;
}
