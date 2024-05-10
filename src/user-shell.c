#include "./user-shell.h"

static char cmd_buffer[MAX_CMD_LENGTH];

static char current_path[MAX_CMD_LENGTH] = "root/";

static uint32_t working_directory = ROOT_CLUSTER_NUMBER;

static uint8_t cur_cmd_length = 0;

//static uint8_t current_path_length = 0;

//static uint8_t current_dir_cluster_num = 2;

//static uint8_t parent_dir_cluster_num = 2;

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
    __asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));
    // Note : gcc usually use %eax as intermediate register,
    //        so it need to be the last one to mov
    __asm__ volatile("int $0x30");
}

void cd(){
    char cur_dir[8];
    int i = strlen(current_path)-2;
    for(; i > 0; i--){
        if(current_path[i] == '/') break;
    }
    if (i != 0) i++;

    memset(cur_dir, 0, 8);
    for(int j = 0; current_path[i] != '/' && j < 8; j++, i++){
        cur_dir[j] = current_path[i];
    }

    // get current directory table
    struct FAT32DirectoryTable cur_dir_table = {0};
    struct FAT32DriverRequest request = {
        .buf                   = &cur_dir_table,
        .name                  = "\0\0\0\0\0\0\0",
        .ext                   = "\0\0",
        .parent_cluster_number = working_directory,
        .buffer_size           = sizeof(struct FAT32DirectoryTable),
    };
    memcpy(request.name, cur_dir, 8);

    int8_t retcode;
    syscall(1, (uint32_t) &request, (uint32_t) &retcode, 0);
    if(retcode == 0){
        char name[MAX_CMD_LENGTH];
        memcpy(name, (void*)cmd_buffer + 3, cur_cmd_length - 3);
        char real_name[8];
        memset(real_name, 0, 8);
        for(uint8_t i = 0; i < 8 && i < cur_cmd_length - 3; i++){
            real_name[i] = name[i];
        }

        int32_t ret;
        ret = change_dir(real_name, cur_dir_table);
        if (ret == 0) {
            syscall(6, (uint32_t) "cd success", 10, 0xA);
        } else {
            syscall(6, (uint32_t) "cd failed", 9, 0xC);
        }
    }else{
        syscall(6, (uint32_t) "Read dir failed", 15, 0xC);
    }
}

void ls(){
    struct FAT32DirectoryTable      cl   = {0};
    char real_name[8];
    int8_t i;
    uint8_t current_path_length = strlen(current_path);
    for(i = current_path_length - 2; i >= 0; --i){
        if(current_path[i] == '/') break;
    }
    i++;
    memset(real_name, 0, 8);
    uint8_t j;
    for(j = 0; i < current_path_length - 1 && j < 8; j++, i++){
        real_name[j] = current_path[i];
    }
    char slashN = '\n';
    /*
    char slashN = '\n';
    syscall(5, (uint32_t)&slashN, 0xF, 0);
    */
   //syscall(6, (uint32_t)real_name, j, 0xF);
   //char wd = parent_dir_cluster_num + '0';
   //syscall(5, (uint32_t)&wd, 0xF, 0);
    struct FAT32DriverRequest request = {
        .buf                   = &cl,
        .name                  = "\0\0\0\0\0\0\0",
        .ext                   = "\0\0",
        .parent_cluster_number = working_directory,
        .buffer_size           = sizeof(struct FAT32DirectoryTable),
    };
    memcpy(request.name, real_name, 8);
    int32_t retcode;
    syscall(1, (uint32_t) &request, (uint32_t) &retcode, 0);
    if(retcode == 0){
        memcpy(real_name, ((struct FAT32DirectoryTable*)request.buf)->table[1].name, 8);
        uint32_t parent_cluster_num = ((struct FAT32DirectoryTable*)request.buf)->table[1].cluster_high << 16 | ((struct FAT32DirectoryTable*)request.buf)->table[1].cluster_low;
        request.parent_cluster_number = parent_cluster_num;
        memset(request.buf, 0, CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry));
        for(uint8_t i = 2; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++){
            if(cl.table[i].user_attribute == UATTR_NOT_EMPTY){
                uint8_t j;
                for(j = 0; j < 8 && cl.table[i].name[j] != '\0'; j++){}
                syscall(6, (uint32_t)cl.table[i].name, j, 0xF);
                if(cl.table[i].attribute != ATTR_SUBDIRECTORY){
                    uint8_t k;
                    for(k = 0; k < 3 && cl.table[i].ext[k] != '\0'; k++){}
                    syscall(6, (uint32_t)cl.table[i].ext, k, 0xF);
                }
                syscall(5, (uint32_t)&slashN, 0xF, 0);
            }
        }
    }else{
        char disp = retcode + '0';
        syscall(6, (uint32_t) "Read failed with code ", 22, 0xC);
        syscall(5, (uint32_t)&disp, 0xC, 0);
    }
}

