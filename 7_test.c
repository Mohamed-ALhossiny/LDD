#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include "7_macros.h"

void handler(int sig){
    printf("Botton pressed!\n");
}

int main(){
    signal(SWITCH_SIGNAL, handler);
    int fd = open("/dev/My_GPIO_Driver", O_RDWR);
    if(fd == -1){
        printf("Can't open the device file!\n");
        return 1;
    }
    printf("-------File is opened!-------\n");

    ioctl(fd, REGISTER_APP, NULL);
    while(1)
        sleep(1);
    
    close(fd);
    printf("------File is closed!-------\n");
}