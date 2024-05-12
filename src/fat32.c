#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "header/stdlib/string.h"
#include "header/filesystem/fat32.h"
#include "header/driver/disk.h"

static struct FAT32DriverState driver_state;
const uint8_t fs_signature[BLOCK_SIZE] = {
    'C',
    'o',
    'u',
    'r',
    's',
    'e',
    ' ',
    ' ',
    ' ',
    ' ',
    ' ',
    ' ',
    ' ',
    ' ',
    ' ',
    ' ',
    'D',
    'e',
    's',
    'i',
    'g',
    'n',
    'e',
    'd',
    ' ',
    'b',
    'y',
    ' ',
    ' ',
    ' ',
    ' ',
    ' ',
    'L',
    'a',
    'b',
    ' ',
    'S',
    'i',
    's',
    't',
    'e',
    'r',
    ' ',
    'I',
    'T',
    'B',
    ' ',
    ' ',
    'M',
    'a',
    'd',
    'e',
    ' ',
    'w',
    'i',
    't',
    'h',
    ' ',
    '<',
    '3',
    ' ',
    ' ',
    ' ',
    ' ',
    '-',
    '-',
    '-',
    '-',
    '-',
    '-',
    '-',
    '-',
    '-',
    '-',
    '-',
    '2',
    '0',
    '2',
    '4',
    '\n',
    [BLOCK_SIZE - 2] = 'O',
    [BLOCK_SIZE - 1] = 'k',
};

/* -- Driver Interfaces -- */

/**
 * Convert cluster number to logical block address
 *
 * @param cluster Cluster number to convert
 * @return uint32_t Logical Block Address
 */
uint32_t cluster_to_lba(uint32_t cluster)
{
    return cluster * 4;
}
/**
 * Initialize DirectoryTable value with
 * - Entry-0: DirectoryEntry about itself
 * - Entry-1: Parent DirectoryEntry
 *
 * @param dir_table          Pointer to directory table
 * @param name               8-byte char for directory name
 * @param parent_dir_cluster Parent directory cluster number
 */
void init_directory_table(struct FAT32DirectoryTable *dir_table, char *name, uint32_t parent_dir_cluster)
{
    memcpy(dir_table->table->name, name, 8);
    memcpy(dir_table->table->ext, "\0\0\0", 3);

    dir_table->table[0].attribute = ATTR_SUBDIRECTORY;
    dir_table->table[0].user_attribute = UATTR_NOT_EMPTY;

    dir_table->table[0].undelete = 0;
    dir_table->table[0].create_time = 0;
    dir_table->table[0].create_date = 0;
    dir_table->table[0].access_date = 0;

    dir_table->table[0].cluster_high = (uint16_t)(parent_dir_cluster >> 16);

    dir_table->table[0].modified_time = 0;
    dir_table->table[0].modified_date = 0;

    dir_table->table[0].cluster_low = (uint16_t)(parent_dir_cluster & 0xFFFF);
    dir_table->table[0].filesize = 0;
}
/**
 * Checking whether filesystem signature is missing or not in boot sector
 *
 * @return True if memcmp(boot_sector, fs_signature) returning inequality
 */

bool is_empty_storage(void)
{
    uint8_t bootsector[BLOCK_SIZE];
    read_blocks(bootsector, 0, 1);
    return memcmp(bootsector, fs_signature, BLOCK_SIZE);
}

/**
 * Create new FAT32 file system. Will write fs_signature into boot sector and
 * proper FileAllocationTable (contain CLUSTER_0_VALUE, CLUSTER_1_VALUE,
 * and initialized root directory) into cluster number 1
 */
void create_fat32(void)
{
    write_blocks(fs_signature, 0, 1);

    driver_state.fat_table.cluster_map[0] = CLUSTER_0_VALUE;
    driver_state.fat_table.cluster_map[1] = CLUSTER_1_VALUE;
    driver_state.fat_table.cluster_map[2] = FAT32_FAT_END_OF_FILE;

    write_clusters(&driver_state.fat_table, 1, 1);

    init_directory_table(&(driver_state.dir_table_buf), "ROOT\0\0\0\0", ROOT_CLUSTER_NUMBER);
    write_clusters(&(driver_state.dir_table_buf), ROOT_CLUSTER_NUMBER, 1);
}

/**
 * Initialize file system driver state, if is_empty_storage() then create_fat32()
 * Else, read and cache entire FileAllocationTable (located at cluster number 1) into driver state
 */
