#include <stdint.h>
#include "header/cpu/interrupt.h"
#include "header/filesystem/fat32.h"
#include "header/stdlib/string.h"
#include "header/driver/disk.h"
#include "header/driver/keyboard.h"
#include "header/process/process.h"

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
typedef struct
{
    char filename[9];
    uint32_t cluster_number;
    uint32_t parent_cluster_number;
    uint32_t clusterpathing[MAX_DIR_STACK_SIZE];
    uint32_t neff;

} FileInfo;
// Antriii
typedef struct
{
    FileInfo items[MAX_QUEUE_SIZE];
    int front;
    int rear;
} Queue;

void createQueue(Queue *queue)
{
    queue->front = -1;
    queue->rear = -1;
}

void enqueue(Queue *queue, FileInfo value)
{
    if ((queue->rear + 1) % MAX_QUEUE_SIZE == queue->front)
    {
        return;
    }
    if (queue->front == -1)
    {
        queue->front = 0;
    }
    queue->rear = (queue->rear + 1) % MAX_QUEUE_SIZE;

    for (int i = 0; i < 9; i++)
    {
        queue->items[queue->rear].filename[i] = value.filename[i];
    }

    queue->items[queue->rear].filename[8] = '\0';
    queue->items[queue->rear].cluster_number = value.cluster_number;
    queue->items[queue->rear].parent_cluster_number = value.parent_cluster_number;
    queue->items[queue->rear].neff = value.neff;
    for (int i = 0; i < MAX_DIR_STACK_SIZE; i++)
    {
        queue->items[queue->rear].clusterpathing[i] = value.clusterpathing[i];
    }
}

FileInfo dequeue(Queue *queue)
{
    FileInfo item;
    if (queue->front == -1)
    {

        for (int i = 0; i < 9; i++)
        {
            item.filename[i] = '\0';
        }
        item.cluster_number = 0;
        item.parent_cluster_number = 0;
        return item;
    }

    for (int i = 0; i < 9; i++)
    {
        item.filename[i] = queue->items[queue->front].filename[i];
    }

    item.filename[8] = '\0';
    item.cluster_number = queue->items[queue->front].cluster_number;
    item.parent_cluster_number = queue->items[queue->front].parent_cluster_number;
    item.neff = queue->items[queue->front].neff;
    for (int i = 0; i < MAX_DIR_STACK_SIZE; i++)
    {
        item.clusterpathing[i] = queue->items[queue->front].clusterpathing[i];
    }

    if (queue->front == queue->rear)
    {
        queue->front = -1;
        queue->rear = -1;
    }
    else
    {
        queue->front = (queue->front + 1) % MAX_QUEUE_SIZE;
    }
    return item;
}
void printInt(int num)
{
    // Check if num is 0
    char input = '0';
    if (num == 0)
    {
        syscall_user(5, (uint32_t)&input, WHITE, 0);
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
    for (;k>=0;k--){
        syscall_user(5, (uint32_t)(result+k), WHITE, 0);
    }
    return;
}
int isEmpty(Queue *queue)
{
    return (queue->front == -1);
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
    int8_t retcode;
    syscall_user(2, (uint32_t)&request, (uint32_t)&retcode, 0);
    if (retcode != 0)
    {
        syscall_user(6, (uint32_t) "[ERROR] FAILED TO MAKE DIR\n", 27, RED);
    }
}

void custom_strcpy(char *dest, const char *src)
{
    while (*src)
    {
        *dest++ = *src++;
    }
    *dest = '\0';
}

void custom_strcat(char *dest, const char *src)
{
    while (*dest)
    {
        dest++;
    }
    while (*src)
    {
        *dest++ = *src++;
    }
    *dest = '\0';
}

// Custom implementation of strlen
size_t custom_strlen(const char *str)
{
    size_t len = 0;
    while (*str++)
    {
        len++;
    }
    return len;
}
void eles(uint32_t *dir_stack, uint8_t *dir_stack_index, char (*dir_name_stack)[8])
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
        req.name[i] = dir_name_stack[*dir_stack_index - 1][i];
    }

    if (*dir_stack_index <= 1)
    {
        req.parent_cluster_number = ROOT_CLUSTER_NUMBER;
    }
    else
    {
        req.parent_cluster_number = dir_stack[*dir_stack_index - 2];
    }

    // read cd
    int8_t errorcde;
    syscall_user(1, (uint32_t)&req, (uint32_t)&errorcde, 0);
    syscall_user(10, (uint32_t)&now_table, dir_stack[*dir_stack_index - 1], 0);
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
                // Kalo file
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

                // Kalo folder
                else
                {
                    syscall_user(6, (uint32_t)filename, 8, LIGHT_PURPLE);
                }

                syscall_user(6, (uint32_t) " ", 1, WHITE);
            }
        }
    }
    else
    {
        syscall_user(6, (uint32_t) "ERROR!\n", 7, RED);
    }
    syscall_user(6, (uint32_t) "\n", 1, WHITE);
}
void cede(char *dirname, uint32_t *dir_stack, uint8_t *dir_stack_index, char (*dir_name_stack)[8])

