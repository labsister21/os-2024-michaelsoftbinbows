#include "./user-shell.h"

static char cmd_buffer[MAX_CMD_LENGTH];

static char current_path[MAX_CMD_LENGTH] = "root/";

static uint32_t working_directory = ROOT_CLUSTER_NUMBER;

static uint8_t cur_cmd_length = 0;

//static uint8_t current_path_length = 0;

// static uint8_t current_dir_cluster_num = 2;

//static uint8_t parent_dir_cluster_num = 2;

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx)
{
    __asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));
    // Note : gcc usually use %eax as intermediate register,
    //        so it need to be the last one to mov
    __asm__ volatile("int $0x30");
}

// handle multiple cd: relative or absolute
uint8_t multiple_cd(char* path, uint8_t len_path){
    char saved_current_path[MAX_CMD_LENGTH];
    char last_folder_name[MAX_CMD_LENGTH];
    int32_t find_fold_name;
    int32_t length_fold_name = 0;
    for(find_fold_name = strlen(current_path) - 2; find_fold_name >= 0; --find_fold_name){
        if(current_path[find_fold_name] == '/') break;
        length_fold_name++;
    }
    find_fold_name++;
    memset(last_folder_name, 0, MAX_CMD_LENGTH);
    memcpy(last_folder_name, (void*)current_path + find_fold_name, length_fold_name);

    uint32_t saved_working_directory = working_directory;
    memcpy(saved_current_path, current_path, MAX_CMD_LENGTH);
    char path_buffer[8];
    uint8_t path_buffer_p = 0;
    memset(path_buffer, 0, 8);
    for(uint8_t i = 0; i < len_path; i++){
        if(path[i] == '/') break;
        if(path_buffer_p >= 8){
            working_directory = saved_working_directory;
            memcpy(current_path, saved_current_path, MAX_CMD_LENGTH);
            return 1;
        }
        path_buffer[path_buffer_p] = path[i];
        path_buffer_p++;
    }
    uint8_t i = 0;
    // abs path
    if(path_buffer_p == 4 && memcmp(path_buffer, "root", 4) == 0){
        working_directory = 2;
        memset(current_path, 0, MAX_CMD_LENGTH);
        memcpy(current_path, "root/", 5);
        memset(last_folder_name, 0, 8);
        memcpy(last_folder_name, "root", 4);
        i += 5;
    }
    uint8_t buf_exist = 0;
    path_buffer_p = 0;
    memset(path_buffer, 0, 8);
    for(; i < len_path; i++){
        if(path[i] == '/'){
            if(!buf_exist){
                working_directory = saved_working_directory;
                memcpy(current_path, saved_current_path, MAX_CMD_LENGTH);
                return 1;
            }else{
                if(path_buffer_p > 8){
                    working_directory = saved_working_directory;
                    memcpy(current_path, saved_current_path, MAX_CMD_LENGTH);
                    return 1;
                }
                struct FAT32DirectoryTable cur_dir_table = {0};
                struct FAT32DriverRequest request = {
                    .buf                   = &cur_dir_table,
                    .name                  = "\0\0\0\0\0\0\0",
                    .ext                   = "\0\0",
                    .parent_cluster_number = working_directory,
                    .buffer_size           = sizeof(struct FAT32DirectoryTable),
                };
                memcpy(request.name, last_folder_name, 8);

                int8_t retcode;
                syscall(1, (uint32_t) &request, (uint32_t) &retcode, 0);
                if(retcode == 0){
                    int32_t ret;
                    ret = change_dir(path_buffer, cur_dir_table);
                    if(ret != 0) {
                        working_directory = saved_working_directory;
                        memcpy(current_path, saved_current_path, MAX_CMD_LENGTH);
                        return 1;
                    }
                    memset(last_folder_name, 0, MAX_CMD_LENGTH);
                    memcpy(last_folder_name, path_buffer, 8);
                    buf_exist = 0;
                    path_buffer_p = 0;
                    memset(path_buffer, 0, 8);
                }else{
                    working_directory = saved_working_directory;
                    memcpy(current_path, saved_current_path, MAX_CMD_LENGTH);
                    return 1;
                }
            }
        }else{
            if(path_buffer_p >= 8){
                working_directory = saved_working_directory;
                memcpy(current_path, saved_current_path, MAX_CMD_LENGTH);
                return 1;
            }
            buf_exist = 1;
            path_buffer[path_buffer_p] = path[i];
            path_buffer_p++;
        }
    }
    if(buf_exist){
        if(path_buffer_p > 8){
            working_directory = saved_working_directory;
            memcpy(current_path, saved_current_path, MAX_CMD_LENGTH);
            return 1;
        }
        struct FAT32DirectoryTable cur_dir_table = {0};
        struct FAT32DriverRequest request = {
            .buf                   = &cur_dir_table,
            .name                  = "\0\0\0\0\0\0\0",
            .ext                   = "\0\0",
            .parent_cluster_number = working_directory,
            .buffer_size           = sizeof(struct FAT32DirectoryTable),
        };
        memcpy(request.name, last_folder_name, 8);
        int8_t retcode;
        syscall(1, (uint32_t) &request, (uint32_t) &retcode, 0);
        if(retcode == 0){
            int32_t ret;
            ret = change_dir(path_buffer, cur_dir_table);
            if(ret != 0) {
                working_directory = saved_working_directory;
                memcpy(current_path, saved_current_path, MAX_CMD_LENGTH);
                return 1;
            }
            memset(last_folder_name, 0, MAX_CMD_LENGTH);
            memcpy(last_folder_name, path_buffer, 8);
            buf_exist = 0;
            path_buffer_p = 0;
            memset(path_buffer, 0, 8);
        }else{
            working_directory = saved_working_directory;
            memcpy(current_path, saved_current_path, MAX_CMD_LENGTH);
            return 1;
        }
    }
    return 0;
}

