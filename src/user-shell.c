#include <stdint.h>
#include "header/filesystem/fat32.h"
#include "header/stdlib/string.h"

#define MAX_CMD_LENGTH 128

static char cmd_buffer[MAX_CMD_LENGTH];

static char current_path[MAX_CMD_LENGTH];

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
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size           = 0,
    };
    memcpy(request.name, real_name, 8);
    int32_t retcode;
    syscall(2, (uint32_t) &request, (uint32_t) &retcode, 0);
    if(retcode == 0){
        syscall(6, (uint32_t) "Write success", 13, 0xA);
    }else{
        syscall(6, (uint32_t) "Write failed", 12, 0xC);
    }
}

void cat(){

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
    printPath();
    syscall(6, (uint32_t)"$", 1, 0x7);
    syscall(6, (uint32_t)" ", 1, 0xF);
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
                exec();
                template_print();
            }else if(buf == '\b'){
                cmd_buffer[cur_cmd_length] = '\0';
                cur_cmd_length--;
            }else{
                usual(buf);
            }
        }
    }

    return 0;
}