{
    struct FAT32DirectoryTable req_table;
    struct FAT32DriverRequest request = {
        .buf = &req_table,
        .parent_cluster_number = dir_stack[*dir_stack_index - 1],
        .ext = "\0\0\0",
        .buffer_size = 0,
    };
    uint8_t parseId = 0;
    uint8_t charcount = 0;
    if (strcmp("../", dirname) == 0)
    {
        do
        {
            parseId++;
            dirname += 3;
            charcount += 3;

        } while (strcmp("../", dirname) == 0);

        if ((*dir_stack_index) - parseId <= 0)
        {
            (*dir_stack_index) = 1;
        }
        else
        {
            ((*dir_stack_index) -= parseId);
        }
    }
    else if (strcmp("..", dirname) == 0)
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

    for (int i = 0; i < 8; i++)
    {
        request.name[i] = dirname[i + charcount];
    }

    int8_t retcode;
    syscall_user(1, (uint32_t)&request, (uint32_t)&retcode, 0);
    syscall_user(10, (uint32_t)&req_table, dir_stack[*dir_stack_index - 1], 0);

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
    for (uint32_t i = 1; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++)
    {
        if (strcmp(req_table.table[i].name, request.name) == 0)

        {
            if (req_table.table[i].attribute == ATTR_SUBDIRECTORY && req_table.table[i].filesize == 0)
            {

                dir_stack[*dir_stack_index] = (req_table.table[i].cluster_high << 16) | req_table.table[i].cluster_low;
                (*dir_stack_index)++;
                return;
            }
        }
    }
    syscall_user(6, (uint32_t) "[ERROR] FILE NOT FOUND\n", 23, RED);
}
void bfs_find(char *target_name)
{
    Queue queue;
    createQueue(&queue);
    int8_t isFirst = 1;
    int8_t n_found = 0;

    FileInfo root_info;
    custom_strcpy(root_info.filename, "ROOT\0\0\0\0");
    root_info.cluster_number = ROOT_CLUSTER_NUMBER;
    root_info.parent_cluster_number = ROOT_CLUSTER_NUMBER;
    root_info.clusterpathing[0] = ROOT_CLUSTER_NUMBER;
    root_info.neff = 0;
    enqueue(&queue, root_info);

    while (!isEmpty(&queue))
    {
        FileInfo current_info = dequeue(&queue);

        // Read current directory contents
        struct FAT32DirectoryTable dir_table;
        struct FAT32DriverRequest req = {
            .buf = &dir_table,
            .parent_cluster_number = current_info.parent_cluster_number,
            .ext = "\0\0\0",
            .buffer_size = 0,
        };
        custom_strcpy(req.name, current_info.filename);
        int8_t error_code;
        syscall_user(1, (uint32_t)&req, (uint32_t)&error_code, 0);
        syscall_user(10, (uint32_t)&dir_table, current_info.cluster_number, 0);

        if (error_code == 0)
        {
            uint32_t i;
            if (isFirst)
            {
                i = 2; // mulai dari 2, karena 0 1 keisi ROOT dan shell
                isFirst = 0;
            }
            else
            {
                i = 1;
            }

            for (; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++)
            {
                if (dir_table.table[i].user_attribute == UATTR_NOT_EMPTY)
                {

                    if (dir_table.table[i].attribute == ATTR_SUBDIRECTORY)
                    {
                        FileInfo sub_dir_info;
                        custom_strcpy(sub_dir_info.filename, dir_table.table[i].name);
                        sub_dir_info.cluster_number = (dir_table.table[i].cluster_high << 16) | dir_table.table[i].cluster_low;
                        sub_dir_info.parent_cluster_number = current_info.cluster_number;

                        sub_dir_info.neff = current_info.neff + 1;
                        for (int i = 0; i < MAX_DIR_STACK_SIZE; i++)
                        {
                            sub_dir_info.clusterpathing[i] = current_info.clusterpathing[i];
                        }
                        sub_dir_info.clusterpathing[sub_dir_info.neff] = sub_dir_info.cluster_number;
                        enqueue(&queue, sub_dir_info);
                    }
                    if (strcmp(dir_table.table[i].name, target_name) == 0)
                    {
                        n_found++;
                        char path[MAX_DIR_STACK_SIZE * 10];

                        uint32_t get_i = 0;
                        while (get_i <= current_info.neff)
                        {
                            struct FAT32DirectoryTable parent_table;
                            syscall_user(10, (uint32_t)&parent_table, current_info.clusterpathing[get_i], 0);
                            custom_strcat(path, parent_table.table[0].name);
                            custom_strcat(path, "/");
                            get_i++;
                        }
                        custom_strcat(path, target_name);

                        if (dir_table.table[i].filesize != 0)
                        {
                            char ext[4];
                            ext[0] = '.';
                            for (int j = 1; j < 4; j++)
                            {
                                ext[j] = dir_table.table[i].ext[j - 1];
                            }
                            custom_strcat(path, ext);
                            syscall_user(6, (uint32_t)path, custom_strlen(path), WHITE);
                        }
                        else
                        {
                            syscall_user(6, (uint32_t)path, custom_strlen(path), LIGHT_PURPLE);
                        }
                        syscall_user(6, (uint32_t) "\n", 1, WHITE);
                        for (int i = 0; i < MAX_DIR_STACK_SIZE * 10; i++)
                        {
                            path[i] = 0;
                        }
                    }
                }
            }
        }
        else
        {
            syscall_user(6, (uint32_t) "Error reading directory.\n", 26, RED);
        }
    }
    if (n_found == 0)
    {
        syscall_user(6, (uint32_t) "Tidak ada file/folder yang ditemukan \n", 39, RED);
    }
}