void cd(){
    uint8_t cmd_len = strlen(cmd_buffer);
    char mult_path[MAX_CMD_LENGTH];
    memset(mult_path, 0, MAX_CMD_LENGTH);
    memcpy(mult_path, (void*)cmd_buffer + 3, cmd_len - 3);
    uint8_t ret = multiple_cd(mult_path, cmd_len - 3);
    if(ret != 0){
        syscall(6, (uint32_t) "cd failed", 9, 0xC);
    }else{
        syscall(6, (uint32_t) "cd success", 10, 0xA);
    }
}

void ls(){
    struct FAT32DirectoryTable      cl   = {0};
    char args[MAX_CMD_LENGTH];
    uint8_t cmd_len = strlen(cmd_buffer);
    char saved_current_path[MAX_CMD_LENGTH];
    uint32_t saved_working_directory = working_directory;
    uint8_t arg_exist = 0;

    if(cmd_len > 2) {

    memcpy(args, (void*)cmd_buffer + 3, cmd_len - 3);
    for(uint8_t i = 0; i < MAX_CMD_LENGTH && !arg_exist; i++){
        if(args[i] == '\0') break;
        if(args[i] != ' ') arg_exist = 1;
    }
    if(arg_exist){
        memset(saved_current_path, 0, MAX_CMD_LENGTH);
        memcpy(saved_current_path, current_path, MAX_CMD_LENGTH);
        uint8_t ret = multiple_cd(args, cmd_len - 3);
        if(ret != 0){
            char disp = ret + '0';
            syscall(6, (uint32_t) "Read failed with code ", 22, 0xC);
            syscall(5, (uint32_t)&disp, 0xC, 0);
            memset(current_path, 0, MAX_CMD_LENGTH);
            memcpy(current_path, saved_current_path, MAX_CMD_LENGTH);
            working_directory = saved_working_directory;
            return;
        }
    }

    }

    char real_name[8];
    int8_t i;
    uint8_t current_path_length = strlen(current_path);
    for(i = current_path_length - 2; i >= 0; --i){
        if(current_path[i] == '/') break;
    }
    i++;
    memset(real_name, 0, 8);
    uint8_t j;
    for (j = 0; i < current_path_length - 1 && j < 8; j++, i++)
    {
        real_name[j] = current_path[i];
    }
    char slashN = '\n';
    char dot = '.';
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
                for (j = 0; j < 8 && cl.table[i].name[j] != '\0'; j++)
                {
                }
                syscall(6, (uint32_t)cl.table[i].name, j, 0xF);
                if (cl.table[i].attribute != ATTR_SUBDIRECTORY)
                {
                    uint8_t k;
                    for (k = 0; k < 3 && cl.table[i].ext[k] != '\0'; k++)
                    {
                    }
                    if(k>0)syscall(5,(uint32_t)&dot,0xF,0);
                    syscall(6, (uint32_t)cl.table[i].ext, k, 0xF);
                }
                syscall(5, (uint32_t)&slashN, 0xF, 0);
            }
        }
    }
    else
    {
        char disp = retcode + '0';
        syscall(6, (uint32_t) "Read failed with code ", 22, 0xC);
        syscall(5, (uint32_t)&disp, 0xC, 0);
    }
}

