#include "asm-generic/errno-base.h"
#include "asm-generic/fcntl.h"
#include "asm-generic/ioctl.h"
#include "asm/page_types.h"
#include "asm/unistd_64.h"
#include "dummy_module_ioctl.h"
#include "linux/errno.h"
#include "linux/printk.h"
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include "dummy_module.h"
#include "linux/semaphore.h"
#include "linux/spinlock.h"
#include "linux/wait.h"
#include <linux/completion.h>
#include <linux/proc_fs.h>
#include <linux/ioctl.h>
#include <linux/sched.h>
#include <linux/syscalls.h>
//device file operations
static int dummy_module_file_open (struct inode *, struct file *);
static int dummy_module_file_release (struct inode *, struct file *);
ssize_t dummy_module_file_read (struct file *, char __user *, size_t, loff_t *);
ssize_t dummy_module_file_write (struct file *, const char __user *, size_t, loff_t *);
//pipe operations 
static int dummy_module_pipe_open (struct inode *, struct file *);
static int dummy_module_pipe_release(struct inode *, struct file *);
ssize_t dummy_module_pipe_read (struct file *, char __user *, size_t, loff_t *);
ssize_t dummy_module_pipe_write (struct file *, const char __user *, size_t, loff_t *);
int dummy_module_pipe_freespace(struct dummy_module_pipe_struct*, struct file*);
uint32_t get_free_space(struct dummy_module_pipe_struct*);
__poll_t dummy_module_pipe_poll (struct file *, struct poll_table_struct *);
//procfs operations
static int dummy_module_procfs_open (struct inode *, struct file *);
static int dummy_module_procfs_release (struct inode *, struct file *);
ssize_t dummy_module_procfs_read (struct file *, char __user *, size_t, loff_t *);
ssize_t dummy_module_procfs_write (struct file *, const char __user *, size_t, loff_t *);
long dummy_module_procfs_unlocked_ioctl (struct file *, unsigned int, unsigned long);
//procfs string
static char *dummy_module_procfs_string = "Hello World! This is procfs filesystem example";

//procfs dir entry pointer
struct proc_dir_entry *prdirenp;
static struct file_operations fops =  {
	.open = dummy_module_file_open,
	.release = dummy_module_file_release,
	.write = dummy_module_file_write,
	.read = dummy_module_file_read,
};
static struct proc_ops proc_fops =  {
	.proc_open = dummy_module_procfs_open,
	.proc_release = dummy_module_procfs_release,
	.proc_write = dummy_module_procfs_write,
	.proc_read = dummy_module_procfs_read,
	.proc_ioctl = dummy_module_procfs_unlocked_ioctl
};

static struct file_operations pipe_fops =  {
	.open = dummy_module_pipe_open,
	.release = dummy_module_pipe_release,
	.write = dummy_module_pipe_write,
	.read = dummy_module_pipe_read,
        .poll = dummy_module_pipe_poll
};
static struct cdev cdevs, cpipes;
static dev_t dev;
static struct dummy_module_device_struct dummy_module_device_array[DUMMY_MODULE_NOF_DEVICES];
static struct dummy_module_pipe_struct dummy_module_pipe_array[DUMMY_MODULE_NOF_PIPES];
DECLARE_COMPLETION(comp);

