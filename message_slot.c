#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/rbtree.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL"); 

#include "message_slot.h"


static struct rb_root channels_trees[SLOTS_NUM] = {RB_ROOT}; // every cell keep rb_root for diffrent minor

typedef struct channel_node{
    struct rb_node node; // the node in the tree for this struct
    unsigned long key; // key is the channel number
    char buffer[BUF_LEN]; // the message 
    int buf_size; // size of the message in buffer
} channel_node;


typedef struct file_data{
    int minor; 
    unsigned long channel;
    channel_node* channel_pointer; // pointer to the struct that contain the message for this channel (for this minor)
} file_data;



//================== Red-Black Tree FUNCTIONS ===========================
// using rb_tree implementation of linux
// The functions based mainly on: https://github.com/torvalds/linux/blob/master/Documentation/core-api/rbtree.rst
// and also used the document: http://tele.sj.ifsc.edu.br/~msobral/prg2/kernel-ds.pdf


/*
searching a node in the tree of the given root with key equals to the given channel
returning tha containig strucat of this node or NULL if not exist
*/
channel_node* search_channel_node(struct rb_root *root, unsigned long channel){
  	struct rb_node *node = root->rb_node;
  	while (node) {
  		channel_node *data = container_of(node, channel_node, node);
		if (channel < data->key)
  			node = node->rb_left;
		else if (channel > data->key)
  			node = node->rb_right;
		else
  			return data;
	}
	return NULL;
}

/*
inserting a node in the tree of the given root with key equals to the given channel number in data sturct 
and insering the node of this struct
returning SUCCESS if inserted or -1 if already exist (we always search before insert so this shouldn't happen)
*/
int insert_channel_node(struct rb_root *root, channel_node *data){
  	struct rb_node **new = &(root->rb_node), *parent = NULL;
  	while (*new) {
  		channel_node *this = container_of(*new, channel_node, node);
		parent = *new;
  		if (data->key < this->key)
  			new = &((*new)->rb_left);
  		else if (data->key > this->key)
  			new = &((*new)->rb_right);
  		else
  			return -1;
  	}
  	// Add new node and rebalance tree
  	rb_link_node(&data->node, parent, new);
  	rb_insert_color(&data->node, root);
	return SUCCESS;
}

// the skelton of the functions is from rec6_kernal_2, folder CHARDEV2
//================== DEVICE FUNCTIONS ===========================

static int device_open(struct inode* inode, struct file* file){
    file_data* data;
    data = kmalloc(sizeof(file_data), GFP_KERNEL);
    if(data == NULL){
        printk("file_data kmalloc failed\n");
        return -ENOMEM; // "cann't allocate memory"
    }
    data->minor = iminor(inode);
    data->channel = 0;
    data->channel_pointer=NULL;
    file->private_data = (void*) data;
    printk("device_open succeded for minor %d\n",data->minor);
    return SUCCESS;
}


static long device_ioctl(struct file* file, unsigned int ioctl_command_id, unsigned long ioctl_param){
    struct rb_root *root;
    file_data *data;
    channel_node* channel_p;
    if((ioctl_command_id != MSG_SLOT_CHANNEL) || ioctl_param == 0){
        printk("device_ioctl failed for minor %d, channel %ld\n", data->minor, ioctl_param);
        return -EINVAL; // "invalid argument"
    }
    data = file->private_data;
    root = &channels_trees[data->minor];
    channel_p = search_channel_node(root,ioctl_param);
    if(channel_p == NULL){ // if the struct for this channel (for this minor) doesn't exist then creating it.
        printk("channel %ld wasn't found on minor %d, so creating it.\n", ioctl_param, data->minor);
        channel_p = kmalloc(sizeof(channel_node),GFP_KERNEL);
        if(channel_p == NULL){
            printk("channel_node kmalloc failed\n");
            return -ENOMEM; // "cann't allocate memory"
        } 
        channel_p->key = ioctl_param;
        (channel_p->node).rb_left = NULL;
        (channel_p->node).rb_right = NULL;
        channel_p->buf_size = 0;
        insert_channel_node(root, channel_p); 
    }
    data->channel = ioctl_param;
    data->channel_pointer = channel_p;
    printk("device_ioctl succeded for minor %d, channel %ld\n", data->minor, ioctl_param);
    return SUCCESS;
}