void mkdir(){
    char name[MAX_CMD_LENGTH];
    uint8_t cmd_len = strlen(cmd_buffer);

    if(cmd_len == 5){
        syscall(6, (uint32_t) "No arguments!", 12, 0xC);
        return;
    }
    char args[MAX_CMD_LENGTH];
    memcpy(args, (void*)cmd_buffer + 6, cur_cmd_length - 6);
    char saved_current_path[MAX_CMD_LENGTH];
    uint32_t saved_working_directory = working_directory;
    uint8_t arg_exist = 0;
    uint8_t file_name_ext_len;
    for(uint8_t i = 0; i < cur_cmd_length - 4 && !arg_exist; i++){
        if(args[i] == '/') arg_exist = 1;
    }

    if(arg_exist){
        int8_t length_file_name;
        for(length_file_name = cur_cmd_length - 1; length_file_name >= 0; --length_file_name){
            if(cmd_buffer[length_file_name] == '/') break;
        }
        length_file_name++;
        char file_path[MAX_CMD_LENGTH];
        memcpy(file_path, (void*)cmd_buffer + 6, length_file_name - 7);
        memset(saved_current_path, 0, MAX_CMD_LENGTH);
        memcpy(saved_current_path, current_path, MAX_CMD_LENGTH);
        memcpy(name, (void*)cmd_buffer + length_file_name, cur_cmd_length - length_file_name);
        uint8_t ret = multiple_cd(file_path, length_file_name - 7);
        file_name_ext_len = cur_cmd_length - length_file_name;
        if(ret != 0){
            char disp = ret + '0';
            syscall(6, (uint32_t) "Failed changing dir with code ", 30, 0xC);
            syscall(5, (uint32_t)&disp, 0xC, 0);
            memset(current_path, 0, MAX_CMD_LENGTH);
            memcpy(current_path, saved_current_path, MAX_CMD_LENGTH);
            working_directory = saved_working_directory;
            return;
        }
    }else{
        memcpy(name, (void*)cmd_buffer + 6, cur_cmd_length - 6);
        file_name_ext_len = cur_cmd_length - 6;
    }
    char real_name[8];
    memset(real_name, 0, 8);
    for(uint8_t i = 0; i < 8 && i < file_name_ext_len; i++){
        real_name[i] = name[i];
    }
    if(strlen(real_name) == 4 && memcmp(real_name, "root", 4) == 0){
        syscall(6, (uint32_t) "Cannot name folder 'root'", 25, 0xC);
        return;
    }
    struct ClusterBuffer      cl   = {0};
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
    if(arg_exist){
        memset(current_path, 0, MAX_CMD_LENGTH);
        memcpy(current_path, saved_current_path, MAX_CMD_LENGTH);
        working_directory = saved_working_directory;
        return;
    }
}

void cpCaller()
{
    int retcode = cp();
    switch (retcode)
    {
    case 0:
        syscall(6, (uint32_t) "Copy success", 13, 0xA);
        break;
    case 1:
        syscall(6, (uint32_t) "Read dir failed", 16, 0xC);
        break;
    case 2:
        syscall(6,(uint32_t) "Src not found", 13, 0xC);
        break;
    case 3:
        syscall(6,(uint32_t) "Write failed", 13, 0xC);
        break;
    case 4:
        syscall(6,(uint32_t) "Dest not found", 14, 0xC);
        break;
    case 5:
        syscall(6,(uint32_t) "Write failed", 13, 0xC);
        break;
    default:
        break;
    }
    
}

