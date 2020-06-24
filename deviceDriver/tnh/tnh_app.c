#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>

#define TNH_MAJOR_NUMBER	507
#define TNH_MINOR_NUMBER	100
#define TNH_DEV_PATH_NAME	"/dev/tnh_dev"
#define TNH_MAGIC_NUMBER	'j'

#define TNH_READ_HUMIDITY   _IOR(TNH_MAGIC_NUMBER, 1, int*)

int main(void){

	dev_t tnh_dev;
	int tnh_fd;
	
	tnh_dev = makedev(TNH_MAJOR_NUMBER, TNH_MINOR_NUMBER);
	if (mknod(TNH_DEV_PATH_NAME, S_IFCHR|0666, tnh_dev)<0){
		fprintf(stderr, "%d\n", errno);
	}
	
	tnh_fd = open(TNH_DEV_PATH_NAME, O_RDWR);
	if(tnh_fd < 0){
		printf("fail to open tnh_dev\n");
		return -1;
	}

	printf("opened TNH\n");
	
	//currently testing

	int buf;
	
	ioctl(tnh_fd, TNH_READ_HUMIDITY, &buf);
	printf("Humidity : %.2f\n", (double)buf/10);
	
	close(tnh_fd);
	
	return 0;
}