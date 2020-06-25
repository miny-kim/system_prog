#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>

#define LED_MAJOR_NUMBER 505
#define LED_MINOR_NUMBER 100
#define LED_DEV_PATH_NAME "led_dev"
#define LED_MAGIC_NUMBER 'j'

#define LED_START _IOW(LED_MAGIC_NUMBER, 0, unsigned int[3])
#define LED_CONTROL _IOW(LED_MAGIC_NUMBER, 1, int)

int main(void){
	dev_t led_dev;
	int led_fd;
	unsigned int gpio[3] = {17,27,13};
    int color;

	led_dev = makedev(LED_MAJOR_NUMBER, LED_MINOR_NUMBER);
	if (mknod(LED_DEV_PATH_NAME, S_IFCHR|0666, led_dev)<0){
		fprintf(stderr, "%d\n", errno);
	}
	
	led_fd = open(LED_DEV_PATH_NAME, O_RDWR);
	if(led_fd < 0){
		printf("fail to open led_dev\n");
		return -1;
	}
	
	ioctl(led_fd, LED_START, &gpio);

	while(1){
        printf("enter color to control (None=0/R=1/G=2/B=3/exit=-1): ");
		scanf("%d", &color);
        if(color == -1) break;
		ioctl(led_fd, LED_CONTROL, &color);
	}
	
	close(led_fd);
	
	return 0;
}