int cp(){
    char buf[MAX_CMD_LENGTH];
    memset(buf, 0, MAX_CMD_LENGTH);
    memcpy(buf, (void*)cmd_buffer+3, cur_cmd_length-3);
    int i = 0;
    // parse for src and dest
    char src[MAX_CMD_LENGTH];
    memset(src, 0, MAX_CMD_LENGTH);
    for(int j=0; j < MAX_CMD_LENGTH && buf[i] != ' ' && buf[i] != '\0'; j++, i++){ 
        if (buf[i] == '.') {
            if (i < MAX_CMD_LENGTH-1 && buf[i+1] == '.') {
                src[j] = buf[i];
                j++;
                i++;
            }
            else break;
        }
        src[j] = buf[i];
    } 
    src[i+1] = '\0';
    i++;
    char src_ext[3];
    memset(src_ext, 0, 3);
    for(int j=0; j < 3 && buf[i] != ' ' && buf[i] != '\0'; j++, i++){
        src_ext[j] = buf[i];
    }
    if (strlen(src_ext) == 0) {
        return 6;
    }
    i++; // asumsi format standar

    char dest[MAX_CMD_LENGTH];
    memset(dest, 0, MAX_CMD_LENGTH);
    for(int k = 0; k < MAX_CMD_LENGTH && buf[i] != '\0' && buf[i] != ' '; k++, i++){
        if (buf[i] == '.') {
            if (i < MAX_CMD_LENGTH-1 && buf[i+1] == '.') {
                dest[k] = buf[i];
                k++;
                i++;
            }
            else break;
        }
        dest[k] = buf[i];
    }
    dest[i+1] = '\0';
    i++;
    char dest_ext[3];
    memset(dest_ext, 0, 3);
    for(int k = 0; k < 3 && buf[i] != '\0' && buf[i] != ' '; k++, i++){
        dest_ext[k] = buf[i];
    }

    char src_name[8];
    memset(src_name,0,8);
    int j = strlen(src);
    for(; j > 0; j--){
        if (src[j] == '/') break;
    }
    if (j != 0) j++;
    for (int k=0; k < 8 && src[j] != '\0'; j++, k++) {
        src_name[k] = src[j];
        src[j] = '\0';
    }

    char dest_name[8];
    memset(dest_name,0,8);
    j = strlen(dest);
    for(; j > 0; j--){
        if (dest[j] == '/') break;
    }
    if (j != 0) j++;
    for (int k=0; k < 8 && dest[j] != '\0'; j++, k++) {
        dest_name[k] = dest[j];
        dest[j] = '\0';
    }

    uint32_t save_directory = working_directory;
    char save_path[MAX_CMD_LENGTH];
    memset(save_path, 0, MAX_CMD_LENGTH);
    memcpy(save_path, current_path, MAX_CMD_LENGTH);

    // cd to intended src path
    uint8_t ret1 = multiple_cd(src,strlen(src));
    if (ret1 != 0) {
        memcpy(current_path, save_path, MAX_CMD_LENGTH);
        working_directory = save_directory;
        return 2; 
    }

    struct ClusterBuffer cl[3] = {0};
    struct FAT32DriverRequest request = {
        .buf                   = &cl,
        .name                  = "\0\0\0\0\0\0\0",
        .ext                   = "\0\0",
        .parent_cluster_number = working_directory,
        .buffer_size           = 3*CLUSTER_SIZE,
    };
    memcpy(request.name, src_name, 8);
    memcpy(request.ext, src_ext, 3);
    uint8_t ret2;
    syscall(0, (uint32_t) &request, (uint32_t) &ret2, 0);

    memcpy(current_path, save_path, MAX_CMD_LENGTH);
    working_directory = save_directory;
    if (ret2 != 0) {
        return 2;
    }

    // cd to intended dest path
    uint8_t ret3 = multiple_cd(dest,strlen(dest));
    if (ret3 != 0) {
        memcpy(current_path, save_path, MAX_CMD_LENGTH);
        working_directory = save_directory;
        return 4; 
    }

    if (strlen(dest_ext) == 0) {
        // get directory name
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

        // get directory table
        struct FAT32DirectoryTable dir_table = {0};
        struct FAT32DriverRequest request2 = {
            .buf                   = &dir_table,
            .name                  = "\0\0\0\0\0\0\0",
            .ext                   = "\0\0",
            .parent_cluster_number = working_directory,
            .buffer_size           = sizeof(struct FAT32DirectoryTable),
        };
        memcpy(request2.name, cur_dir, 8);

        // return path & directory to normal
        memcpy(current_path, save_path, MAX_CMD_LENGTH);
        working_directory = save_directory;

        int8_t retcode;
        syscall(1, (uint32_t) &request2, (uint32_t) &retcode, 0);
        if (retcode != 0) {
            return 1;
        }
        int found = 0;
        for (int n = 2; n < 64; n++) {
            if (dir_table.table[n].user_attribute == UATTR_NOT_EMPTY 
            && strcmp(dest_name,dir_table.table[n].name) == 1 
            && dir_table.table[n].attribute == ATTR_SUBDIRECTORY
            ) {
                uint32_t cluster_num = dir_table.table[n].cluster_high << 16 | dir_table.table[n].cluster_low;

                request.parent_cluster_number = cluster_num;

                uint8_t ret;
                syscall(2, (uint32_t) &request, (uint32_t) &ret, 0);

                if (ret != 0) {
                    return 3;
                }

                found = 1;
            }
        }
        if (found != 1) {
            return 4;
        }
    }
    else {
        // return path & directory to normal
        memcpy(current_path, save_path, MAX_CMD_LENGTH);
        working_directory = save_directory;

        request.parent_cluster_number = working_directory;
        memcpy(request.name, dest_name, 8);
        memcpy(request.ext, dest_ext, 3);
        uint8_t ret;
        syscall(2, (uint32_t) &request, (uint32_t) &ret, 0);

        if (ret != 0) {
            return 5;
        }
    }
    
    return 0;
}