void fain(char *fname)
{
    bfs_find(fname);
}
void ket(char *Filename, uint32_t *dir_stack, uint8_t *dir_stack_index, char (*dir_name_stack)[8])
{
    // cat : Menuliskan sebuah file sebagai text file ke layar (Gunakan format
    int parseId = 0;
    // LF newline)
    if (strcmp("./", Filename) == 0)
    {
        Filename = Filename + 2;
    }
    if (strcmp("../", Filename) == 0)
    {
        do
        {
            parseId += 1;
            Filename = Filename + 3;
        } while (strcmp("../", Filename) == 0);
        if (*dir_stack_index <= parseId)
        {
            syscall_user(6, (uint32_t) "INVALID FILE PATH\n", 18, RED);
            return;
        }
    }
    struct FAT32DirectoryTable parent_dir;
    struct FAT32DriverRequest request = {
        .buf = &parent_dir,
        .parent_cluster_number = dir_stack[*dir_stack_index - (parseId + 1)],
        .ext = "\0\0\0",
        .buffer_size = 0,
    };
    uint8_t i = 0;
    for (; i < 8; i++)
    {
        request.name[i] = dir_name_stack[*dir_stack_index - (parseId + 1)][i];
    }
    int8_t retcode;
    syscall_user(1, (uint32_t)&request, (uint32_t)&retcode, 0);
    syscall_user(10, (uint32_t)&parent_dir, dir_stack[*dir_stack_index - (parseId + 1)], 0);
    if (retcode != 0)
    {
        syscall_user(6, (uint32_t) "FAILED TO READ FOLDER\n", 22, RED);
        return;
    }
    char realFileName[9] = "\0\0\0\0\0\0\0\0\0";
    parseId = 0;
    while (strcmp(".", Filename + parseId) != 0 && parseId < 9)
    {
        realFileName[parseId] = Filename[parseId];
        parseId += 1;
    }
    if (parseId > 8)
    {
        syscall_user(6, (uint32_t) "INVALID FILE NAME\n", 18, RED);
        return;
    }
    else if (strcmp("txt", Filename + parseId + 1) != 0)
    {
        syscall_user(6, (uint32_t) "INVALID FILE EXTENSION\n", 23, RED);
        return;
    }
    parseId = 0;
    for (; parseId < 64; parseId++)
    {
        if (strcmp(parent_dir.table[parseId].name, realFileName) == 0 && strcmp("txt", parent_dir.table[parseId].ext) == 0)
        {
            struct ClusterBuffer fileBuff;

            struct FAT32DriverRequest request2 = {
                .buf = &fileBuff,
                .parent_cluster_number = request.parent_cluster_number,
                .ext = "txt",
                .buffer_size = parent_dir.table[parseId].filesize,
            };
            i = 0;
            for (; i < 8; i++)
            {
                request2.name[i] = realFileName[i];
            }
            syscall_user(0, (uint32_t)&request2, (uint32_t)&retcode, 0);
            if (retcode == 0)
            {
                uint32_t clustnum = 0;
                for (; clustnum < request2.buffer_size; clustnum++)
                {
                    syscall_user(5, (uint32_t)fileBuff.buf + clustnum, WHITE, 0);
                    // syscall_user(6, (uint32_t)fileBuff.buf, request2.buffer_size, WHITE);
                }
                syscall_user(6, (uint32_t) "\n", 1, WHITE);
                return;
            }
            else
            {
                syscall_user(6, (uint32_t) "FAILED TO READ\n", 15, RED);
                return;
            }
        }
    }
    syscall_user(6, (uint32_t) "FILE NOT FOUND\n", 15, RED);
}
void emvi(char *filename, char *dest, uint32_t *dir_stack, uint8_t *dir_stack_index, char (*dir_name_stack)[8])
{
    int parseId = 0;
    // LF newline)
    if (strcmp("./", filename) == 0)
    {
        filename = filename + 2;
    }
    if (strcmp("./", dest) == 0)
    {
        dest = dest + 2;
    }
    if (strcmp("../", dest) == 0)
    {
        do
        {
            parseId += 1;
            dest = dest + 3;
        } while (strcmp("../", dest) == 0);
        if (*dir_stack_index <= parseId)
        {
            syscall_user(6, (uint32_t) "INVALID DESTINATION PATH\n", 18, RED);
            return;
        }
    }
    uint8_t attribute;
    uint8_t user_attribute;
    uint16_t cluster_high;
    uint16_t cluster_low;
    uint32_t filesize;
    struct FAT32DirectoryTable curr_dir;
    struct FAT32DriverRequest requestcurr = {
        .buf = &curr_dir,
        .parent_cluster_number = dir_stack[*dir_stack_index - 1],
        .ext = "\0\0\0",
        .buffer_size = 0,
    };
    uint8_t i = 0;
    for (; i < 8; i++)
    {
        requestcurr.name[i] = dir_name_stack[*dir_stack_index - 1][i];
    }
    int8_t retcode;
    syscall_user(1, (uint32_t)&requestcurr, (uint32_t)&retcode, 0);
    syscall_user(10, (uint32_t)&curr_dir, dir_stack[*dir_stack_index - 1], 0);
    if (retcode != 0)
    {
        syscall_user(6, (uint32_t) "FAILED TO READ SRCFOLDER\n", 25, RED);
        return;
    }
    i = 0;
    for (; i < 64; i++)
    {
        if (strcmp(curr_dir.table[i].name, filename) == 0)
        {
            uint8_t k = 0;
            for (; k < 8; k++)
            {
                curr_dir.table[i].name[k] = 0;
            }
            k = 0;
            for (; k < 3; k++)
            {
                curr_dir.table[i].ext[k] = 0;
            }
            attribute = curr_dir.table[i].attribute;
            user_attribute = curr_dir.table[i].user_attribute;
            cluster_high = curr_dir.table[i].cluster_high;
            cluster_low = curr_dir.table[i].cluster_low;
            filesize = curr_dir.table[i].filesize;
            curr_dir.table[i].attribute = 0;

            curr_dir.table[i].user_attribute = 0;
            curr_dir.table[i].cluster_high = 0;
            curr_dir.table[i].cluster_low = 0;
            curr_dir.table[i].filesize = 0;

            syscall_user(11, (uint32_t)&curr_dir, dir_stack[*dir_stack_index - 1], 0);
        }
    }
    uint32_t parentCluster = dir_stack[*dir_stack_index - (parseId + 1)];
    struct FAT32DirectoryTable parent_dir;
    struct FAT32DriverRequest request = {
        .buf = &parent_dir,
        .parent_cluster_number = parentCluster,
        .ext = "\0\0\0",
        .buffer_size = 0,
    };
    i = 0;
    for (; i < 8; i++)
    {
        request.name[i] = dir_name_stack[*dir_stack_index - (parseId + 1)][i];
    }
    syscall_user(1, (uint32_t)&request, (uint32_t)&retcode, 0);
    syscall_user(10, (uint32_t)&parent_dir, dir_stack[*dir_stack_index - (parseId + 1)], 0);
    if (retcode != 0)
    {
        syscall_user(6, (uint32_t) "FAILED TO READ DSTFOLDER\n", 25, RED);
        return;
    }
    parseId = 0;
    for (; parseId < 64; parseId++)
    {
        if (parent_dir.table[parseId].user_attribute != UATTR_NOT_EMPTY)
        {
            i = 0;
            char destName[9] = "\0\0\0\0\0\0\0\0\0";
            uint8_t j = 0;
            while (strcmp(".", dest + j) != 0 && j < 9)
            {
                destName[j] = dest[j];
                j += 1;
            }
            if (j > 8)
            {
                syscall_user(6, (uint32_t) "INVALID SOURCE FILE NAME\n", 25, RED);
                return;
            }
            for (; i < 8; i++)
            {
                parent_dir.table[parseId].name[i] = destName[i];
            }
            i = 0;
            for (; i < 3; i++)
            {
                parent_dir.table[parseId].ext[i] = dest[j + i + 1];
            }
            parent_dir.table[parseId].attribute = attribute;

            parent_dir.table[parseId].user_attribute = user_attribute;
            parent_dir.table[parseId].cluster_high = cluster_high;
            parent_dir.table[parseId].cluster_low = cluster_low;
            parent_dir.table[parseId].filesize = filesize;
            syscall_user(11, (uint32_t)&parent_dir, parentCluster, 0);
            return;
        }
    }
}
void erem(char *filename, uint32_t *dir_stack, uint8_t *dir_stack_index)
{
    // cat : Menuliskan sebuah file sebagai text file ke layar (Gunakan format
    int parseId = 0;
    // LF newline)
    if (strcmp("./", filename) == 0)
    {
        filename = filename + 2;
    }
    if (strcmp("../", filename) == 0)
    {
        do
        {
            parseId += 1;
            filename = filename + 3;
        } while (strcmp("../", filename) == 0);
        if (*dir_stack_index <= parseId)
        {
            syscall_user(6, (uint32_t) "INVALID FILE PATH\n", 18, RED);
            return;
        }
    }

    struct FAT32DriverRequest request = {
        .parent_cluster_number = dir_stack[*dir_stack_index - (parseId + 1)],
        .ext = "\0\0\0",
        .buffer_size = 0,
    };
    uint8_t i = 0;
    char realFileName[9] = "\0\0\0\0\0\0\0\0\0";
    parseId = 0;
    while (strcmp(".", filename + parseId) != 0 && parseId < 9)
    {
        realFileName[parseId] = filename[parseId];
        parseId += 1;
    }
    if (parseId > 8)
    {
        syscall_user(6, (uint32_t) "INVALID FILE NAME\n", 18, RED);
        return;
    }
    request.ext[0] = filename[parseId + 1];
    request.ext[1] = filename[parseId + 2];
    request.ext[2] = filename[parseId + 3];

    for (; i < 8; i++)
    {
        request.name[i] = realFileName[i];
    }
    int8_t retcode;
    syscall_user(12, (uint32_t)&request, (uint32_t)&retcode, 0);
    if (retcode != 0)
    {
        syscall_user(6, (uint32_t) "FAILED TO DELETE FOLDER\n", 24, RED);
        return;
    }
}
void cepe(char *filename, char *dest, uint32_t *dir_stack, uint8_t *dir_stack_index, char (*dir_name_stack)[8])
{
    int parseId = 0;
    int8_t retcode;
    // LF newline)
    if (strcmp("./", filename) == 0)
    {
        filename = filename + 2;
    }
    if (strcmp("./", dest) == 0)
    {
        dest = dest + 2;
    }
    if (strcmp("../", dest) == 0)
    {
        do
        {
            parseId += 1;
            dest = dest + 3;
        } while (strcmp("../", dest) == 0);
        if (*dir_stack_index <= parseId)
        {
            syscall_user(6, (uint32_t) "INVALID DESTINATION PATH\n", 18, RED);
            return;
        }
    }
    int destParseId = parseId;
    // uint8_t attribute;
    // uint8_t user_attribute;
    // uint16_t cluster_high;
    // uint16_t cluster_low;
    // read current directory
    struct FAT32DirectoryTable curr_dir;
    struct FAT32DriverRequest requestcurr = {
        .buf = &curr_dir,
        .parent_cluster_number = dir_stack[*dir_stack_index - 1],
        .ext = "\0\0\0",
        .buffer_size = 0,

    };
    uint8_t i = 0;
    for (; i < 8; i++)
    {
        requestcurr.name[i] = dir_name_stack[*dir_stack_index - 1][i];
    }
    syscall_user(1, (uint32_t)&requestcurr, (uint32_t)&retcode, 0);
    syscall_user(10, (uint32_t)&curr_dir, dir_stack[*dir_stack_index - 1], 0);
    if (retcode != 0)
    {
        syscall_user(6, (uint32_t) "FAILED TO READ DSTFOLDER\n", 25, RED);
        return;
    }
    // read source file size
    uint32_t filesize;
    struct FAT32DriverRequest request = {
        .parent_cluster_number = dir_stack[*dir_stack_index - 1],
        .ext = "\0\0\0",
    };
    char realFileName[9] = "\0\0\0\0\0\0\0\0\0";
    parseId = 0;
    while (strcmp(".", filename + parseId) != 0 && parseId < 9)
    {
        realFileName[parseId] = filename[parseId];
        parseId += 1;
    }
    if (parseId > 8)
    {
        syscall_user(6, (uint32_t) "INVALID FILE NAME\n", 18, RED);
        return;
    }
    request.ext[0] = filename[parseId + 1];
    request.ext[1] = filename[parseId + 2];
    request.ext[2] = filename[parseId + 3];
    i = 0;
    for (; i < 8; i++)
    {
        request.name[i] = realFileName[i];
    }

    i = 0;
    for (; i < 64; i++)
    {
        if (strcmp(curr_dir.table[i].name, realFileName) == 0)
        {
            // attribute = curr_dir.table[i].attribute;
            // user_attribute=curr_dir.table[i].user_attribute;
            // cluster_high= curr_dir.table[i].cluster_high;
            // cluster_low = curr_dir.table[i].cluster_low ;
            filesize = curr_dir.table[i].filesize;
        }
    }
    uint8_t buffer[filesize];
    request.buf = buffer;
    request.buffer_size = filesize;
    syscall_user(0, (uint32_t)&request, (uint32_t)&retcode, 0);
    if (retcode != 0)
    {
        syscall_user(6, (uint32_t) "FAILED TO READ SRCFILE\n", 23, RED);
        return;
    }
    // delete
    struct FAT32DriverRequest request2 = {
        .parent_cluster_number = dir_stack[*dir_stack_index - (destParseId + 1)],
        .buffer_size = 0,
    };
    i = 0;
    char realFileDestName[9] = "\0\0\0\0\0\0\0\0\0";
    parseId = 0;
    while (strcmp(".", dest + parseId) != 0 && parseId < 9)
    {
        realFileDestName[parseId] = dest[parseId];
        parseId += 1;
    }
    if (parseId > 8)
    {
        syscall_user(6, (uint32_t) "INVALID FILE NAME\n", 18, RED);
        return;
    }
    request2.ext[0] = dest[parseId + 1];
    request2.ext[1] = dest[parseId + 2];
    request2.ext[2] = dest[parseId + 3];
    i = 0;
    for (; i < 8; i++)
    {
        request2.name[i] = realFileDestName[i];
    }
    syscall_user(12, (uint32_t)&request2, (uint32_t)&retcode, 0);
    if (retcode != 0)
    {
        syscall_user(6, (uint32_t) "FAILED TO DELETE FOLDER\n", 24, RED);
        return;
    }
    // write
    request2.buffer_size = filesize;
    request2.buf = buffer;
    syscall_user(2, (uint32_t)&request2, (uint32_t)&retcode, 0);
    if (retcode != 0)
    {
        syscall_user(6, (uint32_t) "FAILED TO WRITE FOLDER\n", 23, RED);
        return;
    }
}
void nano(char *filename, uint32_t *dir_stack, uint8_t *dir_stack_index)
{
    // cat : Menuliskan sebuah file sebagai text file ke layar (Gunakan format
    int parseId = 0;
    // LF newline)
    if (strcmp("./", filename) == 0)
    {
        filename = filename + 2;
    }
    if (strcmp("../", filename) == 0)
    {
        do
        {
            parseId += 1;
            filename = filename + 3;
        } while (strcmp("../", filename) == 0);
        if (*dir_stack_index <= parseId)
        {
            syscall_user(6, (uint32_t) "INVALID FILE PATH\n", 18, RED);
            return;
        }
    }

    struct FAT32DriverRequest request = {
        .parent_cluster_number = dir_stack[*dir_stack_index - (parseId + 1)],
    };
    uint8_t i = 0;
    char realFileName[9] = "\0\0\0\0\0\0\0\0\0";
    parseId = 0;
    while (strcmp(".", filename + parseId) != 0 && parseId < 9)
    {
        realFileName[parseId] = filename[parseId];
        parseId += 1;
    }
    // if (parseId > 8)
    // {
    //     syscall_user(6, (uint32_t) "INVALID FILE NAME\n", 18, RED);
    //     return;
    // }
    request.ext[0] = filename[parseId + 1];
    request.ext[1] = filename[parseId + 2];
    request.ext[2] = filename[parseId + 3];

    for (; i < 8; i++)
    {
        request.name[i] = realFileName[i];
    }
    char buff[CLUSTER_SIZE];

    int j = 0;
    char input = 'a';
    syscall_user(6, (uint32_t) "Write Text Bellow to File: \n", 28, YELLOW);
    do
    {
        syscall_user(4, (uint32_t)&input, 0, 0);
        if (!(input == '\b' && j == 0))
        {
            syscall_user(5, (uint32_t)&input, GREEN, 0);
        }
        if (input != 0 && input != '\n')
        {
            if (input == '\b' && j == 0)
            {
                // do nothing
            }
            else if (input == '\b' && j > 0)
            {
                buff[j - 1] = 0;
                j -= 1;
            }
            else
            {
                buff[j] = input;
                j += 1;
            }
        }
    } while (input != '\n');
    request.buffer_size = j;
    request.buf = buff;
    int8_t retcode;
    syscall_user(2, (uint32_t)&request, (uint32_t)&retcode, 0);
    if (retcode != 0)
    {
        syscall_user(6, (uint32_t) "FAILED TO WRITE FOLDER\n", 23, RED);
        return;
    }
}
void eksek(char *filename, uint32_t *dir_stack, uint8_t *dir_stack_index, char (*dir_name_stack)[8])
{
    int parseId = 0;
    int8_t retcode;
    // LF newline)
    if (strcmp("./", filename) == 0)
    {
        filename = filename + 2;
    }

    if (strcmp("../", filename) == 0)
    {
        do
        {
            parseId += 1;
            filename = filename + 3;
        } while (strcmp("../", filename) == 0);
        if (*dir_stack_index <= parseId)
        {
            syscall_user(6, (uint32_t) "INVALID DESTINATION PATH\n", 18, RED);
            return;
        }
    }
    int destParseId = parseId;
    // uint8_t attribute;
    // uint8_t user_attribute;
    // uint16_t cluster_high;
    // uint16_t cluster_low;
    // read current directory
    struct FAT32DirectoryTable curr_dir;
    struct FAT32DriverRequest requestcurr = {
        .buf = &curr_dir,
        .parent_cluster_number = dir_stack[*dir_stack_index - (destParseId + 1)],
        .ext = "\0\0\0",
        .buffer_size = 0,

    };
    uint8_t i = 0;
    for (; i < 8; i++)
    {
        requestcurr.name[i] = dir_name_stack[*dir_stack_index - (destParseId + 1)][i];
    }
    syscall_user(1, (uint32_t)&requestcurr, (uint32_t)&retcode, 0);
    syscall_user(10, (uint32_t)&curr_dir, dir_stack[*dir_stack_index - (destParseId + 1)], 0);
    if (retcode != 0)
    {
        syscall_user(6, (uint32_t) "FAILED TO READ DSTFOLDER\n", 25, RED);
        return;
    }
    // read source file size

    uint32_t filesize;
    struct FAT32DriverRequest request = {
        .parent_cluster_number = dir_stack[*dir_stack_index - (destParseId + 1)],
        .ext = "\0\0\0",
    };
    char realFileName[9] = "\0\0\0\0\0\0\0\0\0";
    parseId = 0;
    while (strcmp(".", filename + parseId) != 0 && parseId < 9)
    {
        realFileName[parseId] = filename[parseId];
        parseId += 1;
    }

    if (parseId > 8)
    {
        syscall_user(6, (uint32_t) "INVALID FILE NAME\n", 18, RED);
        return;
    }

    request.ext[0] = filename[parseId + 1];
    request.ext[1] = filename[parseId + 2];
    request.ext[2] = filename[parseId + 3];

    i = 0;
    for (; i < 8; i++)
    {
        request.name[i] = realFileName[i];
    }

    i = 0;
    for (; i < 64; i++)
    {
        if (strcmp(curr_dir.table[i].name, realFileName) == 0)
        {
            // attribute = curr_dir.table[i].attribute;
            // user_attribute=curr_dir.table[i].user_attribute;
            // cluster_high= curr_dir.table[i].cluster_high;
            // cluster_low = curr_dir.table[i].cluster_low ;
            filesize = curr_dir.table[i].filesize;
            request.buf = (uint8_t *)0;
            request.buffer_size = filesize;
            syscall_user(15, (uint32_t)&request, (uint32_t)&retcode, 0);
            if (retcode != 0)
            {
                syscall_user(6, (uint32_t) "FAILED TO EXEC SRCFILE\n", 23, RED);
                return;
            }
        }
    }
    syscall_user(6, (uint32_t) "FILE NOT FOUND\n", 15, RED);
}