void mkdir(){
    char name[MAX_CMD_LENGTH];
    memcpy(name, (void*)cmd_buffer + 6, cur_cmd_length - 6);
    char real_name[8];
    memset(real_name, 0, 8);
    for(uint8_t i = 0; i < 8 && i < cur_cmd_length - 6; i++){
        real_name[i] = name[i];
    }
    struct ClusterBuffer      cl[2]   = {0};
    struct FAT32DriverRequest request = {
        .buf                   = &cl,
        .name                  = "\0\0\0\0\0\0\0",
        .ext                   = "\0\0",
        .parent_cluster_number = working_directory,
        .buffer_size           = 0,
    };
    memcpy(request.name, real_name, 8);
    int8_t retcode;
    syscall(2, (uint32_t) &request, (uint32_t) &retcode, 0);
    if (retcode == 0) {
        syscall(6, (uint32_t) "Write success", 13, 0xA);
    }else{
        syscall(6, (uint32_t) "Write failed", 12, 0xC);
    }
}

void cat(){
    char name[MAX_CMD_LENGTH];
    memcpy(name, (void*)cmd_buffer + 4, cur_cmd_length - 4);
    char real_name[11];
    memset(real_name, 0, 11);
    uint8_t len_file_name = 0;
    int8_t len_pure_file_name = -1;
    for(uint8_t i = 0; i < 11 && i < cur_cmd_length - 4; i++, len_file_name++){
        real_name[i] = name[i];
        if(real_name[i] == '.') len_pure_file_name = i;
    }
    char pure_file_name[8];
    char pure_ext[3];
    uint8_t add_one = 1;
    memset(pure_file_name, 0, 8);
    memset(pure_ext, 0, 3);
    uint8_t bad = 0;
    if(len_pure_file_name == -1){
        len_pure_file_name = len_file_name;
        bad = 1;
    }
    memcpy(pure_file_name, real_name, len_pure_file_name);
    if(!bad) memcpy(pure_ext, (void*)real_name + len_pure_file_name + add_one, len_file_name - len_pure_file_name - add_one);
    struct FAT32DirectoryTable current_dir = {0};
    struct FAT32DriverRequest request = {
        .buf                   = current_dir.table,
        .name                  = "\0\0\0\0\0\0\0",
        .ext                   = "\0\0",
        .parent_cluster_number = working_directory,
        .buffer_size           = 0,
    };
    char current_dir_name[8];
    int8_t uu;
    uint8_t current_path_length = strlen(current_path);
    for(uu = current_path_length - 2; uu >= 0; --uu){
        if(current_path[uu] == '/') break;
    }
    if(uu == -1) uu++;
    memset(current_dir_name, 0, 8);
    uint8_t j;
    for(j = 0; uu < current_path_length - 1 && j < 8; j++, uu++){
        real_name[j] = current_path[uu];
    }
    memcpy(request.name, current_dir_name, 8);
    int32_t retcode;
    syscall(1, (uint32_t) &request, (uint32_t) &retcode, 0);
    uint32_t filesize;
    for(uint8_t i = 0; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); ++i){
        if(current_dir.table[i].user_attribute != UATTR_NOT_EMPTY){
            continue;
        }
        if(memcmp(current_dir.table[i].name, pure_file_name, 8) == 0){
            if(current_dir.table[i].attribute != ATTR_SUBDIRECTORY && memcmp(current_dir.table[i].ext, pure_ext, 3) == 0){
                filesize = current_dir.table[i].filesize;
                break;
            }
        }
    }
    filesize /= CLUSTER_SIZE;
    struct ClusterBuffer clb[3] = {0};
    memset(&clb, 0,  3 * CLUSTER_SIZE);
    request.buf = &clb;
    memset(request.name, 0, 8);
    memcpy(request.name, pure_file_name, 8);
    memset(request.ext, 0, 3);
    memcpy(request.ext, pure_ext, 3);
    request.buffer_size = 3 * CLUSTER_SIZE;
    syscall(0, (uint32_t) &request, (uint32_t) &retcode, 0);
    if(retcode != 0){
        char disp = retcode + '0';
        syscall(6, (uint32_t) "Read failed with code ", 22, 0xC);
        syscall(5, (uint32_t)&disp, 0xC, 0);
    }else{
        uint32_t len = 0;
        for(uint32_t i = 0; i < 3 * CLUSTER_SIZE; i++){
            uint8_t uwu = *(clb[0].buf + i);
            if(uwu == 0) break;
            len++;
        }
        syscall(6, (uint32_t)clb[0].buf, len, 0xF);
    }

}

void cp(){

}

void rm(){

}

void mv(){

}

void find(){

}

