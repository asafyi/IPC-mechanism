#include "message_slot.h"    

#include <fcntl.h>      
#include <unistd.h>     
#include <sys/ioctl.h>  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int main(int argc, char *argv[]){
    int fd;
    unsigned int channel_id;
    char buffer[BUF_LEN];
    int len;
    
    if(argc != 3){
        printf("Error: Invalid number of arguments: %s",strerror(EIO));
        exit(1);
    }
    fd = open(argv[1], O_RDONLY);
    if (fd < 0){ 
        perror("Error: open failed: ");
        exit(1);
    }
    channel_id = atoi(argv[2]); // ioctl will handle if 0 (not valid conversion)
    if(ioctl(fd, MSG_SLOT_CHANNEL, channel_id) != 0){
        perror("Error: ioctl failed: ");
        exit(1);
    }

    len = read(fd, buffer, BUF_LEN);
    if(len < 0){
        perror("Error: read failed: ");
        exit(1);
    }
    close(fd);
    if(write(STDOUT_FILENO, buffer, len) < 0){
        perror("Error: write to standard output failed: ");
        exit(1);
    }
    exit(0);
}