void initialize_filesystem_fat32(void)
{
    if (is_empty_storage())
    {
        create_fat32();
    }
    else
    {
        memset(driver_state.fat_table.cluster_map, 0, CLUSTER_SIZE);
        read_clusters(driver_state.fat_table.cluster_map, 1, 1);
    }
}

/**
 * Write cluster operation, wrapper for write_blocks().
 * Recommended to use struct ClusterBuffer
 *
 * @param ptr            Pointer to source data
 * @param cluster_number Cluster number to write
 * @param cluster_count  Cluster count to write, due limitation of write_blocks block_count 255 => max cluster_count = 63
 */
void write_clusters(const void *ptr, uint32_t cluster_number, uint8_t cluster_count)
{
    write_blocks(ptr, cluster_to_lba(cluster_number), cluster_count * CLUSTER_BLOCK_COUNT);
}
/**
 * Read cluster operation, wrapper for read_blocks().
 * Recommended to use struct ClusterBuffer
 *
 * @param ptr            Pointer to buffer for reading
 * @param cluster_number Cluster number to read
 * @param cluster_count  Cluster count to read, due limitation of read_blocks block_count 255 => max cluster_count = 63
 */
void read_clusters(void *ptr, uint32_t cluster_number, uint8_t cluster_count)
{
    read_blocks(ptr, cluster_to_lba(cluster_number), cluster_count * CLUSTER_BLOCK_COUNT);
}

/* -- CRUD Operation -- */

/**
 *  FAT32 Folder / Directory read
 *
 * @param request buf point to struct FAT32DirectoryTable,
 *                name is directory name,
 *                ext is unused,
 *                parent_cluster_number is target directory table to read,
 *                buffer_size must be exactly sizeof(struct FAT32DirectoryTable)
 * @return Error code: 0 success - 1 not a folder - 2 not found - -1 unknown
 */
int8_t read_directory(struct FAT32DriverRequest request)
{
    if (memcmp(request.name, "\0\0\0\0\0\0\0\0", 8) == 0)
    {
        return -1;
    }

    struct FAT32DirectoryTable dir_table;
    read_clusters(&dir_table, request.parent_cluster_number, 1);

    for (uint32_t i = 0; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++)
    {
        if (memcmp(dir_table.table[i].name, request.name, 8) == 0)
        {
            if (dir_table.table[i].attribute == ATTR_SUBDIRECTORY && dir_table.table[i].filesize == 0)
            {

                struct FAT32DirectoryTable reqDirectory;
                uint32_t cn = (dir_table.table[i].cluster_high << 16) | dir_table.table[i].cluster_low;
                read_clusters(&reqDirectory,cn , 1);
                memcpy(request.buf, &reqDirectory, sizeof(struct FAT32DirectoryTable));
                return 0;
            }
            else
            {

                return 1;
            }
        }
    }
    return 2;
}

/**
 * FAT32 read, read a file from file system.
 *
 * @param request All attribute will be used for read, buffer_size will limit reading count
 * @return Error code: 0 success - 1 not a file - 2 not enough buffer - 3 not found - -1 unknown
 */

int8_t read(struct FAT32DriverRequest request)
{
    if (memcmp(request.name, "\0\0\0\0\0\0\0\0", 8) == 0)
    {
        return -1; // unknown, isinia null
    }

    if (request.buffer_size == 0)
    {
        return 1; // not a file
    }

    struct FAT32DirectoryTable parent_table;
    read_clusters(&parent_table, request.parent_cluster_number, 1);

    for (uint32_t i = 1; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++)
    {
        if (memcmp(request.name, parent_table.table[i].name, 8) == 0 && memcmp(request.ext, parent_table.table[i].ext, 3) == 0)
        {

            uint32_t start_cluster = (parent_table.table[i].cluster_high << 16) | parent_table.table[i].cluster_low;

            uint32_t clusters_to_read = (request.buffer_size + CLUSTER_SIZE - 1) / CLUSTER_SIZE;

            uint32_t remaining_bytes = request.buffer_size;
            uint32_t current_cluster = start_cluster;
            uint32_t bytes_read_total = 0;

            while (clusters_to_read > 0 && current_cluster != FAT32_FAT_END_OF_FILE)
            {
                uint32_t bytes_to_read = remaining_bytes > CLUSTER_SIZE ? CLUSTER_SIZE : remaining_bytes;

                read_clusters(request.buf + bytes_read_total, current_cluster, 1);

                bytes_read_total += bytes_to_read;
                remaining_bytes -= bytes_to_read;
                clusters_to_read--;

                current_cluster = driver_state.fat_table.cluster_map[current_cluster];
            }

            if (bytes_read_total == request.buffer_size)
            {
                // terbaca semua
                return 0;
            }
            else if (bytes_read_total < request.buffer_size)
            {
                return 2; // not enough buffer
            }
            else
            {
                return -1; // unknown error
            }
        }
    }

    //  not found in the parent directory
    return 3;
}
/**
 * FAT32 write, write a file or folder to file system.
 *
 * @param request All attribute will be used for write, buffer_size == 0 then create a folder / directory
 * @return Error code: 0 success - 1 file/folder already exist - 2 invalid parent cluster - -1 unknown
 */
