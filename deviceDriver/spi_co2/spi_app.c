#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>

#define CO2_MINOR_NUMBER 101
#define CO2_MAJOR_NUMBER 504
#define CO2_DEV_PATH_NAME "/dev/try_co2"

#define INTERVAL 500000

//#define IOCTL_MAGIC_NUMBER 'u'
//#define IOCTL_CMD_TRANSMIT _IOWR(IOCTL_MAGIC_NUMBER, 0, int)
//#define IOCTL_CMD_RECEIVE _IOWR(IOCTL_MAGIC_NUMBER, 1, int)

int main()
{
    dev_t co2_dev;
    int fd, ppm;
    float temp, rs, ratio, co2_ppm;
    
    co2_dev = makedev(CO2_MAJOR_NUMBER, CO2_MINOR_NUMBER);
    mknod(CO2_DEV_PATH_NAME, S_IFCHR|0666, co2_dev);
    
    fd = open(CO2_DEV_PATH_NAME, O_RDWR | O_NOCTTY | O_NDELAY); //Open in non blocking read/write mode
    
    if(fd <0)
    {
        printf("fail to open\n");
        return -1;
    }
    
    while(1){
        printf("read...\n");
        read(fd, &ppm, sizeof(int));
        
        temp = ppm * 3.3 / 4095.0;
       
        if(temp <1)
            temp = 1;
        
        rs = 10 * (5.0 - temp) / temp;
        ratio = rs / 76.63;
        co2_ppm = (146.15*(2.868-ratio)+10);
        
        printf("Current CO2 ppm is:%f ppm\n", co2_ppm);
        usleep(INTERVAL);
    }
    
    
    close(fd);
    return 0;
}



