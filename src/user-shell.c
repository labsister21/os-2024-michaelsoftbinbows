#include "./user-shell.h"

static char cmd_buffer[MAX_CMD_LENGTH];

static char current_path[MAX_CMD_LENGTH] = "root/";

static uint32_t working_directory = ROOT_CLUSTER_NUMBER;

static int cur_cmd_length = 0;

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
    int i = strlen(current_path)-2;
    for(; i > 0; i--){
        if(current_path[i] == '/') break;
    }
    if (i != 0) i++;

    char cur_dir[8];
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
    if(retcode != 0){
        syscall(6, (uint32_t) "Read dir failed", 15, 0xC);
        return;
    }
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
}

void ls(){

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
    
}

void cp(){
    char buf[MAX_CMD_LENGTH];
    memcpy(buf, (void*)cmd_buffer+3, cur_cmd_length-3);
    int i = 0;
    // parse for src and dest
    char src[8];
    memset(src, 0, 8);
    for(int j=0; j < 8 && buf[i] != '.' && buf[i] != ' ' && buf[i] != '\0'; j++, i++){
        src[j] = buf[i];
    }
    i++;
    char src_ext[3];
    memset(src_ext, 0, 3);
    for(int j=0; j < 3 && buf[i] != ' ' && buf[i] != '\0'; j++, i++){
        src_ext[j] = buf[i];
    }
    if (strlen(src_ext) > 0) i++;   

    char dest[8];
    memset(dest, 0, 8);
    for(int k = 0; k < 8 && buf[i] != '.' && buf[i] != '\0' && buf[i] != ' '; k++, i++){
        dest[k] = buf[i];
    }
    i++;
    char dest_ext[3];
    memset(dest_ext, 0, 3);
    for(int k = 0; k < 3 && buf[i] != '\0' && buf[i] != ' '; k++, i++){
        dest_ext[k] = buf[i];
    }

    // get current directory name
    int l = strlen(current_path)-2;
    for(; l > 0; l--){
        if(current_path[l] == '/') break;
    }
    if (l != 0) l++;

    char cur_dir[8];
    memset(cur_dir, 0, 8);
    for(int m = 0; current_path[l] != '/' && m < 8; m++, l++){
        cur_dir[m] = current_path[l];
    }

    // get current directory table
    struct FAT32DirectoryTable dir_table = {0};
    struct FAT32DriverRequest request = {
        .buf                   = &dir_table,
        .name                  = "\0\0\0\0\0\0\0",
        .ext                   = "\0\0",
        .parent_cluster_number = working_directory,
        .buffer_size           = sizeof(struct FAT32DirectoryTable),
    };
    memcpy(request.name, cur_dir, 8);

    int8_t retcode;
    syscall(1, (uint32_t) &request, (uint32_t) &retcode, 0);
    if (retcode != 0) {
        syscall(6, (uint32_t) "Read dir failed", 16, 0xC);
        return;
    }

    int found = 0;
    struct ClusterBuffer cl[1] = {0};
    struct FAT32DriverRequest request2 = {
        .buf                   = &cl,
        .name                  = "\0\0\0\0\0\0\0",
        .ext                   = "\0\0",
        .parent_cluster_number = working_directory,
        .buffer_size           = CLUSTER_SIZE,
    };
    memcpy(request2.name, src, 8);
    memcpy(request2.ext, src_ext, 8);
    for (int m = 2; m < 64; m++) {
        if (
            dir_table.table[m].user_attribute == UATTR_NOT_EMPTY
            && dir_table.table[m].attribute != ATTR_SUBDIRECTORY 
            && strcmp(src,dir_table.table[m].name) == 1 
            && memcmp(src_ext,dir_table.table[m].ext,3) == 0
            ) {
            
            uint32_t cluster_num = dir_table.table[m].cluster_high << 16 | dir_table.table[m].cluster_low;

            request2.parent_cluster_number = cluster_num;
            
            uint8_t ret;
            syscall(0, (uint32_t) &request2, (uint32_t) &ret, 0);

            found = 1;
        }
    }

    if (found != 1) {
        syscall(6,(uint32_t) "Src not found", 13, 0xC);
        return;
    }

    found = 0;
    for (int n = 2; n < 64; n++) {
        if (dir_table.table[n].user_attribute == UATTR_NOT_EMPTY 
        && strcmp(dest,dir_table.table[n].name) == 1 
        ) {
            if ((strlen(dest_ext) > 0 && dir_table.table[n].attribute != ATTR_SUBDIRECTORY)
            || (strlen(dest_ext) == 0 && dir_table.table[n].attribute == ATTR_SUBDIRECTORY) 
            ) {
                uint32_t cluster_num = dir_table.table[n].cluster_high << 16 | dir_table.table[n].cluster_low;

                if (cluster_num == request2.parent_cluster_number) continue; // if not src file

                request2.parent_cluster_number = cluster_num;

                uint8_t ret;
                syscall(2, (uint32_t) &request2, (uint32_t) &ret, 0);

                found = 1;
            }
        }
    }
    if (found != 1) {
        syscall(6,(uint32_t) "Dest not found", 14, 0xC);
        return;
    }

    syscall(6, (uint32_t) "Copy success", 13, 0xA);
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

void printPath(){
    for(uint8_t i = 0; i < MAX_CMD_LENGTH && current_path[i] != 0; i++){
        syscall(5, (uint32_t)&current_path[i], 0xD, 0);
    }
}

void template_print(){
    syscall(6, (uint32_t) "Binbows@IF-2230", 15, 0xB);
    syscall(6, (uint32_t) ":", 1, 0x7);
    syscall(6, (uint32_t) current_path, (uint32_t) strlen(current_path), 0xD);
    syscall(6, (uint32_t)"$", 1, 0x7);
    syscall(6, (uint32_t)" ", 1, 0xF);
}

int main(void) {
    // dummy file for testing
    struct ClusterBuffer      cl   = {0};
    char *owo = "KAAAAAA\n";
    memset(cl.buf, 0, CLUSTER_SIZE);
    memcpy(cl.buf, owo, 9);
    struct FAT32DriverRequest request = {
        .buf                   = &cl,
        .name                  = "kaa\0",
        .ext                   = "txt",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size           = sizeof(struct ClusterBuffer),
    };
    int8_t retcode;
    syscall(2, (uint32_t) &request, (uint32_t) &retcode, 0);

    syscall(7, 0, 0, 0);
    memset(cmd_buffer, 0, MAX_CMD_LENGTH);
    template_print();
    char buf;
    while (true) {
        syscall(4, (uint32_t) &buf, 0, 0);
        if(buf){
            if(buf == '\n'){
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
    if (strlen(path) >= 3 && memcmp(path, "..", 2) == 0) {
        if (working_directory != ROOT_CLUSTER_NUMBER) {
            uint32_t parent_cluster_num = dir_table.table[1].cluster_high << 16 | dir_table.table[1].cluster_low;
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

void find_file(char *name, struct FAT32DirectoryTable dir_table) {
    for (int i = 2; i < 64; i++) {
        if (dir_table.table[i].user_attribute == UATTR_NOT_EMPTY && dir_table.table[i].attribute != ATTR_SUBDIRECTORY && strcmp(name,dir_table.table[i].name) == 1) {
            uint32_t cluster_num = dir_table.table[i].cluster_high << 16 | dir_table.table[i].cluster_low;

            char disp = cluster_num + '0';
            syscall(5, (uint32_t)&disp, 0xD, 0);

            // return 0;
        }
    }
}