#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>

#define BUTTON_MAJOR_NUMBER 504
#define BUTTON_DEV_NAME "button_dev"
#define BUTTON_MAGIC_NUMBER 'j'

#define BUTTON_START _IOW(BUTTON_MAGIC_NUMBER, 0 , unsigned int)
#define BUTTON_GET_STATE _IOR(BUTTON_MAGIC_NUMBER, 1 , int)

int main(void){
	dev_t button_dev;
	int button_fd;
	
	int button_state = 0;
	int new_state = 0;
	int count =0;
	
	button_dev = makedev(BUTTON_MAJOR_NUMBER, BUTTON_MINOR_NUMBER);
	if (mknod(BUTTON_DEV_PATH_NAME, S_IFCHR|0666, button_dev)<0){
		fprintf(stderr, "%d\n", errno);
	}
	
	button_fd = open(BUTTON_DEV_PATH_NAME, O_RDWR);
	if(button_fd < 0){
		printf("fail to open button_dev\n");
		return -1;
	}
	
	while(count < 1000){
		usleep(10000);
		ioctl(button_fd, BUTTON_GET_STATE, &new_state);
		if(button_state==0 && new_state==1){
			printf("Pressed\n");
		}
		button_state = new_state;
		count++;
	}
	
	close(button_fd);
	
	return 0;
}
