#include "linux/printk.h"
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

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

static int __init  my_init(void) {
	pr_info("Hello dummy_module\n");
	//allocate char device region
	if(alloc_chrdev_region(&dev, 0, 1, "dummy_module")) {
		pr_err("Error allocating chrdev region\n");
		return -1;
	}
	//create new device
	cdev_init(&cdevs, &fops);	
	if(cdev_add(&cdevs, dev, 1)) {
		pr_err("Error creating new char device\n");
	}
	pr_info("Correctly initialized dummy_module\n");
	return 0;
}

static void __exit  my_exit(void) {
	cdev_del(&cdevs);
	unregister_chrdev_region(dev, 1);
	pr_info("Goodbye dummy_module\n");
	return;
}

static int dummy_module_file_open (struct inode *inp, struct file *filp) {
	pr_info("dummy_module opened device\n");
	return 0;
}
static int dummy_module_file_release (struct inode *inp, struct file *filp) {
	pr_info("dummy_module closed device\n");
	return 0;
}

ssize_t dummy_module_file_read (struct file *filp, char __user *buffer, size_t size, loff_t *offset) {
	int i;
	unsigned long written;
	for(i  = 0; i < size; i++) {
		if((written = copy_to_user(&buffer[i], "a", 1))) {
			pr_err("Error reading device");
			pr_err("Size %lu i %d written %lu", size, i, written);
			return -1;
		}
	}
	*offset += size;
	pr_info("Read %lu bytes",size);
	return size;
}

ssize_t dummy_module_file_write (struct file *filp, const char __user *buffer, size_t size, loff_t *offset) {
	return 0;
}
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Harlock");
MODULE_DESCRIPTION("Dummy module");

module_init(my_init);
module_exit(my_exit);
