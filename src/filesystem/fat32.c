#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../header/stdlib/string.h"
#include "../header/filesystem/fat32.h"
#include "../header/stdlib/string.h"

const uint8_t fs_signature[BLOCK_SIZE] = {
    'C', 'o', 'u', 'r', 's', 'e', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',  ' ',
    'D', 'e', 's', 'i', 'g', 'n', 'e', 'd', ' ', 'b', 'y', ' ', ' ', ' ', ' ',  ' ',
    'L', 'a', 'b', ' ', 'S', 'i', 's', 't', 'e', 'r', ' ', 'I', 'T', 'B', ' ',  ' ',
    'M', 'a', 'd', 'e', ' ', 'w', 'i', 't', 'h', ' ', '<', '3', ' ', ' ', ' ',  ' ',
    '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '2', '0', '2', '4', '\n',
    [BLOCK_SIZE-2] = 'O',
    [BLOCK_SIZE-1] = 'k',
};
/*
int memcmp (const void *str1, const void *str2, size_t count)
{
  register const unsigned char *s1 = (const unsigned char*)str1;
  register const unsigned char *s2 = (const unsigned char*)str2;

  while (count-- > 0)
    {
      if (*s1++ != *s2++)
	  return s1[-1] < s2[-1] ? -1 : 1;
    }
  return 0;
}

void *memset (void *dest, register int val, register size_t len)
{
  register unsigned char *ptr = (unsigned char*)dest;
  while (len-- > 0)
    *ptr++ = val;
  return dest;
}

void *memcpy (void *dest, const void *src, size_t len)
{
  char *d = dest;
  const char *s = src;
  while (len--)
    *d++ = *s++;
  return dest;
}*/

static struct FAT32DriverState fat32_driver = {};

/* -- Driver Interfaces -- */

/**
 * Convert cluster number to logical block address
 * 
 * @param cluster Cluster number to convert
 * @return uint32_t Logical Block Address
 */
uint32_t cluster_to_lba(uint32_t cluster){
    return cluster * CLUSTER_BLOCK_COUNT;
}

/**
 * Initialize DirectoryTable value with 
 * - Entry-0: DirectoryEntry about itself
 * - Entry-1: Parent DirectoryEntry
 * note: DirectoryEntry di index 0 cluster high & low undefined
 * 
 * @param dir_table          Pointer to directory table
 * @param name               8-byte char for directory name
 * @param parent_dir_cluster Parent directory cluster number
 */
void create_directory_table(struct FAT32DirectoryTable *dir_table, char *name, uint32_t parent_dir_cluster, uint32_t self_cluster){
    // DANGER!! JANGAN DIPAKE BUAT INIT ROOT!!
    memcpy(dir_table->table[0].name, name, 8);
    memset(dir_table->table[0].ext, 0, 3);
    dir_table->table[0].attribute = ATTR_SUBDIRECTORY;
    dir_table->table[0].user_attribute = UATTR_NOT_EMPTY;
    dir_table->table[0].cluster_high = (uint16_t)(self_cluster >> 16);
    dir_table->table[0].cluster_low = (uint16_t)(self_cluster & 0xFFFF);
    dir_table->table[0].filesize = 0;

    // init parent
    read_clusters(fat32_driver.dir_table_buf.table, parent_dir_cluster, 1);
    memcpy(dir_table->table[1].name, fat32_driver.dir_table_buf.table[0].name, 8);
    memset(dir_table->table[0].ext, 0, 3);
    dir_table->table[1].attribute = ATTR_SUBDIRECTORY;
    dir_table->table[1].user_attribute = UATTR_NOT_EMPTY;
    dir_table->table[1].cluster_high = (uint16_t)(parent_dir_cluster >> 16);
    dir_table->table[1].cluster_low = (uint16_t)(parent_dir_cluster & 0xFFFF);
    dir_table->table[1].filesize = 0;

    // init sisanya
    for(uint8_t i = 2; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); ++i){
        memset(dir_table->table[i].name, 0, 8);
        memset(dir_table->table[i].ext, 0, 3);
        dir_table->table[i].attribute = 0;
        dir_table->table[i].user_attribute = !UATTR_NOT_EMPTY;
        dir_table->table[i].cluster_high = 0;
        dir_table->table[i].cluster_low = 0;
        dir_table->table[i].filesize = 0;
    }
    write_clusters(dir_table, self_cluster, 1);
    
    fat32_driver.fat_table.cluster_map[self_cluster] = FAT32_FAT_END_OF_FILE;
    
    if(parent_dir_cluster == self_cluster && parent_dir_cluster == 2){
        return;
    }

    // update parent
    for(uint8_t i = 0; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); ++i){
        if(fat32_driver.dir_table_buf.table[i].user_attribute == UATTR_NOT_EMPTY){
            continue;
        }
        memcpy(fat32_driver.dir_table_buf.table[i].name, name, 8);
        memset(fat32_driver.dir_table_buf.table[i].ext, 0, 3);
        fat32_driver.dir_table_buf.table[i].attribute = ATTR_SUBDIRECTORY;
        fat32_driver.dir_table_buf.table[i].user_attribute = UATTR_NOT_EMPTY;
        fat32_driver.dir_table_buf.table[i].cluster_high = (uint16_t)(self_cluster >> 16);
        fat32_driver.dir_table_buf.table[i].cluster_low = (uint16_t)(self_cluster & 0xFFFF);
        fat32_driver.dir_table_buf.table[i].filesize = 0;
        break;
    }
    write_clusters(fat32_driver.dir_table_buf.table, parent_dir_cluster, 1);
}

