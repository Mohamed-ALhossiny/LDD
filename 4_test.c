#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "4_EEPROM_ioctl.h"

unsigned char seek = 0;
unsigned char tx[] = "Mohamed Ebrahim 123";
unsigned char rx[256];
int main(){
    int rec;
    int fd = open("/dev/My_EEPROM_Driver", O_RDWR);
    if(fd == -1){
        printf("Can't open the device file!\n");
        return 1;
    }
    printf("-------File is opened!-------\n");

    ioctl(fd, SET_SEEK, &seek);
    write(fd, tx, sizeof(tx));
    ioctl(fd, SET_SEEK, &seek);
    rec = read(fd, rx, sizeof(tx));

    for (size_t i = 0; i < rec; i++){
        printf("%c\n", rx[i]);
    }
    
    close(fd);
    printf("------File is closed!-------\n");
}