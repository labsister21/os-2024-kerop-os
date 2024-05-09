#include <stdint.h>
#include "header/cpu/interrupt.h"
#include "header/filesystem/fat32.h"
#include "header/stdlib/string.h"
#include "header/driver/disk.h"
#include "header/driver/keyboard.h"

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
    while (s1[i] == s2[i])
    {
        if (s2[i] == '\0')
        {
            return 0;
        }
        i++;
    }
    return s1[i] - s2[i];
}

void mkdir_command(char *dirname, uint32_t parent_cluster_number)
{
    
    struct FAT32DriverRequest request;
    request.buf = NULL;              
    memcpy(request.name, dirname, 8); 
    memcpy(request.ext, "\0\0\0", 3);
    request.parent_cluster_number = parent_cluster_number;
    request.buffer_size = 0; 

    
    syscall_user(2, (uint32_t)&request, 0, 0);
}
void exec_command()
{
    char buff[MAX_INPUT_BUFFER];
    int i = 0;
    syscall_user(6, (uint32_t) "kerop-os$", 10, GREEN);
    char input = 'a';
    do
    {
        syscall_user(4, (uint32_t)&input, 0, 0);
        if (input != 0 && input != '\n')
        {
            buff[i] = input;
            i += 1;
        }
        syscall_user(5, (uint32_t)&input, 0xF, 0);
    } while (input != '\n');

    // Null-terminate the input buffer
    buff[i] = '\0';

    // Parse the input buffer for command and arguments
    char *args[MAX_ARGS];
    int argc = 0;
    char *token = strtok(buff, " ");
    while (token != NULL && argc < MAX_ARGS)
    {
        args[argc++] = token;
        token = strtok(NULL, " ");
    }

    // syscall_user(5,(uint32_t)&input,0xF,0);
    // char cd[2] = "cd";
    // char ls[2] = "ls";
    // char mkdir[5] = "mkdir";
    // char cat[3] = "cat";
    // char cp[2] = "cp";
    // char rm[2] = "rm";
    // char mv[2] = "mv";
    // char find[4] = "find";
    // Execute command based on input
    if (strcmp("cd", args[0]) == 0)
    {
        // Handle cd command
    }
    else if (strcmp("ls", args[0]) == 0)
    {
        // Handle ls command
    }
    else if (strcmp("mkdir", args[0]) == 0)
    {
        if (argc >= 2)
        {
            char *dirname = args[1];
            uint32_t parent_cluster_number = ROOT_CLUSTER_NUMBER;
            mkdir_command(dirname, parent_cluster_number);
        }
        else
        {
           
            syscall_user(6, (uint32_t) "[ERROR]: Usage: mkdir <dirname>\n", 39, RED);
        }
    }
    else if (strcmp("cat", args[0]) == 0)
    {
        // Handle cat command
    }
    // Add other commands here...

    else
    {
        syscall_user(6, (uint32_t) "[ERROR]: Invalid Command !\n", 27, RED);
    }
    memset(buff, 0, sizeof(buff));
    i = 0;
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

    syscall_user(7, 0, 0, 0);
    syscall_user(0, (uint32_t)&request, (uint32_t)&retcode, 0);


    while (true)
    {
        exec_command();
    }

    return 0;
}