void pees()
{
    struct ProcessControlBlock pcb;

    syscall_user(15, (uint32_t)&pcb, 0, 0);
    syscall_user(6, (uint32_t) "PROCESS INFO: \n", 15, BLUE);
    syscall_user(6, (uint32_t) "ID: ", 4, GREEN);
    printInt(pcb.metadata.pid);
    syscall_user(5, (uint32_t) & "\n", GREEN, 0);
    syscall_user(6, (uint32_t) "NAME: ", 7, GREEN);
    uint8_t i = 0;
    for (; i < PROCESS_NAME_LENGTH_MAX && pcb.metadata.name[i] != 0; i++)
    {
        syscall_user(5, (uint32_t)pcb.metadata.name + i, WHITE, 0);
    }
    syscall_user(5, (uint32_t) "\n", WHITE, 0);
    syscall_user(6, (uint32_t) "STATUS: ", 8, WHITE);
    if (pcb.metadata.state == 0)
    {
        syscall_user(6, (uint32_t) "READY ", 6, YELLOW);
    }
    else if (pcb.metadata.state == 1)
    {
        syscall_user(6, (uint32_t) "RUNNING ", 8, GREEN);
    }
    else if (pcb.metadata.state == 2)
    {
        syscall_user(6, (uint32_t) "BLOCKED ", 8, RED);
    }
    syscall_user(5, (uint32_t) & "\n", WHITE, 0);
}
void exec_command(uint32_t *dir_stack, uint8_t *dir_stack_index, char (*dir_name_stack)[8])