int8_t write(struct FAT32DriverRequest request)
{

    if (memcmp(request.name, "\0\0\0\0\0\0\0\0", 8) == 0)
    {
        return -1; // unknown, isinia null
    }

    if (request.parent_cluster_number < 2)
    {
        return 2;
    }

    struct FAT32DirectoryTable parent_dir;
    read_clusters(driver_state.cluster_buf.buf, request.parent_cluster_number, 1);
    memcpy(parent_dir.table,driver_state.cluster_buf.buf,sizeof(driver_state.cluster_buf.buf));

    for (uint8_t i = 1; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++)
    {

        if (memcmp(parent_dir.table[i].name, request.name, 8) == 0)
        {
            return 1;
        }
    }

    // directory entry create

    uint32_t n = 0;
    while (n < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry) &&
           parent_dir.table[n].user_attribute == UATTR_NOT_EMPTY)
    {
        n++;
    }

    memcpy(parent_dir.table[n].name, request.name, 8);
    if (request.buffer_size == 0)
    {
        parent_dir.table[n].attribute = ATTR_SUBDIRECTORY;
    }
    else
    {
        parent_dir.table[n].attribute = ATTR_ARCHIVE;
    }
    parent_dir.table[n].user_attribute = UATTR_NOT_EMPTY;
    parent_dir.table[n].undelete = 0;
    parent_dir.table[n].create_time = 0;
    parent_dir.table[n].create_date = 0;
    parent_dir.table[n].access_date = 0;
    parent_dir.table[n].modified_time = 0;
    parent_dir.table[n].modified_date = 0;
    parent_dir.table[n].filesize = request.buffer_size;
    // write folder
    if (request.buffer_size == 0)
    {
        uint32_t i = 2;
        while (driver_state.fat_table.cluster_map[i] != FAT32_FAT_EMPTY_ENTRY && i < CLUSTER_MAP_SIZE)
        {
            i++;
        }
        if (i == CLUSTER_MAP_SIZE)
        {
            return 1;
        }
        driver_state.fat_table.cluster_map[i] = FAT32_FAT_END_OF_FILE;

        struct FAT32DirectoryTable newDir;
        init_directory_table(&newDir, request.name, request.parent_cluster_number);
        write_clusters(newDir.table, i, 1);

        parent_dir.table[n].cluster_high = (uint16_t)((i >> 16) & 0xFFFF);
        parent_dir.table[n].cluster_low = (uint16_t)(i & 0xFFFF);
    }
    else
    {
        // write file
        uint8_t n_cluster = (int)(request.buffer_size / CLUSTER_SIZE) + (request.buffer_size % CLUSTER_SIZE != 0);
        uint32_t j = 0;
        uint32_t k = 2;
        uint32_t cluster_empty[n_cluster];
        memset(driver_state.cluster_buf.buf,0,CLUSTER_SIZE);
        while (j < n_cluster && k < CLUSTER_MAP_SIZE)
        {
            if (driver_state.fat_table.cluster_map[k] == FAT32_FAT_EMPTY_ENTRY)
            {
                cluster_empty[j] = k;
                j += 1;
            }
            k += 1;
        }
        if (j < n_cluster)
        {
            // no enough space left
            return 1;
        }
        if (request.buffer_size > 0)
        {
            for (uint8_t i = 0; i < n_cluster - 1; i++)
            {
                memcpy(driver_state.cluster_buf.buf ,request.buf + i * CLUSTER_SIZE,CLUSTER_SIZE);
                driver_state.fat_table.cluster_map[cluster_empty[i]] = cluster_empty[i + 1];
                write_clusters(driver_state.cluster_buf.buf, cluster_empty[i], 1);
                request.buffer_size -= CLUSTER_SIZE;
                memset(driver_state.cluster_buf.buf,0,CLUSTER_SIZE);
            }
        }
        memcpy(driver_state.cluster_buf.buf ,request.buf + (n_cluster-1) * CLUSTER_SIZE, request.buffer_size);
        driver_state.fat_table.cluster_map[cluster_empty[n_cluster - 1]] = FAT32_FAT_END_OF_FILE;
        write_clusters(driver_state.cluster_buf.buf, cluster_empty[n_cluster - 1], 1);
        memset(driver_state.cluster_buf.buf,0,CLUSTER_SIZE);

        memcpy(parent_dir.table[n].ext, request.ext, 3);
        parent_dir.table[n].cluster_high = (uint16_t)((cluster_empty[0] >> 16) & 0xFFFF);
        parent_dir.table[n].cluster_low = (uint16_t)(cluster_empty[0] & 0xFFFF);
    }
    write_clusters(driver_state.fat_table.cluster_map, FAT_CLUSTER_NUMBER, 1);
    write_clusters(parent_dir.table, request.parent_cluster_number, 1);
    return 0;
}

