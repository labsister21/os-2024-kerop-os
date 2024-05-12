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
void ls(uint32_t *dir_stack, uint8_t *dir_stack_index, char (*dir_name_stack)[8])
{
    struct FAT32DirectoryTable now_table;
    struct FAT32DriverRequest req = {
        .buf = &now_table,
        .ext = "\0\0\0",
        .buffer_size = 0,

    };

    // ambil nama folder di stack skrg
    for (uint8_t i = 0; i < 8; i++)
    {
        req.name[i] = dir_name_stack[*dir_stack_index-1][i];
    }

    if (*dir_stack_index <= 1)
    {
        req.parent_cluster_number = ROOT_CLUSTER_NUMBER;
    }
    else
    {
        req.parent_cluster_number = dir_stack[*dir_stack_index-2];
    }

    // read cd
    int8_t errorcde;
    syscall_user(1, (uint32_t)&req, (uint32_t)&errorcde, 0);
    if (errorcde == 0)
    {
        for (uint32_t i = 1; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry);
             i++)
        {
            if (now_table.table[i].user_attribute == UATTR_NOT_EMPTY)
            {
                char filename[9];
                for (int j = 0; j < 8; j++)
                {
                    filename[j] = now_table.table[i].name[j];
                }
                filename[8] = '\0';
                //Kalo file
                if (now_table.table[i].attribute == ATTR_ARCHIVE)
                {
                    syscall_user(6, (uint32_t)filename, 8, WHITE);

                    if (now_table.table[i].ext[0] != '\0')
                    {
                        char ext_name[4];
                        for (int j = 0; j < 3; j++)
                        {
                            ext_name[j] = now_table.table[i].ext[j];
                        }
                        ext_name[3] = '\0';

                        syscall_user(6, (uint32_t) ".", 1, WHITE);
                        syscall_user(6, (uint32_t)ext_name, 3, WHITE);
                    }
                }

                //Kalo folder
                else
                {
                    syscall_user(6, (uint32_t)filename, 8, LIGHT_PURPLE);
                }

                syscall_user(6, (uint32_t) " ", 1, WHITE);
            }
        }
    }else{
        syscall_user(6,(uint32_t)"ERROR!\n",7,RED);
    }
    syscall_user(6, (uint32_t)"\n", 1, WHITE);
}
void cede(char *dirname, uint32_t *dir_stack, uint8_t *dir_stack_index, char (*dir_name_stack)[8])

{
    struct FAT32DirectoryTable req_table;
    struct FAT32DriverRequest request = {
        .buf = &req_table,
        .parent_cluster_number = dir_stack[*dir_stack_index-1],
        .ext = "\0\0\0",
        .buffer_size = 0,
    };
    char ddot[2] = "..";
    if (strcmp(dirname, ddot) == 0)
    {
        if (*dir_stack_index <= 1)
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
        syscall_user(10, (uint32_t)&req_table, dir_stack[*dir_stack_index-1], 0);

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
                    
                    dir_stack[*dir_stack_index] = (req_table.table[i].cluster_high << 16) | req_table.table[i].cluster_low;
                    (*dir_stack_index)++;
                }
            }
        }
    }
}

