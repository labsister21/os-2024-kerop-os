#include <stdint.h>
#include "header/cpu/interrupt.h"
#include "header/filesystem/fat32.h"
#include "header/stdlib/string.h"
#include "header/driver/disk.h"
#include "header/driver/keyboard.h"

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
char *custom_strchr(const char *str, int c)
{
    while (*str != '\0')
    {
        if (*str == c)
        {
            return (char *)str;
        }
        str++;
    }

    return NULL;
}
char *strtok(char *str, const char *delim)
{
    static char *next_token = NULL;
    char *token_start;

    if (!str)
    {
        str = next_token;
        if (!str)
        {
            return NULL;
        }
    }

    while (*str && custom_strchr(delim, *str))
    {
        str++;
    }
    if (*str == '\0')
    {
        next_token = NULL;
        return NULL;
    }

    token_start = str;
    while (*str && !custom_strchr(delim, *str))
    {
        str++;
    }
    if (*str)
    {
        *str = '\0';
        next_token = str + 1;
    }
    else
    {
        next_token = NULL;
    }

    return token_start;
}

int atoi(const char *str)
{
    int result = 0;
    int sign = 1;

    while (*str == ' ' || (*str >= 9 && *str <= 13))
    {
        str++;
    }

    if (*str == '-')
    {
        sign = -1;
        str++;
    }
    else if (*str == '+')
    {
        str++;
    }

    while (*str >= '0' && *str <= '9')
    {
        result = result * 10 + (*str - '0');
        str++;
    }

    return result * sign;
}

int strcmp(char *s1, char *s2)
{
    int i = 0;

    do
    {
        if (s1[i] != s2[i])
        {
            return s1[i] - s2[i];
        }
        i++;
    } while (s1[i] != '\0');

    return 0;
}

void mkdir_command(char *dirname, uint32_t parent_cluster_number)
{

    struct FAT32DriverRequest request;
    request.buf = NULL;
    for (int8_t i = 0; i < 8; i++)
    {
        request.name[i] = dirname[i];
    }
    for (int8_t i = 0; i < 3; i++)
    {
        request.ext[i] = '\0';
    }
    request.parent_cluster_number = parent_cluster_number;
    request.buffer_size = 0;

    syscall_user(2, (uint32_t)&request, 0, 0);
}
void ls(uint32_t *dir_stack, uint8_t *dir_stack_index, char (*dir_name_stack)[8]){
    struct FAT32DirectoryTable now_table;
    struct FAT32DriverRequest req={
        .buf = &now_table,
        .ext = "\0\0\0",
        .buffer_size = 0,

    };
}
void cede(char *dirname, uint32_t *dir_stack, uint8_t *dir_stack_index, char (*dir_name_stack)[8])

{
    struct FAT32DirectoryTable req_table;
    struct FAT32DriverRequest request = {
        .buf = &req_table,
        .parent_cluster_number = dir_stack[*dir_stack_index],
        .ext = "\0\0\0",
        .buffer_size = 0,
    };
    char ddot[2] = "..";
    if (strcmp(dirname, ddot) == 0)
    {
        if (*dir_stack_index <= 0)
        {
            return;
        }
        else
        {
            (*dir_stack_index)--;
        }
    }
    else if (strcmp(dirname, ".") == 0)
    {
        return;
    }
    else
    {
        for (int i = 0; i < 8; i++)
        {
            request.name[i] = dirname[i];
        }

        int8_t retcode;
        syscall_user(1, (uint32_t)&request, (uint32_t)&retcode, 0);
        syscall_user(10, (uint32_t)&req_table, dir_stack[*dir_stack_index], 0);

        if (retcode != 0)
        {
            char err[18] = "INVALID DIRECTORY\n";
            syscall_user(6, (uint32_t)err, 18, RED);
            return;
        }
        for (uint8_t j = 0; j < 8; j++)
        {
            dir_name_stack[*dir_stack_index][j] = dirname[j];
        }
        for (uint32_t i = 0; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++)
        {
            if (strcmp(req_table.table[i].name, request.name) == 0)

            {
                if (req_table.table[i].attribute == ATTR_SUBDIRECTORY && req_table.table[i].filesize == 0)
                {
                    (*dir_stack_index)++;
                    dir_stack[*dir_stack_index] = (req_table.table[i].cluster_high << 16) | req_table.table[i].cluster_low;
                }
            }
        }
    }
}


void exec_command( uint32_t *dir_stack, uint8_t *dir_stack_index, char (*dir_name_stack)[8])

{
    char buff[MAX_INPUT_BUFFER];
    int i = 0;
    char input = 'a';
    do{
        syscall_user(4, (uint32_t)&input, 0, 0);
        if (input!=0 && input != '\n'){
            buff[i] = input;
            i+=1;
        }
        syscall_user(5,(uint32_t)&input,0xF,0);
    }while(input!='\n');


    char *args[MAX_ARGS];
    int argc = 0;
    char delim[1] = " ";
    char *token = strtok(buff, delim);
    while (token != NULL && argc < MAX_ARGS)
    {
        args[argc++] = token;
        token = strtok(NULL, delim);
    }
    char cd[2] = "cd";
    char ls[2] = "ls";
    char mkdir[5] = "mkdir";
    char cat[3] = "cat";
    char cls[3] = "cls";

    if (strcmp(args[0], cd) == 0)
    {
        char *dirname = args[1];
        cede(dirname, dir_stack, dir_stack_index, dir_name_stack);
    }
    else if (strcmp(args[0], ls) == 0)
    {
        // Handle ls command
    }
    else if (strcmp(args[0], mkdir) == 0)
    {
        if (argc >= 2)
        {
            char *dirname = args[1];
            mkdir_command(dirname, dir_stack[*dir_stack_index]);
        }
        else
        {
            char errorcode[33] = "[ERROR]: Usage: mkdir <dirname>\n";
            syscall_user(6, (uint32_t)errorcode, 33, RED);
        }
    }
    else if (strcmp(args[0], cat) == 0)
    {
        // Handle cat command
    }
    else if (strcmp(args[0], cls) == 0)
    {
        syscall_user(9, 0, 0, 0);
    }

    else
    {
        char errorcode2[28] = "[ERROR]: Invalid Command !\n";
        syscall_user(6, (uint32_t)errorcode2, 28, RED);
    }
    for (int8_t i = 0; i < MAX_INPUT_BUFFER; i++)
    {
        buff[i] = 0x0;
    }
}

int main(void)
{
    uint32_t DIR_NUMBER_STACK[MAX_DIR_STACK_SIZE];
    char DIR_NAME_STACK[MAX_DIR_STACK_SIZE][8];
    uint8_t DIR_STACK_INDEX = 0;
    DIR_NUMBER_STACK[DIR_STACK_INDEX] = ROOT_CLUSTER_NUMBER;

    syscall_user(7, 0, 0, 0);
 
    
    while (true)
    {
        char intro[10] = "kerop-os/";
        char dolar[3] = " $ ";
        syscall_user(6, (uint32_t)intro, 10, GREEN);
        for (int i = 0; i < DIR_STACK_INDEX; i++)
        {
            char path[8];
            for (int j = 0; j < 8; j++)
            {
                path[j] = DIR_NAME_STACK[i][j];

            }
            
            syscall_user(6,(uint32_t)path,8,DARK_GREEN); 
            syscall_user(6,(uint32_t)"/",1,GREEN);
        }
        syscall_user(6,(uint32_t)dolar,3,GREEN);
        
        exec_command(DIR_NUMBER_STACK, &DIR_STACK_INDEX,DIR_NAME_STACK);
    }

    return 0;
}