/**
 * FAT32 delete, delete a file or empty directory (only 1 DirectoryEntry) in file system.
 *
 * @param request buf and buffer_size is unused
 * @return Error code: 0 success - 1 not found - 2 folder is not empty - -1 unknown
 */
int8_t delete(struct FAT32DriverRequest request)
{

    if (memcmp(request.name, "\0\0\0\0\0\0\0\0", 8) == 0)
    {
        return -1;
    }

    struct FAT32DirectoryTable parent_dir;
    read_clusters(&parent_dir, request.parent_cluster_number, 1);
    for (uint32_t i = 1; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++)
    {
        if (memcmp(request.name, parent_dir.table[i].name, 8) == 0 && memcmp(request.ext, parent_dir.table[i].ext, 3) == 0)
        {

            uint32_t cluster_number = parent_dir.table[i].cluster_high << 16 | parent_dir.table[i].cluster_low;

            if (parent_dir.table[i].attribute == ATTR_SUBDIRECTORY)
            {
                // Delete directory

                struct FAT32DirectoryTable curr_table;
                read_clusters(curr_table.table, cluster_number, 1);

                uint8_t empty = 1;
                uint32_t j = 1;
                for (;j < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry) &&
                                     curr_table.table[j].user_attribute == UATTR_NOT_EMPTY;
                     j++)
                {
                }

                if (j > 1)
                {
                    empty = 0;
                }

                if (!empty)
                {
                    return 2;
                }

                // isi null cluster directory
                struct ClusterBuffer cbuf;
                for (j = 0; j < CLUSTER_SIZE; j++)
                {
                    cbuf.buf[j] = '\0';
                }
                write_clusters(&cbuf, cluster_number, 1);

                // Hapus directory
                driver_state.fat_table.cluster_map[cluster_number] = FAT32_FAT_EMPTY_ENTRY;
            }
            else
            {
                // Delete file

                // isi null file

                struct ClusterBuffer cbuf;
                uint32_t j = 0;
                for (; j < CLUSTER_SIZE; j++)
                {
                    cbuf.buf[j] = '\0';
                }
                // hapus dari storage
                do
                {

                    write_clusters(cbuf.buf, cluster_number, 1);
                    uint32_t temp_cluster_number = cluster_number;
                    cluster_number = driver_state.fat_table.cluster_map[cluster_number];
                    driver_state.fat_table.cluster_map[temp_cluster_number] = FAT32_FAT_EMPTY_ENTRY;
                } while (cluster_number!= FAT32_FAT_END_OF_FILE);

                // driver_state.fat_table.cluster_map[cluster_number] = FAT32_FAT_EMPTY_ENTRY;
                // write_clusters(cbuf.buf, cluster_number, 1);
            }

            // memcpy(parent_dir.table[i].name, "\0\0\0\0\0\0\0\0", 8);
            // memcpy(parent_dir.table[i].ext, "\0\0\0", 3);
            // parent_dir.table[i].attribute = 0;
            // parent_dir.table[i].user_attribute = 0;
            // parent_dir.table[i].undelete = 0;
            // parent_dir.table[i].create_time = 0;
            // parent_dir.table[i].create_date = 0;
            // parent_dir.table[i].access_date = 0;
            // parent_dir.table[i].modified_time = 0;
            // parent_dir.table[i].modified_date = 0;
            // parent_dir.table[i].filesize = 0;
            // parent_dir.table[i].cluster_low = 0;
            // parent_dir.table[i].cluster_high = 0;
           
            memset(parent_dir.table+i,0,sizeof(struct FAT32DirectoryEntry));
            write_clusters(driver_state.fat_table.cluster_map, FAT_CLUSTER_NUMBER, 1);
            write_clusters(parent_dir.table, request.parent_cluster_number, 1);

            return 0;
        }
    }
    return -1;
}