static int __init  my_init(void) {
        int i;
	pr_info("Hello dummy_module\n");
	//allocate char device region
	if(alloc_chrdev_region(&dev, 0, DUMMY_MODULE_NOF_DEVICES + DUMMY_MODULE_NOF_PIPES, "dummy_module")) {
		pr_err("Error allocating chrdev region\n");
		return -1;
	}
        for(i = 0; i < DUMMY_MODULE_NOF_DEVICES; i++) {
                dummy_module_device_array[i].alloc_memory = 1024;
                dummy_module_device_array[i].data_pointer = kmalloc(1024, GFP_KERNEL);
                memset(dummy_module_device_array[i].data_pointer,'a',1024);
                sema_init(&(dummy_module_device_array[i].sem),1);
                dummy_module_device_array[i].device_major_minor = MKDEV(MAJOR(dev),i);
        }
	for(i = 0; i < DUMMY_MODULE_NOF_PIPES; i++) {
		dummy_module_pipe_array[i].start = 0;
		dummy_module_pipe_array[i].end = 0;
		dummy_module_pipe_array[i].device_major_minor = MKDEV(MAJOR(dev), i + DUMMY_MODULE_NOF_DEVICES);
		sema_init(&dummy_module_pipe_array[i].sem,1);
		init_waitqueue_head(&dummy_module_pipe_array[i].rq);
		init_waitqueue_head(&dummy_module_pipe_array[i].wq);
		
	}
	//create new device
	cdev_init(&cdevs, &fops);	
	if(cdev_add(&cdevs, dev, DUMMY_MODULE_NOF_DEVICES)) {
		pr_err("Error creating new char device\n");
	}
	cdev_init(&cpipes, &pipe_fops);
	if(cdev_add(&cpipes, dummy_module_pipe_array[0].device_major_minor, DUMMY_MODULE_NOF_PIPES)) {
		pr_err("Error creating new pipes\n");
	}
	//creating procfs entry
	prdirenp = proc_create("dummy_module", 0666, NULL, &proc_fops);
	pr_info("Correctly initialized dummy_module\n");
	return 0;
}

static void __exit  my_exit(void) {
	int i;
	cdev_del(&cdevs);
	unregister_chrdev_region(dev, DUMMY_MODULE_NOF_DEVICES);
	for(i = 0; i < DUMMY_MODULE_NOF_DEVICES; i++) {
		kfree(dummy_module_device_array[i].data_pointer);
	}
	proc_remove(prdirenp);
	pr_info("Goodbye dummy_module\n");
	return;
}


