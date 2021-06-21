#ifndef _DUMMY_MODULE_H
#define _DUMMY_MODULE_H

#include <linux/semaphore.h>
#include <linux/ioctl.h>
#include "dummy_module_ioctl.h"
#include "linux/types.h"
#include "linux/wait.h"


struct dummy_module_pipe_struct {
	//wait queue for readers
	wait_queue_head_t rq, wq;
        //device write semaphore
        struct semaphore sem;
	//buffer
	char buffer[DUMMY_MODULE_PIPE_BUFFER_SIZE];
	//start and end index
	//buffer empty when start == end, full when end = start - 1
	//hence start: first index to read, end: first index to write 
	uint32_t start, end;
        //dev_t value for specific device
        dev_t device_major_minor;
};
struct dummy_module_device_struct {
        //device write semaphore
        struct semaphore sem;
        //allocated memory size
        uint32_t alloc_memory;
        //data pointer
        uint8_t *data_pointer;
        //dev_t value for specific device
        dev_t device_major_minor;
};
struct dummy_module_device_struct* dummy_module_find_device_struct(struct inode*);
unsigned long long ** find_syscall_table(void);

#endif /* _DUMMY_MODULE_H */   
