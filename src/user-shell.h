#include <stdint.h>
#include "header/filesystem/fat32.h"
#include "header/stdlib/string.h"

#define MAX_CMD_LENGTH 128

int32_t change_dir(char *path, struct FAT32DirectoryTable);
void find_file(char *name, struct FAT32DirectoryTable);

void cd();
void mkdir();
int cp();
int rm();
void find();
void cat();
void ls();
void clear();
void cpCaller();
void rmCaller();
void exec();
void ps();
void kill();