static int dummy_module_pipe_open (struct inode *inp, struct file *filp) {
	if(MINOR(inp->i_rdev) >= DUMMY_MODULE_NOF_DEVICES + DUMMY_MODULE_NOF_PIPES || 
			MINOR(inp->i_rdev) < DUMMY_MODULE_NOF_DEVICES) {
		pr_err("dummy module can't find dummy pipe struct\n");
		return -1;
	}
        filp->private_data = &dummy_module_pipe_array[MINOR(inp->i_rdev) - DUMMY_MODULE_NOF_DEVICES];
	pr_info("dummy_module opened pipe\n");
	return 0;
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

static int dummy_module_pipe_release (struct inode *inp, struct file *filp) {
	pr_info("dummy_module closed pipe\n");
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
	//wait_for_completion(&comp);
	return i;
}

ssize_t dummy_module_file_write (struct file *filp, const char __user *buffer, size_t size, loff_t *offset) {
        int available_space, i;
        unsigned long written;
        struct dummy_module_device_struct *p;
        p = filp->private_data;
        if(down_interruptible(&p->sem))
		return -ERESTARTSYS;
        available_space = p->alloc_memory - *offset;
        for(i = 0; i < available_space && i < size; i++) {
                written = copy_from_user(&(p->data_pointer[*offset + i]), &buffer[i], 1);
                if(written) {
                        pr_err("Error copy_from_user\n");
                        up(&p->sem);
                        return -1;
                }
        }
        *offset += i;
        pr_info("Written %d bytes\n",i);
        up(&p->sem);
	return i;
}
__poll_t dummy_module_pipe_poll (struct file *filp, struct poll_table_struct *ptsp) {
        __poll_t ret = 0;
        //retrieve device structure
        struct dummy_module_pipe_struct* dmpsp = (struct dummy_module_pipe_struct*) filp->private_data;
        //get semaphore
        down(&dmpsp->sem);
        if(dmpsp->start != dmpsp->end)
                ret |= POLLRDNORM | POLLIN;
        if(get_free_space(dmpsp))
                ret |= POLLOUT | POLLWRNORM;
        up(&dmpsp->sem);
        return ret;
}

ssize_t dummy_module_pipe_read (struct file *filp, char __user *buffer, size_t count, loff_t *off) {
	uint32_t read_count;
	struct dummy_module_pipe_struct *dmpsp = filp->private_data;
	pr_info("dummy_module pipe read\n");
	//try to get semaphore
	if(down_interruptible(&dmpsp->sem)) 
		//restart read if interrupted
		return -ERESTARTSYS;
	//got semaphore, check if data available
	pr_info("dummy_module got pipe semaphore\n");
	while(dmpsp->start == dmpsp->end) {
		pr_info("dummy_module pipe no data available\n");
		//release the semaphore
		up(&dmpsp->sem);
		//check for O_NONBLOCK flag
		if(filp->f_flags & O_NONBLOCK) 
			//repeat system call
			return -EAGAIN;
		//go to sleep, waiting for end != start event
		pr_info("dummy_module go to sleep\n");
		if(wait_event_interruptible(dmpsp->rq, (dmpsp->end != dmpsp->start)))
			return -ERESTARTSYS;
		//"thundering herd" is avoided as condition is checked again
		//after waking up, with semaphore protection
		//acquire again lock before checking condition and reading data
		if(down_interruptible(&dmpsp->sem))
			return -ERESTARTSYS;
	}
	//semaphore acquired, we can read
	if(dmpsp->end < dmpsp->start)
		read_count = dmpsp->start + DUMMY_MODULE_PIPE_BUFFER_SIZE - dmpsp->end - 1;
	else
		read_count =  dmpsp->end - dmpsp->start;
	//getting min between available data and requested data
	read_count = read_count < count ? read_count : count;
	//TODO: for now, avoid multiple copy_to_user, just read at max to buffer end
	if(read_count + dmpsp->start > DUMMY_MODULE_PIPE_BUFFER_SIZE )
		read_count = DUMMY_MODULE_PIPE_BUFFER_SIZE - dmpsp->start;
	if(copy_to_user(buffer,&dmpsp->buffer[dmpsp->start],read_count)) {
		up(&dmpsp->sem);
		return -EFAULT;
	}
	dmpsp->start = (dmpsp->start + read_count) % DUMMY_MODULE_PIPE_BUFFER_SIZE;
	//unlocking semaphore
	up(&dmpsp->sem);
	//signaling writer waiting for free space in buffer
	wake_up_interruptible(&dmpsp->wq);
	return read_count;
}

int dummy_module_pipe_freespace(struct dummy_module_pipe_struct* pipe_struct, struct file* filp) {
	//semaphore already taken
	while(get_free_space(pipe_struct) == 0) {
		DEFINE_WAIT(wait);
		//release semaphore
		up(&pipe_struct->sem);
		if(filp->f_flags & O_NONBLOCK)
			return -EAGAIN;
		prepare_to_wait(&pipe_struct->wq, &wait, TASK_INTERRUPTIBLE);
		//check if space has become available between this two lines
		if(get_free_space(pipe_struct) == 0)
			schedule();
		finish_wait(&pipe_struct->wq, &wait);
		if(signal_pending(current))
			return -ERESTARTSYS;
		if(down_interruptible(&pipe_struct->sem))
			return -ERESTARTSYS;
	}
	return 0;
}
ssize_t dummy_module_pipe_write (struct file *filp, const char __user *buffer, size_t count, loff_t *off) {

	uint32_t write_count, free_space_result;
	struct dummy_module_pipe_struct *dmpsp = filp->private_data;
	//try to get semaphore
	if(down_interruptible(&dmpsp->sem)) 
		//restart write if interrupted
		return -ERESTARTSYS;
	free_space_result = dummy_module_pipe_freespace(dmpsp, filp);
	if(free_space_result != 0)
		//error returned by dummy_module_pipe_freespace function
		//it also frees semaphore in this case
		return free_space_result;
	//semaphore acquired, we can write 
	write_count = get_free_space(dmpsp);
	write_count = write_count < count ? write_count : count;
	if(dmpsp->end < dmpsp->start)
		write_count = write_count <  dmpsp->start - dmpsp->end - 1 ? write_count : dmpsp->start - dmpsp->end - 1;
	else
		write_count =  write_count < DUMMY_MODULE_PIPE_BUFFER_SIZE - dmpsp->end ? write_count : DUMMY_MODULE_PIPE_BUFFER_SIZE - dmpsp->end;
	//TODO: for now, avoid multiple copy_to_user, just read at max to buffer end
	if(copy_from_user(&dmpsp->buffer[dmpsp->end],buffer,write_count)) {
		up(&dmpsp->sem);
		return -EFAULT;
	}
	dmpsp->end += write_count;
	if(dmpsp->end == DUMMY_MODULE_PIPE_BUFFER_SIZE)
		dmpsp->end = 0;
	//unlocking semaphore
	up(&dmpsp->sem);
	//signaling writer waiting for free space in buffer
	wake_up_interruptible(&dmpsp->rq);
	return write_count;
}
uint32_t get_free_space(struct dummy_module_pipe_struct *pipe_struct) {
	if(pipe_struct->start == pipe_struct->end)
		return DUMMY_MODULE_PIPE_BUFFER_SIZE - 1;
	return (pipe_struct->start - pipe_struct->end + DUMMY_MODULE_PIPE_BUFFER_SIZE) % DUMMY_MODULE_PIPE_BUFFER_SIZE - 1;
}
static int dummy_module_procfs_open (struct inode *inp, struct file *filp) {
        pr_info("/proc/dummy_module open\n");
	return 0;
}
static int dummy_module_procfs_release (struct inode *inp, struct file *filp) {
        pr_info("/proc/dummy_module close\n");
	return 0;
}
ssize_t dummy_module_procfs_read (struct file *filp, char __user *buffer, size_t size, loff_t *offset) {
	int i;
	unsigned long written;
        int limit;
        //residual memory
        limit = strlen(dummy_module_procfs_string) - *offset;
        for(i = 0; i < size && i < limit; i++) {
                written = copy_to_user(&buffer[i], &(dummy_module_procfs_string[*offset + i]), 1);
                if(written) {
                        pr_err("Error copy_to_user\n");
                        return -1;
                }
        }
	*offset += i;
	pr_info("Read %i bytes",i);
	//wait_for_completion(&comp);
	return i;
}
ssize_t dummy_module_procfs_write (struct file *filp, const char __user *buffer, size_t size, loff_t *offset) {
	return size;
}

long dummy_module_procfs_unlocked_ioctl (struct file *filp, unsigned int cmd, unsigned long arg) {
	//The Long Voyage
	int err = 0, pid;
        struct task_struct *found_proc;
        struct list_head *list_item_pointer;
	//check magic
	if(_IOC_TYPE(cmd) != DUMMY_MODULE_MAGIC) {
		pr_info("Wrong magic number\n");
		return -ENOTTY;
	}
	//check cmd number
	if(_IOC_NR(cmd) > DUMMY_MODULE_MAX_IO_CMD) {
		pr_info("Wrong command number\n");
		return -ENOTTY;
	}
	//check access
	if(_IOC_DIR(cmd) & _IOC_READ || _IOC_DIR(cmd) & _IOC_WRITE) {
		err = !access_ok((void __user *)arg, _IOC_SIZE(cmd));
        }
	if(err) {
                pr_info("Wrong user memory pointer\n"); 
                return -EFAULT;
        }
        pr_info("Correctly executed ioctl\n"); 
        //getting pid 
        copy_from_user((void*)&pid, (void*)arg, sizeof(int));
        pr_info("Pid %d\n",pid);
        //getting process info
        found_proc = pid_task(find_vpid(pid), PIDTYPE_PID);
        if(!found_proc) {
                pr_err("Process not found\n");
                return -EFAULT;
        }
        //write smthg into user arg
        list_for_each(list_item_pointer, &(found_proc->children)) {
                pr_info("Child process id %u\n", container_of(list_item_pointer, struct task_struct, children)->tgid);
        }
	return 0;
}
struct dummy_module_device_struct* dummy_module_find_device_struct(struct inode *inp) {
        int i;
        i = MINOR(inp->i_rdev);
        if(i >= DUMMY_MODULE_NOF_DEVICES)
                return NULL;
        return &dummy_module_device_array[i];
                
}
unsigned long long ** find_syscall_table() {
	unsigned long long** syscall_table_addr = NULL;
	unsigned long long i;
	for(i = __START_KERNEL; i < ULLONG_MAX; i = i + sizeof(void*)) {
		pr_info("Checking address %llu\n", i);
		syscall_table_addr = (unsigned long long**)i;
		if(syscall_table_addr[__NR_close] == (unsigned long long*)ksys_close) {
			pr_info("Syscall table found\n");
			return syscall_table_addr;	
		}
	}
	return NULL;
}
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Harlock");
MODULE_DESCRIPTION("Dummy module");

module_init(my_init);

