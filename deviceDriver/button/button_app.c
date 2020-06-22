#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>

#define BUTTON_MAJOR_NUMBER 504
#define BUTTON_MINOR_NUMBER 100
#define BUTTON_DEV_PATH_NAME "button_dev"
#define BUTTON_MAGIC_NUMBER 'j'

#define BUTTON_GET_STATE _IOR(BUTTON_MAGIC_NUMBER, 0 , int)

int main(void){
	dev_t button_dev;
	int button_fd;
	
	volatile int button_state = 0;
	volatile int new_state = 0;
	volatile int count =0;

	unsigned int gpio_input = 20;
	
	button_dev = makedev(BUTTON_MAJOR_NUMBER, BUTTON_MINOR_NUMBER);
	if (mknod(BUTTON_DEV_PATH_NAME, S_IFCHR|0666, button_dev)<0){
		fprintf(stderr, "%d\n", errno);
	}
	
	button_fd = open(BUTTON_DEV_PATH_NAME, O_RDWR);
	if(button_fd < 0){
		printf("fail to open button_dev\n");
		return -1;
	}

	ioctl(button_fd, BUTTON_START, &gpio_input)

	while(count < 100){
		usleep(1000000);
		ioctl(button_fd, BUTTON_GET_STATE, &new_state);
		if(button_state==0 && new_state==1){
			printf("Pressed 1\n");
		}
		else if(button_state==1 && new_state==0){
			printf("Pressed 0\n");
		}
		button_state = new_state;
		count++;
	}
	
	close(button_fd);
	
	return 0;
}
