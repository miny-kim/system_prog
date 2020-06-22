#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>

#define LED_MAJOR_NUMBER 502
#define LED_MINOR_NUMBER 100
#define LED_DEV_PATH_NAME "led_dev"
#define LED_MAGIC_NUMBER 'j'

#define LED_CONTROL _IOW(LED_MAGIC_NUMBER, 0, int)

int main(void){
	dev_t led_dev;
	int led_fd;

    int color;
    int on;

	led_dev = makedev(LED_MAJOR_NUMBER, LED_MINOR_NUMBER);
	if (mknod(LED_DEV_PATH_NAME, S_IFCHR|0666, led_dev)<0){
		fprintf(stderr, "%d\n", errno);
	}
	
	led_fd = open(LED_DEV_PATH_NAME, O_RDWR);
	if(led_fd < 0){
		printf("fail to open led_dev\n");
		return -1;
	}
	
	while(1){
        printf("enter color to control (R=0/G=1/B=2/exit=-1): ");
		scanf("%d", &color);
        if(color == -1) break;
		ioctl(led_fd, LED_CONTROL, &color);
	}
	
	close(led_fd);
	
	return 0;
}
