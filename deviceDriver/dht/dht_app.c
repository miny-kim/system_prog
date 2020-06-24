#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>

#define DHT_MAJOR_NUMBER	506
#define DHT_MINOR_NUMBER    100
#define DHT_DEV_PATH_NAME	"dht_dev"
#define DHT_MAGIC_NUMBER	'j'

#define DHT_GET_HUMIDITY	_IOR(DHT_MAGIC_NUMBER, 0 , int)

int main(void){
	dev_t dht_dev;
	int dht_fd;

	unsigned int gpio_input = 12;
    int count = 0;
    int humidity;
	
	dht_dev = makedev(DHT_MAJOR_NUMBER, DHT_MINOR_NUMBER);
	if (mknod(DHT_DEV_PATH_NAME, S_IFCHR|0666, dht_dev)<0){
		fprintf(stderr, "%d\n", errno);
	}
	
	dht_fd = open(DHT_DEV_PATH_NAME, O_RDWR);
	if(dht_fd < 0){
		printf("fail to open dht_dev\n");
		return -1;
	}

	while(count < 3){
		usleep(1000);
		ioctl(dht_fd, DHT_GET_HUMIDITY, &humidity);
		printf("humidity : %d\n", humidity);
		count++;
	}
	
	close(dht_fd);
	
	return 0;
}
