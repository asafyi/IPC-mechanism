#ifndef MESSAGE_SLOT_H
#define MESSAGE_SLOT_H

#include <linux/ioctl.h>
#define MAJOR_NUM 235
#define DEVICE_NAME "message_slot"
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned int)
#define BUF_LEN 128
#define SUCCESS 0
#define SLOTS_NUM 256
#endif