/**
 * Checking whether filesystem signature is missing or not in boot sector
 * 
 * @return True if memcmp(boot_sector, fs_signature) returning inequality
 */
bool is_empty_storage(void){
    uint8_t boot_sector[BLOCK_SIZE];
    read_blocks(boot_sector, 0, 1);
    return memcmp(boot_sector, fs_signature, BLOCK_SIZE) != 0;
}

/**
 * Create new FAT32 file system. Will write fs_signature into boot sector and 
 * proper FileAllocationTable (contain CLUSTER_0_VALUE, CLUSTER_1_VALUE, 
 * and initialized root directory) into cluster number 1
 */


void create_fat32(void){
    uint8_t boot_sector[BLOCK_SIZE];
    memset(boot_sector, 0, BLOCK_SIZE);
    memcpy(boot_sector, fs_signature, BLOCK_SIZE);
    write_blocks(boot_sector, BOOT_SECTOR, 1);

    fat32_driver.fat_table.cluster_map[0] = CLUSTER_0_VALUE;
    fat32_driver.fat_table.cluster_map[1] = CLUSTER_1_VALUE;
    fat32_driver.fat_table.cluster_map[2] = FAT32_FAT_END_OF_FILE;
    for(int i = 3; i < CLUSTER_MAP_SIZE; ++i){
        fat32_driver.fat_table.cluster_map[i] = FAT32_FAT_EMPTY_ENTRY;
    }
    
    struct FAT32DirectoryTable rootTable = {};
    memcpy(rootTable.table[0].name, "root\0\0\0", 8);
    memcpy(rootTable.table[0].ext, "\0\0", 3);
    
    write_clusters(rootTable.table, 2, 1);

    struct FAT32DirectoryTable rootTable2 = {};

    create_directory_table(&rootTable2, "root\0\0\0", 2, 2);
}

/**
 * Initialize file system driver state, if is_empty_storage() then create_fat32()
 * Else, read and cache entire FileAllocationTable (located at cluster number 1) into driver state
 */