void ket(char* Filename, uint32_t *dir_stack, uint8_t *dir_stack_index,  char (*dir_name_stack)[8]){
  // cat : Menuliskan sebuah file sebagai text file ke layar (Gunakan format
    int parseId = 0;
      // LF newline)
    if (strcmp("./",Filename)==0){
        Filename = Filename + 2;
    }
    if (strcmp("../",Filename)==0){
        do {
            parseId += 1;
            Filename = Filename + 3;
        }while (strcmp("../",Filename)==0);
        if (*dir_stack_index <= parseId){
            syscall_user(6, (uint32_t)"INVALID FILE PATH\n", 18, RED);
            return;
        }
    }
    struct FAT32DirectoryTable parent_dir;
    struct FAT32DriverRequest request = {
        .buf = &parent_dir,
        .parent_cluster_number = dir_stack[*dir_stack_index-(parseId+1)],
        .ext = "\0\0\0",
        .buffer_size = 0,
    };
    uint8_t i = 0;
    for (;i<8;i++){
        request.name[i] = dir_name_stack[*dir_stack_index-(parseId+1)][i];
    }
    int8_t retcode;
    syscall_user(1,(uint32_t)&request,(uint32_t)&retcode,0);
    syscall_user(10, (uint32_t)&parent_dir, dir_stack[*dir_stack_index-(parseId+1)], 0);
    if (retcode!=0){
        syscall_user(6, (uint32_t)"FILE NOT FOUND\n", 15, RED);
        return;
    }
    char realFileName[8] = "\0\0\0\0\0\0\0\0";
    parseId = 0;
    while (strcmp(".",Filename+parseId)!=0 && parseId < 8){
        realFileName[parseId] = Filename[parseId];
        parseId += 1;
    }
    parseId += 1;
    if (parseId > 8){
        syscall_user(6, (uint32_t)"INVALID FILE NAME\n", 18, RED);
        return;
    }else if (strcmp("txt",Filename+parseId)!=0){
        syscall_user(6, (uint32_t)"INVALID FILE EXTENSION\n", 23, RED);
        return;
    }
    parseId = 0;
    for (;parseId<64;parseId++){
        if (strcmp(parent_dir.table[parseId].name,realFileName)==0 && strcmp("txt",parent_dir.table[parseId].ext)==0){
            struct ClusterBuffer fileBuff;

            struct FAT32DriverRequest request2 = {
            .buf = &fileBuff,
            .parent_cluster_number = request.parent_cluster_number,
            .ext = "txt",
            .buffer_size = parent_dir.table[parseId].filesize,
            };
            i = 0;
            for (;i<8;i++){
                request2.name[i] = realFileName[i];
            }
            syscall_user(0,(uint32_t)&request2,(uint32_t)&retcode,0);
            if (retcode==0){
                syscall_user(6,(uint32_t)fileBuff.buf,request2.buffer_size,WHITE);
                syscall_user(6, (uint32_t) "\n", 1, WHITE);
                return;
            }else{
                syscall_user(6, (uint32_t)"FAILED TO READ\n", 15, RED);
                return;
            }
            

        }
    }   
}

void exec_command(uint32_t *dir_stack, uint8_t *dir_stack_index, char (*dir_name_stack)[8])