void cat(){
    char name[MAX_CMD_LENGTH];

    uint8_t cmd_len = strlen(cmd_buffer);

    if(cmd_len == 3){
        syscall(6, (uint32_t) "No arguments!", 12, 0xC);
        return;
    }

    char args[MAX_CMD_LENGTH];
    memcpy(args, (void*)cmd_buffer + 4, cur_cmd_length - 4);
    char saved_current_path[MAX_CMD_LENGTH];
    uint32_t saved_working_directory = working_directory;
    uint8_t arg_exist = 0;
    uint8_t file_name_ext_len;
    for(uint8_t i = 0; i < cur_cmd_length - 4 && !arg_exist; i++){
        if(args[i] == '/') arg_exist = 1;
    }

    if(arg_exist){
        int8_t length_file_name;
        for(length_file_name = cur_cmd_length - 1; length_file_name >= 0; --length_file_name){
            if(cmd_buffer[length_file_name] == '/') break;
        }
        length_file_name++;
        char file_path[MAX_CMD_LENGTH];
        memcpy(file_path, (void*)cmd_buffer + 4, length_file_name - 5);
        memset(saved_current_path, 0, MAX_CMD_LENGTH);
        memcpy(saved_current_path, current_path, MAX_CMD_LENGTH);
        memcpy(name, (void*)cmd_buffer + length_file_name, cur_cmd_length - length_file_name);
        uint8_t ret = multiple_cd(file_path, length_file_name - 5);
        file_name_ext_len = cur_cmd_length - length_file_name;
        if(ret != 0){
            char disp = ret + '0';
            syscall(6, (uint32_t) "Failed changing dir with code ", 30, 0xC);
            syscall(5, (uint32_t)&disp, 0xC, 0);
            memset(current_path, 0, MAX_CMD_LENGTH);
            memcpy(current_path, saved_current_path, MAX_CMD_LENGTH);
            working_directory = saved_working_directory;
            return;
        }
    }else{
        memcpy(name, (void*)cmd_buffer + 4, cur_cmd_length - 4);
        file_name_ext_len = cur_cmd_length - 4;
    }
    char real_name[11];
    memset(real_name, 0, 11);
    uint8_t len_file_name = 0;
    int8_t len_pure_file_name = -1;
    for(uint8_t i = 0; i < 11 && i < file_name_ext_len; i++, len_file_name++){
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
    uint8_t retcode;
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
    if(arg_exist){
        memset(current_path, 0, MAX_CMD_LENGTH);
        memcpy(current_path, saved_current_path, MAX_CMD_LENGTH);
        working_directory = saved_working_directory;
        return;
    }
}

int32_t rm()
{
    // struct ClusterBuffer cl[2];
    struct FAT32DriverRequest request = {
        .name = "\0\0\0\0\0\0\0",
        .ext = "\0\0",
        .parent_cluster_number = working_directory,
    };
    int nameLen = 0;
    char *itr = (char *)cmd_buffer + 3;
    for (int i = 0; i < strlen(itr); i++)
    {
        if (itr[i] == '.')
        {
            // identifies what's the extension for the file is
            // breaks from the loop
            request.ext[0] = itr[i + 1];
            request.ext[1] = itr[i + 2];
            request.ext[2] = itr[i + 3];
            break;
        }
        else
        {
            nameLen++;
        }
    }
    memcpy(request.name, (void *)(cmd_buffer + 3), nameLen);
    int32_t retcode;
    syscall(3, (uint32_t)&request, (uint32_t)&retcode, 0);
    return retcode;
}

void rmCaller()
{
    int retcode = rm();
    if (retcode == 0)
    {
        syscall(6, (uint32_t) "Delete Succeeded !!! ", 21, 0xA);
    }
    else if (retcode == 1)
    {
        syscall(6, (uint32_t) "File/Folder Not Found ", 22, 0xC);
    }
    else if (retcode == 2)
    {
        syscall(6, (uint32_t) "Folder is empty ", 16, 0xC);
    }
    else
    {
        syscall(6, (uint32_t) "Ada apa ini ", 12, 0xC);
    }
}

void mv()
{
    // what the fuck?
    int cpRetcode = cp();
    if (cpRetcode == 0){
        rm();
        syscall(6, (uint32_t) "move success!", 14, 0xA);
    }
    else{
        switch (cpRetcode)
        {
            case 1:
                syscall(6, (uint32_t) "Read dir failed", 16, 0xC);
                break;
            case 2:
                syscall(6,(uint32_t) "Src not found", 13, 0xC);
                break;
            case 3:
                syscall(6,(uint32_t) "Write failed", 13, 0xC);
                break;
            case 4:
                syscall(6,(uint32_t) "Dest not found", 14, 0xC);
                break;
            case 5:
                syscall(6,(uint32_t) "Write failed", 13, 0xC);
                break;
            default:
                break;
        }
    }
}


void findHelper(char* target,int prev,char* local_current_path, uint32_t local_working_directory, struct FAT32DirectoryTable cl){
    char real_name[8];
    int8_t i;
    uint8_t current_path_length = strlen(local_current_path);
    for(i = current_path_length - 2; i >= 0; --i){
        if(local_current_path[i] == '/') break;
    }
    i++;
    memset(real_name, 0, 8);
    uint8_t j;
    for(j = 0; i < current_path_length - 1 && j < 8; j++, i++){
        real_name[j] = local_current_path[i];
    }
    for (int i = 2; i < 64; i++) {
            if (cl.table[i].user_attribute == UATTR_NOT_EMPTY && cl.table[i].attribute == ATTR_SUBDIRECTORY && strcmp(real_name,cl.table[i].name) == 1) {
                local_working_directory = cl.table[i].cluster_high << 16 | cl.table[i].cluster_low;
                break;
            }
        }
    struct FAT32DriverRequest request = {
        .buf                   = &cl,
        .name                  = "\0\0\0\0\0\0\0",
        .ext                   = "\0\0",
        .parent_cluster_number = local_working_directory,
        .buffer_size           = sizeof(struct FAT32DirectoryTable),
    };
    memcpy(request.name, real_name, 8);
    char slashN = '\n';
    char dot = '.';
    int32_t retcode;
    retcode = 0;
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
                    char slash_target[128]; // Make sure this size is enough for your needs
                    slash_target[0] = '/';
                    int k;
                    for (k = 0; target[k] != '\0'; k++) {slash_target[k + 1] = target[k];}
                    slash_target[k + 1] = '/'; slash_target[k + 2] = '\0';
                    if( strcmp(cl.table[i].name,target)|| strstr(local_current_path,slash_target) || strlen(target)==0){
                    char* trimmed_current_path = &local_current_path[prev];
                    syscall(6, (uint32_t)"./", j, 0x9);
                    syscall(6, (uint32_t)trimmed_current_path, strlen(trimmed_current_path), 0x9);
                    syscall(6, (uint32_t)cl.table[i].name, j, 0x9);
                    if(cl.table[i].attribute != ATTR_SUBDIRECTORY){
                        uint8_t k;
                        for(k = 0; k < 3 && cl.table[i].ext[k] != '\0'; k++){}
                        if(k>0)syscall(5,(uint32_t)&dot,0x9,0);
                        syscall(6, (uint32_t)cl.table[i].ext, k, 0x9);
                    }
                    syscall(5,(uint32_t)&slashN,0x9,0);
                    }
                char path[MAX_CMD_LENGTH];
                strcat(path,local_current_path);
                strcat(path,cl.table[i].name);
                strcat(path,"/");
                findHelper(target,prev,path,local_working_directory,cl);
                memset(path,0,MAX_CMD_LENGTH);
            }
        }
    }
}