void initialize_filesystem_fat32(void){
    if (is_empty_storage()){
        create_fat32();
    }else{
        read_clusters(fat32_driver.fat_table.cluster_map, 1, 1);
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
void write_clusters(const void *ptr, uint32_t cluster_number, uint8_t cluster_count){
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
void read_clusters(void *ptr, uint32_t cluster_number, uint8_t cluster_count){
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
int8_t read_directory(struct FAT32DriverRequest request){
    if(request.buffer_size != sizeof(struct FAT32DirectoryTable)){
        return -1;
    }
    if(fat32_driver.fat_table.cluster_map[request.parent_cluster_number] != FAT32_FAT_END_OF_FILE){
        return -1;
    }
    uint8_t found_but_not_folder = 0;
    uint8_t found = 0;
    read_clusters(fat32_driver.dir_table_buf.table, request.parent_cluster_number, 1);
    for(uint8_t i = 0; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); ++i){
        if(fat32_driver.dir_table_buf.table[i].user_attribute != UATTR_NOT_EMPTY){
            continue;
        }
        if(memcmp(fat32_driver.dir_table_buf.table[i].name, request.name, 8) == 0){
            found = 1;
            if(fat32_driver.dir_table_buf.table[i].attribute == ATTR_SUBDIRECTORY){
                uint32_t cluster_num = (fat32_driver.dir_table_buf.table[i].cluster_high << 16) | fat32_driver.dir_table_buf.table[i].cluster_low;
                read_clusters(request.buf, cluster_num, 1);
                return 0;
            }else{
                found_but_not_folder = 1;
            }
        }
    }
    if(found_but_not_folder){
        return 1;
    }else if(!found){
        return 2;
    }
    return -1;
}


/**
 * FAT32 read, read a file from file system.
 *
 * @param request All attribute will be used for read, buffer_size will limit reading count
 * @return Error code: 0 success - 1 not a file - 2 not enough buffer - 3 not found - -1 unknown
 */
int8_t read(struct FAT32DriverRequest request){
    if(fat32_driver.fat_table.cluster_map[request.parent_cluster_number] != FAT32_FAT_END_OF_FILE){
        return -1;
    }
    read_clusters(fat32_driver.dir_table_buf.table, request.parent_cluster_number, 1);
    uint8_t found_but_not_file = 0;
    uint8_t found = 0;
    for(uint8_t i = 0; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); ++i){
        if(fat32_driver.dir_table_buf.table[i].user_attribute != UATTR_NOT_EMPTY){
            continue;
        }
        if(memcmp(fat32_driver.dir_table_buf.table[i].name, request.name, 8) == 0){
            found = 1;
            if(fat32_driver.dir_table_buf.table[i].attribute != ATTR_SUBDIRECTORY && memcmp(fat32_driver.dir_table_buf.table[i].ext, request.ext, 3) == 0){
                if(fat32_driver.dir_table_buf.table[i].filesize > request.buffer_size){
                    return 2;
                }
                uint32_t cluster_num = (fat32_driver.dir_table_buf.table[i].cluster_high << 16) | fat32_driver.dir_table_buf.table[i].cluster_low;
                while(cluster_num != FAT32_FAT_END_OF_FILE){
                    read_clusters(request.buf, cluster_num, 1);
                    request.buf += CLUSTER_SIZE;
                    cluster_num = fat32_driver.fat_table.cluster_map[cluster_num]; 
                }
                return 0;
            }else if(fat32_driver.dir_table_buf.table[i].attribute == ATTR_SUBDIRECTORY){
                found_but_not_file = 1;
            }
        }
    }
    if(found_but_not_file){
        return 1;
    }else if(!found){
        return 3;
    }
    return -1;
}

/**
 * FAT32 write, write a file or folder to file system.
 *
 * @param request All attribute will be used for write, buffer_size == 0 then create a folder / directory
 * @return Error code: 0 success - 1 file/folder already exist - 2 invalid parent cluster - -1 unknown
 */
int8_t write(struct FAT32DriverRequest request){
    if(fat32_driver.fat_table.cluster_map[request.parent_cluster_number] != FAT32_FAT_END_OF_FILE){
        return 2;
    }
    read_clusters(fat32_driver.dir_table_buf.table, request.parent_cluster_number, 1);
    uint8_t empty_slot_exist = 0;
    for(uint8_t i = 0; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); ++i){
        if(fat32_driver.dir_table_buf.table[i].user_attribute != UATTR_NOT_EMPTY){
            empty_slot_exist = 1;
            continue;
        }
        if(memcmp(fat32_driver.dir_table_buf.table[i].name, request.name, 8) == 0){
            if(fat32_driver.dir_table_buf.table[i].attribute == ATTR_SUBDIRECTORY && request.buffer_size == 0){
                return 1;
            }
            if(request.buffer_size != 0 && memcmp(fat32_driver.dir_table_buf.table[i].ext, request.ext, 3) == 0){
                return 1;
            }
        }
    }
    if(!empty_slot_exist){
        return -1;
    }
    uint32_t cluster_number_rounded_down = request.buffer_size / CLUSTER_SIZE;
    uint32_t remainder_from_cluster_size = request.buffer_size % CLUSTER_SIZE;
    uint32_t one_or_zero_remainder = 0;
    if(remainder_from_cluster_size){
        one_or_zero_remainder = 1;
    }
    uint32_t needed_size = cluster_number_rounded_down + one_or_zero_remainder;
    if(needed_size == 0){
        needed_size = 1;
    }
    uint32_t available_size = 0;
    for(uint32_t i = 0; i < CLUSTER_MAP_SIZE; i++){
        if(fat32_driver.fat_table.cluster_map[i] == FAT32_FAT_EMPTY_ENTRY){
            available_size++;
        }
    }
    if(needed_size > available_size){
        return -1;
    }
    uint32_t last_cluster_written = 0, first_cluster_written = 0;
    for(uint32_t i = 0, size_written = 0; i < CLUSTER_MAP_SIZE && size_written < needed_size; ++i){
        if(fat32_driver.fat_table.cluster_map[i] == FAT32_FAT_EMPTY_ENTRY){
            if(request.buffer_size != 0){
                write_clusters(request.buf, i, 1);
            }
            request.buf += CLUSTER_SIZE;
            size_written++;
            if(last_cluster_written != 0){
                fat32_driver.fat_table.cluster_map[last_cluster_written] = i;
            }else{
                first_cluster_written = i;
            }
            last_cluster_written = i; 
        }
    }
    fat32_driver.fat_table.cluster_map[last_cluster_written] = FAT32_FAT_END_OF_FILE;
    if(request.buffer_size == 0){
        struct FAT32DirectoryTable buffer2 = {};
        create_directory_table(&buffer2, request.name, request.parent_cluster_number, last_cluster_written);
        write_cluster_map();
        return 0;
    }else{
        read_clusters(fat32_driver.dir_table_buf.table, request.parent_cluster_number, 1);
        for(uint8_t i = 0; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); ++i){
            if(fat32_driver.dir_table_buf.table[i].user_attribute == UATTR_NOT_EMPTY){
                continue;
            }
            memcpy(fat32_driver.dir_table_buf.table[i].name, request.name, 8);
            memcpy(fat32_driver.dir_table_buf.table[i].ext, request.ext, 3);
            fat32_driver.dir_table_buf.table[i].attribute = !ATTR_SUBDIRECTORY;
            fat32_driver.dir_table_buf.table[i].user_attribute = UATTR_NOT_EMPTY;
            fat32_driver.dir_table_buf.table[i].cluster_high = (uint16_t)(first_cluster_written >> 16);
            fat32_driver.dir_table_buf.table[i].cluster_low = (uint16_t)(first_cluster_written & 0xFFFF);
            fat32_driver.dir_table_buf.table[i].filesize = needed_size * CLUSTER_SIZE;
            break;
        }
        write_clusters(fat32_driver.dir_table_buf.table, request.parent_cluster_number, 1);
        write_cluster_map();
        return 0;
    }
    return -1;
}