static ssize_t device_read(struct file* file, char __user* buffer, size_t length, loff_t* offset){
    ssize_t i;
    file_data *data;
    struct rb_root *root;
    channel_node* node;
    data = file->private_data;
    root = &channels_trees[data->minor];
    node = data->channel_pointer;
    if((data->channel == 0) || (buffer == NULL)){
        printk("device_read failed for minor %d, channel %ld -'EINVL'\n", data->minor, data->channel);
        return -EINVAL; // "invalid argument"
    }
    if((node == NULL) || (node->buf_size == 0)){
        printk("device_read failed for minor %d, channel %ld -'EWOULDBLOCK'\n", data->minor, data->channel);
        return -EWOULDBLOCK; // "Operation would block"
    }
    if(length < node->buf_size){
        printk("device_read failed for minor %d, channel %ld -'ENOSPC'\n", data->minor, data->channel);
        return -ENOSPC; // "No space left on device"
    }
    for(i = 0; i < node->buf_size; ++i){ //readining the buffer
        if(put_user((node->buffer)[i],&buffer[i]) != 0){
            printk("device_read failed for minor %d, channel %ld -'EFAULT'\n", data->minor, data->channel);
            return -EFAULT; // "Bad address" - same as put_user return when it fails
        }
    }
    printk("performing device_read for minor %d, channel %ld\n", data->minor, data->channel);
    return node->buf_size;
}

static ssize_t device_write(struct file* file, const char __user* buffer, size_t length, loff_t* offset){
    ssize_t i;
    file_data *data;
    struct rb_root *root;
    channel_node* node;
    data = file->private_data;
    root = &channels_trees[data->minor];
    node = data->channel_pointer;
    if((data->channel == 0) || (buffer == NULL) || (node == NULL)){
        printk("device_write failed for minor %d, channel %ld -'EINVL'\n", data->minor, data->channel);
        return -EINVAL; // "invalid argument"
    }
    if((length == 0) || (length > BUF_LEN)){
        printk("device_write failed for minor %d, channel %ld -'EMSGSIZE'\n", data->minor, data->channel);
        return -EMSGSIZE; // Message too long
    }
    for(i = 0; (i < length); ++i){ // wrriting to the buffer
        if(get_user((node->buffer)[i],&buffer[i]) != 0){
            printk("device_write failed for minor %d, channel %ld -'EFAULT'\n", data->minor, data->channel);
            node->buf_size = 0;
            return -EFAULT; // "Bad address" - same as get_user return when it fails
        }
    }
    printk("performing device_write for minor %d, channel %ld\n", data->minor, data->channel);
    node->buf_size = length;
    return length;
}


static int device_release(struct inode* inode, struct file* file){
    file_data *data = file->private_data;
    printk("performing device_release for channel %ld in minor %d", data->channel,data->minor);
    kfree(data);
    return SUCCESS;
}


//==================== DEVICE SETUP =============================
struct file_operations Fops = {
    .owner	        = THIS_MODULE, 
    .read           = device_read,
    .write          = device_write,
    .open           = device_open,
    .unlocked_ioctl = device_ioctl,
    .release        = device_release,
};

static int __init simple_init(void){
    int rc = -1;  
    rc = register_chrdev(MAJOR_NUM, DEVICE_NAME, &Fops);
    if(rc < 0){
        printk( KERN_ERR "%s registraion failed for  %d\n",DEVICE_NAME, MAJOR_NUM );
        return rc;
    }
    printk("Registeration is successful. Major number: %d\n",MAJOR_NUM);
    return SUCCESS;
}


static void __exit simple_cleanup(void){
    int i;
    printk("performing cleanup\n");
    unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
    for(i = 0; i < SLOTS_NUM; ++i){ // free all the allocations for the rb_node continers (channel_node) in every minor
        struct rb_node *node;
        struct rb_node *next;
        struct rb_root *root = &channels_trees[i];
        for (node = rb_first(root); node; node = next){ // iteating over the tree 
        //and free every channel_node struct which contains a node
            channel_node* data;
            next = rb_next(node);
            data = rb_entry(node, channel_node, node);
            kfree(data);
        }
        channels_trees[i]=RB_ROOT;
    }
}

module_init(simple_init);
module_exit(simple_cleanup);