void find(){
        struct FAT32DirectoryTable      cl   = {0};
        char local_current_path[128];
        memset(local_current_path,0,128);
        strcpy(local_current_path,current_path);

        
        char buf[MAX_CMD_LENGTH];
        memset(buf,0,MAX_CMD_LENGTH);
        if(strlen(cmd_buffer)>5){
            memcpy(buf, (void*)cmd_buffer+5, cur_cmd_length-5);
        }
        int current_path_length = strlen(local_current_path); 
        findHelper(buf,current_path_length,local_current_path,working_directory,cl);
}

void clear(){
    syscall(69,0,0,0);
}

void testing(){
    syscall(666,0,0,0);
}

void execute()
{
    char command[MAX_CMD_LENGTH];
    int cmd_length = 0;
    memset(command, 0, MAX_CMD_LENGTH);
    for (int i = 0; i < cur_cmd_length; i++)
    {
        if (cmd_buffer[i] == ' ')
            break;
        cmd_length++;
    }
    memcpy(command, cmd_buffer, cmd_length);
    if (cmd_length == 2 && memcmp(command, "cd", 2) == 0)
    {
        cd();
    }
    else if (cmd_length == 2 && memcmp(command, "ls", 2) == 0)
    {
        ls();
    }
    else if (cmd_length == 5 && memcmp(command, "mkdir", 5) == 0)
    {
        mkdir();
    }
    else if (cmd_length == 3 && memcmp(command, "cat", 3) == 0)
    {
        cat();
    }
    else if (cmd_length == 2 && memcmp(command, "cp", 2) == 0)
    {
        cpCaller();
    }
    else if (cmd_length == 2 && memcmp(command, "rm", 2) == 0)
    {
        rmCaller();
    }
    else if (cmd_length == 2 && memcmp(command, "mv", 2) == 0)
    {
        mv();
    }
    else if (cmd_length == 4 && memcmp(command, "find", 4) == 0)
    {
        find();
    }else if(cmd_length == 5 && memcmp(command, "clear", 5) == 0){
        clear();
    }else if(cmd_length == 4 && memcmp(command, "exec", 4) == 0){
        exec();
    }else if(cmd_length == 2 && memcmp(command, "ps", 2) == 0){
        ps();
    }else if(cmd_length == 4 && memcmp(command, "kill", 4) == 0){
        kill();
    }
    else if(cmd_length == 3 && memcmp(command, "wow", 3) == 0){
        testing();
    }else{

    }
    memset(cmd_buffer, 0, MAX_CMD_LENGTH);
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
    syscall(5, (uint32_t)&buf, 0xF, 0);
}