void exec(){
    char command[MAX_CMD_LENGTH];
    int cmd_length = 0;
    memset(command, 0, MAX_CMD_LENGTH);
    for(int i = 0; i < cur_cmd_length; i++){
        if(cmd_buffer[i] == ' ') break;
        cmd_length++;
    }
    memcpy(command, cmd_buffer, cmd_length);
    if(cmd_length == 2 && memcmp(command, "cd", 2) == 0){
        cd();
    }else if(cmd_length == 2 && memcmp(command, "ls", 2) == 0){
        ls();
    }else if(cmd_length == 5 && memcmp(command, "mkdir", 5) == 0){
        mkdir();
    }else if(cmd_length == 3 && memcmp(command, "cat", 3) == 0){
        cat();
    }else if(cmd_length == 2 && memcmp(command, "cp", 2) == 0){
        cp();
    }else if(cmd_length == 2 && memcmp(command, "rm", 2) == 0){
        rm();
    }else if(cmd_length == 2 && memcmp(command, "mv", 2) == 0){
        mv();
    }else if(cmd_length == 4 && memcmp(command, "find", 4) == 0){
        find();
    }else{

    }
    memset(cmd_buffer, 9, MAX_CMD_LENGTH);
    char slashN = '\n';
    syscall(5, (uint32_t)&slashN, 0xF, 0);
    cur_cmd_length = 0;
}

void usual(char buf){
    if (cur_cmd_length >= MAX_CMD_LENGTH) {
        return;
    }

    cmd_buffer[cur_cmd_length] = buf;
    cur_cmd_length++;
    syscall(5, (uint32_t) &buf, 0xF, 0);
}

uint8_t printPath(){
    uint8_t i = 0;
    for(; i < MAX_CMD_LENGTH && current_path[i] != 0; i++){
        syscall(5, (uint32_t)&current_path[i], 0xD, 0);
    }
    return i;
}

void template_print(){
    syscall(6, (uint32_t) "Binbows@IF-2230", 15, 0xB);
    syscall(6, (uint32_t) ":", 1, 0x7);
    syscall(6, (uint32_t) current_path, (uint32_t) strlen(current_path), 0xD);
    syscall(6, (uint32_t)"$", 1, 0x7);
    syscall(6, (uint32_t)" ", 1, 0xF);
    uint8_t x = strlen(current_path) + 18;
    syscall(19, (uint32_t)&x, 0, 0);
}

int main(void) {
    /*
    struct ClusterBuffer      cl[2]   = {0};
    struct FAT32DriverRequest request = {
        .buf                   = &cl,
        .name                  = "shell",
        .ext                   = "\0\0\0",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size           = CLUSTER_SIZE,
    };
    int32_t retcode;
    syscall(0, (uint32_t) &request, (uint32_t) &retcode, 0);
    syscall(7, 0, 0, 0);
    if (retcode == 0)
        syscall(6, (uint32_t) "owo", 3, 0xF);*/
    syscall(7, 0, 0, 0);
    memset(cmd_buffer, 0, MAX_CMD_LENGTH);
    memset(current_path, 0, MAX_CMD_LENGTH);
    current_path[0] = 'r';
    current_path[1] = 'o';
    current_path[2] = 'o';
    current_path[3] = 't';
    current_path[4] = '/';
    template_print();
    char buf;
    while (true) {
        syscall(4, (uint32_t) &buf, 0, 0);
        if(buf){
            if(buf == '\n'){
                syscall(5, (uint32_t)&buf, 0, 0);
                exec();
                template_print();
            }else if(buf == '\b'){
                if (cur_cmd_length > 0) {
                    syscall(5, (uint32_t) &buf, 0, 0);
                    cmd_buffer[cur_cmd_length] = '\0';
                    cur_cmd_length--;
                }
            }else{
                usual(buf);
            }
        }
    }

    return 0;
}

int32_t change_dir(char *path, struct FAT32DirectoryTable dir_table) {
    uint32_t parent_cluster_num = dir_table.table[1].cluster_high << 16 | dir_table.table[1].cluster_low;

    if (strlen(path) >= 3 && memcmp(path, "../", 3) == 0) {
        if (working_directory != ROOT_CLUSTER_NUMBER) {
            working_directory = parent_cluster_num;
            int n = strlen(current_path), i=n-1;
            for(; i > 4; i--){
                if(current_path[i] == '/' && i < n-2) break;
                current_path[i] = '\0';
            }

            return 0;
        }
    }
    else{
        for (int i = 2; i < 64; i++) {
            if (dir_table.table[i].user_attribute == UATTR_NOT_EMPTY && dir_table.table[i].attribute == ATTR_SUBDIRECTORY && strcmp(path,dir_table.table[i].name) == 1) {
                strcat(current_path,path);
                strcat(current_path,"/");

                uint32_t cluster_num = dir_table.table[i].cluster_high << 16 | dir_table.table[i].cluster_low;

                working_directory = cluster_num;

                return 0;
            }
        }
    }

    return 1;
}