{
    char buff[MAX_INPUT_BUFFER];
    int i = 0;
    char input = 'a';
    do
    {
        syscall_user(4, (uint32_t)&input, 0, 0);
        if (!(input=='\b' && i==0)){
            syscall_user(5, (uint32_t)&input, 0xF, 0);
        }
        if (input != 0 && input != '\n')
        {
            if (input=='\b' && i==0){
                // do nothing
            }else if (input=='\b' && i>0) {
                buff[i-1] = ' ';
                i -= 1;
            }else{
                buff[i] = input;
                i += 1;
            }
            if (i==2){
                if (buff[0]=='l' && buff[1]=='s'){
                    syscall_user(5, (uint32_t)"\b", 0xF, 0);
                    syscall_user(5, (uint32_t)"\b", 0xF, 0);
                    syscall_user(5, (uint32_t)"l", BLUE, 0);
                    syscall_user(5, (uint32_t)"s", BLUE, 0);
                }
            }else if (i==3){
                if (buff[0]=='c' && buff[1]=='d' && buff[2]==' '){
                    syscall_user(5, (uint32_t)"\b", 0xF, 0);
                    syscall_user(5, (uint32_t)"\b", 0xF, 0);
                    syscall_user(5, (uint32_t)"\b", 0xF, 0);
                    syscall_user(5, (uint32_t)"c", YELLOW, 0);
                    syscall_user(5, (uint32_t)"d", YELLOW, 0);
                    syscall_user(5, (uint32_t)" ", WHITE, 0);

                } 
            }else if (i==4){
                if (buff[0]=='c' && buff[1]=='a' && buff[2]=='t' && buff[3]==' ' ){
                    syscall_user(5, (uint32_t)"\b", 0xF, 0);
                    syscall_user(5, (uint32_t)"\b", 0xF, 0);
                    syscall_user(5, (uint32_t)"\b", 0xF, 0);
                    syscall_user(5, (uint32_t)"\b", 0xF, 0);
                    syscall_user(5, (uint32_t)"c", AQUA, 0);
                    syscall_user(5, (uint32_t)"a", AQUA, 0);
                    syscall_user(5, (uint32_t)"t", AQUA, 0);
                    syscall_user(5, (uint32_t)" ", WHITE, 0);

                }
            }else if (i==6){
                if (buff[0]=='m'&& buff[1]=='k' && buff[2]=='d'&& buff[3]=='i'&& buff[4]=='r' && buff[5]==' '){
                    syscall_user(5, (uint32_t)"\b", 0xF, 0);
                    syscall_user(5, (uint32_t)"\b", 0xF, 0);
                    syscall_user(5, (uint32_t)"\b", 0xF, 0);
                    syscall_user(5, (uint32_t)"\b", 0xF, 0);
                    syscall_user(5, (uint32_t)"\b", 0xF, 0);
                    syscall_user(5, (uint32_t)"\b", 0xF, 0);
                    syscall_user(5, (uint32_t)"m", GOLD, 0);
                    syscall_user(5, (uint32_t)"k", GOLD, 0);
                    syscall_user(5, (uint32_t)"d", GOLD, 0);
                    syscall_user(5, (uint32_t)"i", GOLD, 0);
                    syscall_user(5, (uint32_t)"r", GOLD, 0);
                    syscall_user(5, (uint32_t)" ", WHITE, 0);
                }
            }
        }
       
      
    } while (input != '\n');

    char *args[MAX_ARGS];
    int argc = 0;
    char delim[1] = " ";
    char *token = strtok(buff, delim);
    while (token != NULL && argc < MAX_ARGS)
    {
        args[argc++] = token;
        token = strtok(NULL, delim);
    }

    if (strcmp("cd",args[0]) == 0)
    {

        char *dirname = args[1];
        cede(dirname, dir_stack, dir_stack_index, dir_name_stack);
    }
    else if (strcmp("ls",args[0]) == 0)
    {
        ls(dir_stack,dir_stack_index,dir_name_stack);
    }
    else if (strcmp("mkdir",args[0]) == 0)
    {
        if (argc >= 2)
        {
           
            char *dirname = args[1];
            mkdir_command(dirname, dir_stack[*dir_stack_index-1]);
        }
        else
        {
            char errorcode[33] = "[ERROR]: Usage: mkdir <dirname>\n";
            syscall_user(6, (uint32_t)errorcode, 33, RED);
        }
    }
    else if (strcmp("cat",args[0]) == 0)
    {
        // Handle cat command
        char *filename = args[1];
        ket(filename, dir_stack, dir_stack_index,dir_name_stack);

    }
    else if (strcmp("clear",args[0]) == 0)
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
    char DIR_NAME_STACK[MAX_DIR_STACK_SIZE][8] ={"ROOT\0\0\0\0"};
    DIR_NUMBER_STACK[0] = ROOT_CLUSTER_NUMBER;
    uint8_t DIR_STACK_INDEX = 1;


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

            syscall_user(6, (uint32_t)path, 8, DARK_GREEN);
            syscall_user(6, (uint32_t) "/", 1, GREEN);
        }
        syscall_user(6, (uint32_t)dolar, 3, GREEN);
       

        exec_command(DIR_NUMBER_STACK, &DIR_STACK_INDEX, DIR_NAME_STACK);
    }

    return 0;
}