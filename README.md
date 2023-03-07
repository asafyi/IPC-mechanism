# IPC-mechanism
Inter-process Communication (IPC) provides a mechanism to exchange data and information across multiple processes.  
The project implement a kernel module that provides a new IPC mechanism, called a message slot. A message slot is a character device file through which processes communicate.  
A message slot device has multiple message channels active concurrently, which can be used by multiple processes. After opening a message slot device file, a process uses ioctl() to specify the id of the message channel it wants to use. It subsequently uses read()/write() to receive/send messages on the channel. 
In contrast to pipes, a message channel preserves a message until itis overwritten, so the same message can be read multiple times.
In additiod to the kernel module the project contain an interface (message_reader.c and message_sender.c) for the user to receive/send messages from the message slot.

##  Message slot kernel module
A message slot appears in the system as a character device file which is managed by the message slot device driver (message_slot.c). Every message slot device file has a major number and a minor number. The major number tells the kernel which driver is associated with the device file (in our case hard-coded to 235). The minor number is used internally by the driver for its own purposes, in this case for allowing the driver to distinguish between diffrent message slot files.

For each message slot file can be multiple message channels and every channel can save a diffrent message. The channels for every minor (diffrent message slot file) are stored using the built in [linux kernel structure - Red-Black Tree](https://github.com/torvalds/linux/blob/master/Documentation/core-api/rbtree.rst) which allow switching between channels efficiently.

