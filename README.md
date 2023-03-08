# IPC-mechanism
Inter-process Communication (IPC) provides a mechanism to exchange data and information across multiple processes.  
The project implement a kernel module that provides a new IPC mechanism, called a message slot. A message slot is a character device file through which processes communicate.  
A message slot device has multiple message channels active concurrently, which can be used by multiple processes. After opening a message slot device file, a process uses ioctl() to specify the id of the message channel it wants to use. It subsequently uses read()/write() to receive/send messages on the channel. 
In contrast to pipes, a message channel preserves a message until itis overwritten, so the same message can be read multiple times.
In additiod to the kernel module the project contain an interface (message_reader.c and message_sender.c) for the user to receive/send messages from the message slot.

##  Message slot kernel module
A message slot appears in the system as a character device file which is managed by the message slot device driver (message_slot.c). Every message slot device file has a major number and a minor number. The major number tells the kernel which driver is associated with the device file (in our case hard-coded to 235). The minor number is used internally by the driver for its own purposes, in this case for allowing the driver to distinguish between diffrent message slot files.

For each message slot file can be multiple message channels and every channel can save a diffrent message. The channels for every minor (diffrent message slot file) are stored using the built in [linux kernel structure - Red-Black Tree](https://github.com/torvalds/linux/blob/master/Documentation/core-api/rbtree.rst) which allow switching between channels efficiently.

The full details of the project are described in [project-description.pdf](project-description.pdf)

## Usage
### Loading the kernel module and creating device files
In order to use the IPC in linux, you should compile the module and load it to the kernel:
```bash
make
sudo insmod message_slot.ko
```
then create the device files:
```bash
sudo mknod /dev/<dir> c <major> <minor>
sudo chmod o+rw /dev/<dir>
# <dir> - device directory
# <major> - hard-coded to 235 (can be changed in the code).
# <minor> - number that will help distinguish between diffrent message slot files for the device.

# for example:
sudo mknod /dev/msgslot c 235 1
sudo chmod o+rw /dev/msgslot
```
### Unloading the kernel module and cleanig device files
In order to unload the module from the kernel and cleaning device files of the device:
```bash
sudo rmmod message_slot
sudo rm /dev/<dir>
# <dir> - device directory as defined in loading the kernel
```
### Interface for communicating with the module
#### Message Sender
```bash
gcc -O3 -Wall -std=c11 message_sender.c -o message_sender.o
./message_sender.o /dev/<dir> <channel> <msg>
# <dir> - device directory
# <channel> - the channel to which we want the message to be written.
# <message> - the message.
```

#### Message Reader
```bash
gcc -O3 -Wall -std=c11 message_reader.c -o message_reader.o
./message_reader.o /dev/<dir> <channel>
# <dir> - device directory
# <channel> - the channel through which the message should be read.
```