/**
 * FAT32 delete, delete a file or empty directory (only 1 DirectoryEntry) in file system.
 *
 * @param request buf and buffer_size is unused
 * @return Error code: 0 success - 1 not found - 2 folder is not empty - -1 unknown
 */
int8_t delete(struct FAT32DriverRequest request){
    if(fat32_driver.fat_table.cluster_map[request.parent_cluster_number] != FAT32_FAT_END_OF_FILE){
        return -1;
    }
    read_clusters(fat32_driver.dir_table_buf.table, request.parent_cluster_number, 1);
    uint8_t found = 0;
    for(uint8_t i = 0; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); ++i){
        if(fat32_driver.dir_table_buf.table[i].user_attribute != UATTR_NOT_EMPTY){
            continue;
        }
        if(memcmp(request.name, fat32_driver.dir_table_buf.table[i].name, 8) == 0 && memcmp(request.ext, fat32_driver.dir_table_buf.table[i].ext, 3) == 0){
            found = 1;
            if(fat32_driver.dir_table_buf.table[i].attribute == ATTR_SUBDIRECTORY){
                if(i == 0){ // tried to delete root
                    return -1;
                }
                uint8_t not_empty = 0;
                struct FAT32DirectoryTable secondDirTable = {};
                uint32_t cluster_num = (fat32_driver.dir_table_buf.table[i].cluster_high << 16) | (fat32_driver.dir_table_buf.table[i].cluster_low);
                read_clusters(secondDirTable.table, cluster_num, 1);
                for(uint8_t j = 3; j < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry) && !not_empty; ++j){
                    if(secondDirTable.table[j].user_attribute == UATTR_NOT_EMPTY){
                        not_empty = 1;
                    }
                }
                if(not_empty){
                    return 2;
                }
                memset(fat32_driver.dir_table_buf.table[i].name, 0, 8);
                memset(fat32_driver.dir_table_buf.table[i].ext, 0, 3);
                fat32_driver.dir_table_buf.table[i].attribute = 0;
                fat32_driver.dir_table_buf.table[i].user_attribute = !UATTR_NOT_EMPTY;
                fat32_driver.dir_table_buf.table[i].cluster_high = 0;
                fat32_driver.dir_table_buf.table[i].cluster_low = 0;
                fat32_driver.dir_table_buf.table[i].filesize = 0;
                write_clusters(fat32_driver.dir_table_buf.table, request.parent_cluster_number, 1);
                fat32_driver.fat_table.cluster_map[cluster_num] = FAT32_FAT_EMPTY_ENTRY;
                memset(fat32_driver.cluster_buf.buf, 0, CLUSTER_SIZE);
                write_clusters(fat32_driver.cluster_buf.buf, cluster_num, 1);
                write_cluster_map();
                return 0;
            }else{
                uint32_t cluster_num = (fat32_driver.dir_table_buf.table[i].cluster_high << 16) | (fat32_driver.dir_table_buf.table[i].cluster_low);
                memset(fat32_driver.dir_table_buf.table[i].name, 0, 8);
                memset(fat32_driver.dir_table_buf.table[i].ext, 0, 3);
                fat32_driver.dir_table_buf.table[i].attribute = 0;
                fat32_driver.dir_table_buf.table[i].user_attribute = !UATTR_NOT_EMPTY;
                fat32_driver.dir_table_buf.table[i].cluster_high = 0;
                fat32_driver.dir_table_buf.table[i].cluster_low = 0;
                fat32_driver.dir_table_buf.table[i].filesize = 0;
                write_clusters(fat32_driver.dir_table_buf.table, request.parent_cluster_number, 1);
                while(cluster_num != FAT32_FAT_END_OF_FILE){
                    uint32_t copy_cluster_num = fat32_driver.fat_table.cluster_map[cluster_num];
                    memset(fat32_driver.cluster_buf.buf, 0, CLUSTER_SIZE);
                    write_clusters(fat32_driver.cluster_buf.buf, cluster_num, 1);
                    fat32_driver.fat_table.cluster_map[cluster_num] = FAT32_FAT_EMPTY_ENTRY;
                    cluster_num = copy_cluster_num;
                }
                write_cluster_map();
                return 0;
            }
            break;
        }
    }
    if(!found){
        return 1;
    }
    return -1;
}

void write_cluster_map(void){
    write_clusters(fat32_driver.fat_table.cluster_map, 1, 1);
}

void end_filesystem_fat32(void){}