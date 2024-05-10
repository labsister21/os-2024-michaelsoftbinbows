#include <stdint.h>
#include "header/filesystem/fat32.h"
#include "header/stdlib/string.h"

#define MAX_CMD_LENGTH 128

int8_t change_dir(char *path, struct FAT32DirectoryTable);

void cd();
void mkdir();