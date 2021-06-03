#ifndef _DUMMY_MODULE_H
#define _DUMMY_MODULE_H

#include <linux/semaphore.h>
#include <linux/ioctl.h>

#define DUMMY_MODULE_NOF_DEVICES 5
#define DUMMY_MODULE_MAGIC 'j'

#define DUMMY_MODULE_GET_PR_INFO _IOWR(DUMMY_MODULE_MAGIC,0,int)
#define DUMMY_MODULE_MAX_IO_CMD 0

struct dummy_module_device_struct {
        //device write semaphore
        struct semaphore write_sem;
        //allocated memory size
        uint32_t alloc_memory;
        //data pointer
        uint8_t *data_pointer;
        //dev_t value for specific device
        dev_t device_major_minor;
};
struct dummy_module_device_struct* dummy_module_find_device_struct(struct inode*);

#endif /* _DUMMY_MODULE_H */   