{
    char buff[MAX_INPUT_BUFFER];
    int i = 0;
    char input = 'a';
    do
    {
        syscall_user(4, (uint32_t)&input, 0, 0);
        if (!(input == '\b' && i == 0))
        {
            syscall_user(5, (uint32_t)&input, 0xF, 0);
        }
        if (input != 0 && input != '\n')
        {
            if (input == '\b' && i == 0)
            {
                // do nothing
            }
            else if (input == '\b' && i > 0)
            {
                buff[i - 1] = ' ';
                i -= 1;
            }
            else
            {
                buff[i] = input;
                i += 1;
            }
            if (i == 2)
            {
                if (buff[0] == 'l' && buff[1] == 's')
                {
                    syscall_user(5, (uint32_t) "\b", 0xF, 0);
                    syscall_user(5, (uint32_t) "\b", 0xF, 0);
                    syscall_user(5, (uint32_t) "l", BLUE, 0);
                    syscall_user(5, (uint32_t) "s", BLUE, 0);
                }
            }
            else if (i == 3)
            {
                if (buff[0] == 'c' && buff[1] == 'd' && buff[2] == ' ')
                {
                    syscall_user(5, (uint32_t) "\b", 0xF, 0);
                    syscall_user(5, (uint32_t) "\b", 0xF, 0);
                    syscall_user(5, (uint32_t) "\b", 0xF, 0);
                    syscall_user(5, (uint32_t) "c", YELLOW, 0);
                    syscall_user(5, (uint32_t) "d", YELLOW, 0);
                    syscall_user(5, (uint32_t) " ", WHITE, 0);
                }
                else if (buff[0] == 'm' && buff[1] == 'v' && buff[2] == ' ')
                {
                    syscall_user(5, (uint32_t) "\b", 0xF, 0);
                    syscall_user(5, (uint32_t) "\b", 0xF, 0);
                    syscall_user(5, (uint32_t) "\b", 0xF, 0);
                    syscall_user(5, (uint32_t) "m", BLUE, 0);
                    syscall_user(5, (uint32_t) "v", BLUE, 0);
                    syscall_user(5, (uint32_t) " ", WHITE, 0);
                }
                else if (buff[0] == 'r' && buff[1] == 'm' && buff[2] == ' ')
                {
                    syscall_user(5, (uint32_t) "\b", 0xF, 0);
                    syscall_user(5, (uint32_t) "\b", 0xF, 0);
                    syscall_user(5, (uint32_t) "\b", 0xF, 0);
                    syscall_user(5, (uint32_t) "r", YELLOW, 0);
                    syscall_user(5, (uint32_t) "m", YELLOW, 0);
                    syscall_user(5, (uint32_t) " ", WHITE, 0);
                }
                else if (buff[0] == 'c' && buff[1] == 'p' && buff[2] == ' ')
                {
                    syscall_user(5, (uint32_t) "\b", 0xF, 0);
                    syscall_user(5, (uint32_t) "\b", 0xF, 0);
                    syscall_user(5, (uint32_t) "\b", 0xF, 0);
                    syscall_user(5, (uint32_t) "c", AQUA, 0);
                    syscall_user(5, (uint32_t) "p", AQUA, 0);
                    syscall_user(5, (uint32_t) " ", WHITE, 0);
                }
            }
            else if (i == 4)
            {
                if (buff[0] == 'c' && buff[1] == 'a' && buff[2] == 't' && buff[3] == ' ')
                {
                    syscall_user(5, (uint32_t) "\b", 0xF, 0);
                    syscall_user(5, (uint32_t) "\b", 0xF, 0);
                    syscall_user(5, (uint32_t) "\b", 0xF, 0);
                    syscall_user(5, (uint32_t) "\b", 0xF, 0);
                    syscall_user(5, (uint32_t) "c", AQUA, 0);
                    syscall_user(5, (uint32_t) "a", AQUA, 0);
                    syscall_user(5, (uint32_t) "t", AQUA, 0);
                    syscall_user(5, (uint32_t) " ", WHITE, 0);
                }
            }
            else if (i == 5)
            {
                if (buff[0] == 'f' && buff[1] == 'i' && buff[2] == 'n' && buff[3] == 'd' && buff[4] == ' ')
                {
                    syscall_user(5, (uint32_t) "\b", 0xF, 0);
                    syscall_user(5, (uint32_t) "\b", 0xF, 0);
                    syscall_user(5, (uint32_t) "\b", 0xF, 0);
                    syscall_user(5, (uint32_t) "\b", 0xF, 0);
                    syscall_user(5, (uint32_t) "\b", 0xF, 0);
                    syscall_user(5, (uint32_t) "f", LIGHT_PURPLE, 0);
                    syscall_user(5, (uint32_t) "i", LIGHT_PURPLE, 0);
                    syscall_user(5, (uint32_t) "n", LIGHT_PURPLE, 0);
                    syscall_user(5, (uint32_t) "d", LIGHT_PURPLE, 0);
                    syscall_user(5, (uint32_t) " ", WHITE, 0);
                }
                else if (buff[0] == 'c' && buff[1] == 'l' && buff[2] == 'e' && buff[3] == 'a' && buff[4] == 'r')
                {
                    syscall_user(5, (uint32_t) "\b", 0xF, 0);
                    syscall_user(5, (uint32_t) "\b", 0xF, 0);
                    syscall_user(5, (uint32_t) "\b", 0xF, 0);
                    syscall_user(5, (uint32_t) "\b", 0xF, 0);
                    syscall_user(5, (uint32_t) "\b", 0xF, 0);
                    syscall_user(5, (uint32_t) "c", LIGHT_PURPLE, 0);
                    syscall_user(5, (uint32_t) "l", LIGHT_PURPLE, 0);
                    syscall_user(5, (uint32_t) "e", LIGHT_PURPLE, 0);
                    syscall_user(5, (uint32_t) "a", LIGHT_PURPLE, 0);
                    syscall_user(5, (uint32_t) "r", LIGHT_PURPLE, 0);
                }
                else if (buff[0] == 'n' && buff[1] == 'a' && buff[2] == 'n' && buff[3] == 'o' && buff[4] == ' ')
                {
                    syscall_user(5, (uint32_t) "\b", 0xF, 0);
                    syscall_user(5, (uint32_t) "\b", 0xF, 0);
                    syscall_user(5, (uint32_t) "\b", 0xF, 0);
                    syscall_user(5, (uint32_t) "\b", 0xF, 0);
                    syscall_user(5, (uint32_t) "\b", 0xF, 0);
                    syscall_user(5, (uint32_t) "n", LIGHT_PURPLE, 0);
                    syscall_user(5, (uint32_t) "a", LIGHT_PURPLE, 0);
                    syscall_user(5, (uint32_t) "n", LIGHT_PURPLE, 0);
                    syscall_user(5, (uint32_t) "o", LIGHT_PURPLE, 0);
                    syscall_user(5, (uint32_t) " ", WHITE, 0);
                }
            }
            else if (i == 6)
            {
                if (buff[0] == 'm' && buff[1] == 'k' && buff[2] == 'd' && buff[3] == 'i' && buff[4] == 'r' && buff[5] == ' ')
                {
                    syscall_user(5, (uint32_t) "\b", 0xF, 0);
                    syscall_user(5, (uint32_t) "\b", 0xF, 0);
                    syscall_user(5, (uint32_t) "\b", 0xF, 0);
                    syscall_user(5, (uint32_t) "\b", 0xF, 0);
                    syscall_user(5, (uint32_t) "\b", 0xF, 0);
                    syscall_user(5, (uint32_t) "\b", 0xF, 0);
                    syscall_user(5, (uint32_t) "m", YELLOW, 0);
                    syscall_user(5, (uint32_t) "k", YELLOW, 0);
                    syscall_user(5, (uint32_t) "d", YELLOW, 0);
                    syscall_user(5, (uint32_t) "i", YELLOW, 0);
                    syscall_user(5, (uint32_t) "r", YELLOW, 0);
                    syscall_user(5, (uint32_t) " ", WHITE, 0);
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

    if (strcmp("cd", args[0]) == 0)
    {

        char *dirname = args[1];
        cede(dirname, dir_stack, dir_stack_index, dir_name_stack);
    }
    else if (strcmp("ls", args[0]) == 0)
    {
        eles(dir_stack, dir_stack_index, dir_name_stack);
    }
    else if (strcmp("mkdir", args[0]) == 0)
    {
        if (argc >= 2)
        {

            char *dirname = args[1];
            mkdir_command(dirname, dir_stack[*dir_stack_index - 1]);
        }
        else
        {
            syscall_user(6, (uint32_t) "[ERROR]: Usage: mkdir <dirname>\n", 33, RED);
        }
    }
    else if (strcmp("find", args[0]) == 0)
    {
        if (argc >= 2)
        {
            fain(args[1]);
        }
        else
        {
            syscall_user(6, (uint32_t) "[ERROR]: Usage: find <folder/filename> \n", 41, RED);
        }
    }
    else if (strcmp("cat", args[0]) == 0)
    {
        // Handle cat command
        char *filename = args[1];
        ket(filename, dir_stack, dir_stack_index, dir_name_stack);
    }
    else if (strcmp("clear", args[0]) == 0)
    {
        syscall_user(9, 0, 0, 0);
    }
    else if (strcmp("mv", args[0]) == 0)
    {
        if (argc == 3)
        {
            emvi(args[1], args[2], dir_stack, dir_stack_index, dir_name_stack);
        }
        else
        {
            syscall_user(6, (uint32_t) "[ERROR]: Usage: mv <sourcename> <destpath>\n", 42, RED);
        }
    }
    else if (strcmp("rm", args[0]) == 0)
    {
        if (argc == 2)
        {
            erem(args[1], dir_stack, dir_stack_index);
        }
        else
        {
            syscall_user(6, (uint32_t) "[ERROR]: Usage: rm <filepath>\n", 30, RED);
        }
    }
    else if (strcmp("cp", args[0]) == 0)
    {
        if (argc == 3)
        {
            cepe(args[1], args[2], dir_stack, dir_stack_index, dir_name_stack);
        }
        else
        {
            syscall_user(6, (uint32_t) "[ERROR]: Usage: rm <filesrcpath> <filesdestpath>\n", 49, RED);
        }
    }
    else if (strcmp("nano", args[0]) == 0)
    {
        if (argc == 2)
        {
            nano(args[1], dir_stack, dir_stack_index);
        }
        else
        {
            syscall_user(6, (uint32_t) "[ERROR]: Usage: nano <filepath> \n", 33, RED);
        }
    }
    else if (strcmp("./", args[0]) == 0 || strcmp("../", args[0]) == 0)
    {
        if (argc == 1)
        {
            eksek(args[0], dir_stack, dir_stack_index, dir_name_stack);
        }
        else
        {
            syscall_user(6, (uint32_t) "[ERROR]: Usage: ./<filepath>\n", 32, RED);
        }
    }
    else if (strcmp("ps", args[0]) == 0)
    {
        if (argc == 1)
        {
            pees();
        }
        else
        {
            syscall_user(6, (uint32_t) "[ERROR]: Usage: ps\n", 20, RED);
        }
    }
    else if (strcmp("clock", args[0]) == 0)
    {

        syscall_user(16, 0,0,0);
        
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
    char DIR_NAME_STACK[MAX_DIR_STACK_SIZE][8] = {"ROOT\0\0\0\0"};
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