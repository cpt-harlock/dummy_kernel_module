#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include "dummy_module_ioctl.h"

int main(int argc, char *argv[]) {
	int fd,  ret;
        char user_mem[200] = {0};
        //passing process id inside char buffer
        ((int*)user_mem)[0] = atoi(argv[1]);
	fd = open("/proc/dummy_module", O_RDWR);
        ret = ioctl(fd, DUMMY_MODULE_GET_PR_INFO, user_mem);
        printf("returned %d\n", ret);
        printf("arg %s\n", user_mem);
        //ioctl(fd, DUMMY_MODULE_GET_PR_INFO);
}