void template_print()
{
    syscall(6, (uint32_t) "Binbows@IF-2230", 15, 0xB);
    syscall(6, (uint32_t) ":", 1, 0x7);
    syscall(6, (uint32_t) current_path, (uint32_t) strlen(current_path), 0xD);
    syscall(6, (uint32_t)"$", 1, 0x7);
    syscall(6, (uint32_t)" ", 1, 0xF);
    uint8_t x = strlen(current_path) + 18;
    syscall(19, (uint32_t)&x, 0, 0);
}

int main(void) {
    // dummy file for testing
    // struct ClusterBuffer      cl   = {0};
    // char *owo = "KAAAAAA\n";
    // memset(cl.buf, 0, CLUSTER_SIZE);
    // memcpy(cl.buf, owo, 9);
    // struct FAT32DriverRequest request = {
    //     .buf                   = &cl,
    //     .name                  = "kaa\0",
    //     .ext                   = "txt",
    //     .parent_cluster_number = ROOT_CLUSTER_NUMBER,
    //     .buffer_size           = sizeof(struct ClusterBuffer),
    // };
    // int8_t retcode;
    // syscall(2, (uint32_t) &request, (uint32_t) &retcode, 0);

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
                execute();
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
    if(strlen(path) == 1 && memcmp(path, ".", 1) == 0){
        memset(path, 0, 8);
        memcpy(path, dir_table.table[0].name, 8);
        return 0;
    }

    if (strlen(path) >= 2 && memcmp(path, "..", 2) == 0) {
        if (working_directory != ROOT_CLUSTER_NUMBER) {
            uint32_t parent_cluster_num = dir_table.table[1].cluster_high << 16 | dir_table.table[1].cluster_low;
            working_directory = parent_cluster_num;
            int n = strlen(current_path), i=n-1;
            for(; i > 4; i--){
                if(current_path[i] == '/' && i < n-2) break;
                current_path[i] = '\0';
            }
            memset(path, 0, 8);
            memcpy(path, dir_table.table[1].name, strlen(dir_table.table[1].name));
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

void exec() {
    char buf[MAX_CMD_LENGTH];
    memset(buf, 0, MAX_CMD_LENGTH);
    memcpy(buf, (void*)cmd_buffer+5, cur_cmd_length-5);
    
    char file[8];
    memset(file, 0, 8);
    int i = strlen(buf);
    for (; i>0; i--) {
        if (buf[i] == '/') break;
    }
    if (i != 0) i++;
    for (int j=0; j < 8 && buf[i] != '\0'; j++, i++) {
        file[j] = buf[i];
        buf[i] = '\0';
    }

    uint32_t save_directory = working_directory;
    char save_path[MAX_CMD_LENGTH];
    memset(save_path, 0, MAX_CMD_LENGTH);
    memcpy(save_path, current_path, MAX_CMD_LENGTH);

    // cd to intended src path
    uint8_t ret = multiple_cd(buf,strlen(buf));
    if (ret != 0) {
        memcpy(current_path, save_path, MAX_CMD_LENGTH);
        working_directory = save_directory;
        syscall(6, (uint32_t) "File not found", 15, 0xC);
        return; 
    }

    struct FAT32DriverRequest request = {
        .buf                   = (uint8_t*) 0,
        .name                  = "shell",
        .ext                   = "\0\0\0",
        .parent_cluster_number = working_directory,
        .buffer_size           = 0x100000,
    };
    memcpy(request.name, file, 8);

    memcpy(current_path, save_path, MAX_CMD_LENGTH);
    working_directory = save_directory;

    uint8_t retcode = 5;
    syscall(8, (uint32_t) &request, (uint32_t) &retcode, 0);

    switch (retcode)
    {
    case 0:
        syscall(6, (uint32_t) "Exec success", 12, 0xA);
        break;
    case 1:
        syscall(6, (uint32_t) "Max process exceeded", 21, 0xC);
        break;
    case 2:
        syscall(6, (uint32_t) "Invalid entrypoint", 19, 0xC);
        break;
    case 3:
        syscall(6, (uint32_t) "Not enough memory", 18, 0xC);
        break;
    case 4:
        syscall(6, (uint32_t) "Read failure", 13, 0xC);
        break;
    }
}

void ps() {
    syscall(9, 0, 0, 0);
}

void kill() {
    char buf[MAX_CMD_LENGTH];
    memset(buf, 0, MAX_CMD_LENGTH);
    memcpy(buf, (void*)cmd_buffer+5, cur_cmd_length-5);
    
    char pid[2];
    memset(pid,0,2);
    memcpy(pid,buf,2);
    
    uint8_t retcode;
    syscall(11, (uint32_t) pid, (uint32_t) &retcode, 0);

    if (retcode == 0) {
        syscall(6, (uint32_t) "Kill success", 12, 0xA);
    } else {
        syscall(6, (uint32_t) "Kill failed", 12, 0xC);
    }
}