#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>

#define CO2_MAJOR_NUMBER 503
#define CO2_MINOR_NUMBER 101
#define CO2_DEV_PATH_NAME "/dev/co2"

#define INTERVAL 1000000  //1s

int main(void){
	
	 dev_t sensor_dev;
	 int fd_sensor;
	 float ppm = 0;
	 
	 sensor_dev = makedev(CO2_MAJOR_NUMBER, CO2_MINOR_NUMBER);
	 mknod(CO2_DEV_PATH_NAME, S_IFCHR|0666, sensor_dev);
	 
	 fd_sensor = open(CO2_DEV_PATH_NAME, O_RDWR);
	 
	 if(fd_sensor<0){
		 printf("fail to open\n");
		 return -1;
	 }
	 
	
	while(1){
		 printf("read...\n");
		read(fd_sensor, &ppm, sizeof(float));
		printf("%f ppm\n", ppm);
		usleep(INTERVAL);
	}
	
	close(fd_sensor);
	 
	 return 0;
 }
