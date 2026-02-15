#include <stdio.h>
#include <unistd.h>



int main(void){
    if(getuid() != 0){
        fprintf(stderr, "[ERROR]: Please run this program with super user previleges.\n");
        return 1;
    }
    int timer_value = -1;
    char file_path[255];

    if(access(KERNEL_LOADED_PATH, F_OK) == 0){
        fprintf(stderr, "[ERROR]: Kernel module is loaded now! You cant change slko parmameters now.\n");
        return 1;
    }

    fprintf(stderr, "[SLKO_SETTINGS]: Enter timer value in milliseconds: ");
    scanf("%d", &timer_value);
    fprintf(stderr, "[SLKO_SETTINGS]: Enter file name for slko.ko logging: ");
    scanf("%254s", file_path);
    FILE *slko_config_fd = fopen(SLKO_CONFIG_PATH, "w");// defined in makefile via -DNAME=VALUE inside compile command
    if(slko_config_fd == NULL){
        fprintf(stderr, "[ERROR]: Failed to open config file.\n");
        return 1;
    }
    fprintf(slko_config_fd, "module_timer = %d\nfile_name = %s\n", timer_value, file_path);
    fclose(slko_config_fd);
    return 0;
}