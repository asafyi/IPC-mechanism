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
    char* message = argv[3];
    int msg_len = strlen(message);

    if(argc != 4){
        printf("Error: Invalid number of arguments: %s",strerror(EIO)); 
        exit(1);
    }
    fd = open(argv[1], O_WRONLY);
    if (fd < 0){ 
        perror("Error: open failed: ");
        exit(1);
    }
    channel_id = atoi(argv[2]); // ioctl will handle if 0 (not valid conversion)
    
    if(ioctl(fd, MSG_SLOT_CHANNEL, channel_id) != 0){
        perror("Error: ioctl failed: ");
        exit(1);
    }
    if(write(fd, message, msg_len) != msg_len){
        perror("Error: write failed: ");
        exit(1);
    }
    close(fd);
    exit(0);
}