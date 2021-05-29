#include "linux/printk.h"
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include "dummy_module.h"

static int dummy_module_file_open (struct inode *, struct file *);
static int dummy_module_file_release (struct inode *, struct file *);
ssize_t dummy_module_file_read (struct file *, char __user *, size_t, loff_t *);
ssize_t dummy_module_file_write (struct file *, const char __user *, size_t, loff_t *);

static struct file_operations fops =  {
	.open = dummy_module_file_open,
	.release = dummy_module_file_release,
	.write = dummy_module_file_write,
	.read = dummy_module_file_read
};
static struct cdev cdevs;
static dev_t dev;
static struct dummy_module_device_struct dummy_module_device_array[DUMMY_MODULE_NOF_DEVICES];

static int __init  my_init(void) {
        int i;
	pr_info("Hello dummy_module\n");
	//allocate char device region
	if(alloc_chrdev_region(&dev, 0, DUMMY_MODULE_NOF_DEVICES, "dummy_module")) {
		pr_err("Error allocating chrdev region\n");
		return -1;
	}
	//create new device
	cdev_init(&cdevs, &fops);	
	if(cdev_add(&cdevs, dev, DUMMY_MODULE_NOF_DEVICES)) {
		pr_err("Error creating new char device\n");
	}
        for(i = 0; i < DUMMY_MODULE_NOF_DEVICES; i++) {
                dummy_module_device_array[i].alloc_memory = 1024;
                dummy_module_device_array[i].data_pointer = kmalloc(1024, GFP_KERNEL);
                memset(dummy_module_device_array[i].data_pointer,'a',1024);
                sema_init(&(dummy_module_device_array[i].write_sem),1);
                dummy_module_device_array[i].device_major_minor = MKDEV(MAJOR(dev),i);
        }
	pr_info("Correctly initialized dummy_module\n");
	return 0;
}

static void __exit  my_exit(void) {
	cdev_del(&cdevs);
	unregister_chrdev_region(dev, DUMMY_MODULE_NOF_DEVICES);
	pr_info("Goodbye dummy_module\n");
	return;
}

static int dummy_module_file_open (struct inode *inp, struct file *filp) {
        struct dummy_module_device_struct *p;
	pr_info("dummy_module opened device\n");
        p = dummy_module_find_device_struct(inp);
        if(!p) {
                pr_err("dummy_module can't find dummy device struct\n");
                return -1;
        }
        filp->private_data = p;
	return 0;
}
static int dummy_module_file_release (struct inode *inp, struct file *filp) {
	pr_info("dummy_module closed device\n");
	return 0;
}

ssize_t dummy_module_file_read (struct file *filp, char __user *buffer, size_t size, loff_t *offset) {
	int i;
	unsigned long written;
        struct dummy_module_device_struct *p;
        int limit;
        p = filp->private_data;
        //residual memory
        limit = p->alloc_memory - *offset;
        for(i = 0; i < size && i < limit; i++) {
                written = copy_to_user(&buffer[i], &(p->data_pointer[*offset + i]), 1);
                if(written) {
                        pr_err("Error copy_to_user\n");
                        return -1;
                }
        }
	*offset += i;
	pr_info("Read %i bytes",i);
	return i;
}

ssize_t dummy_module_file_write (struct file *filp, const char __user *buffer, size_t size, loff_t *offset) {
        int available_space, i;
        unsigned long written;
        struct dummy_module_device_struct *p;
        p = filp->private_data;
        down(&p->write_sem);
        available_space = p->alloc_memory - *offset;
        for(i = 0; i < available_space && i < size; i++) {
                written = copy_from_user(&(p->data_pointer[*offset + i]), &buffer[i], 1);
                if(written) {
                        pr_err("Error copy_from_user\n");
                        up(&p->write_sem);
                        return -1;
                }
        }
        *offset += i;
        pr_info("Written %d bytes\n",i);
        up(&p->write_sem);
	return i;
}


struct dummy_module_device_struct* dummy_module_find_device_struct(struct inode *inp) {
        int i;
        i = MINOR(inp->i_rdev);
        if(i >= DUMMY_MODULE_NOF_DEVICES)
                return NULL;
        return &dummy_module_device_array[i];
                
}
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Harlock");
MODULE_DESCRIPTION("Dummy module");

module_init(my_init);
module_exit(my_exit);
