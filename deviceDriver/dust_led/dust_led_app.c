#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>

#define DUST_LED_MAJOR_NUMBER 510
#define DUST_LED_MINOR_NUMBER 100
#define DUST_LED_DEV_PATH_NAME "dust_led_dev"
#define DUST_LED_MAGIC_NUMBER 'j'

#define DUST_LED_START _IOW(DUST_LED_MAGIC_NUMBER, 0, unsigned int[3])
#define DUST_LED_CONTROL _IOW(DUST_LED_MAGIC_NUMBER, 1, int)

int main(void){
	dev_t dust_led_dev;
	int dust_led_fd;
	unsigned int gpio[3] = {16,19,18};
    int color;

	dust_led_dev = makedev(DUST_LED_MAJOR_NUMBER, DUST_LED_MINOR_NUMBER);
	if (mknod(DUST_LED_DEV_PATH_NAME, S_IFCHR|0666, dust_led_dev)<0){
		fprintf(stderr, "%d\n", errno);
	}
	
	dust_led_fd = open(DUST_LED_DEV_PATH_NAME, O_RDWR);
	if(dust_led_fd < 0){
		printf("fail to open dust_led_dev\n");
		return -1;
	}
	
	ioctl(dust_led_fd, DUST_LED_START, &gpio);

	while(1){
        printf("enter color to control (None=0/R=1/G=2/B=3/exit=-1): ");
		scanf("%d", &color);
        if(color == -1) break;
		ioctl(dust_led_fd, DUST_LED_CONTROL, &color);
	}
	
	close(dust_led_fd);
	
	return 0;
}
