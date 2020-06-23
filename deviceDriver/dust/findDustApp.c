#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>

#define DUST_MINOR_NUMBER 101
#define DUST_MAJOR_NUMBER 505
#define DUST_DEV_PATH_NAME "/dev/finedust"

#define INTERVAL1 9680000 // 3초
#define INTERVAL2  280000 //0.28초
#define INTERVAL3 1000000

//#define IOCTL_MAGIC_NUMBER 'u'
//#define IOCTL_CMD_TRANSMIT _IOWR(IOCTL_MAGIC_NUMBER, 0, int)
//#define IOCTL_CMD_RECEIVE _IOWR(IOCTL_MAGIC_NUMBER, 1, int)

int main()
{
   dev_t dust_dev;
   int fd, pm;
   float temp, dust_pm, no_dust = 0.35;
   

   dust_dev = makedev(DUST_MAJOR_NUMBER, DUST_MINOR_NUMBER);
   mknod(DUST_DEV_PATH_NAME, S_IFCHR|0666, dust_dev);
   
   fd = open(DUST_DEV_PATH_NAME, O_RDWR | O_NOCTTY | O_NDELAY); //Open in non blocking read/write mode

   
   
   if(fd <0)
   {
      printf("fail to open\n");
      return -1;
   }
   
	while(1){
		printf("read...\n");
		
        
        read(fd, &pm, sizeof(int));
		
	   temp = (float)pm * (5.0/1024.0);
      //dust_pm = (temp-no_dust) / 0.005;
      dust_pm = (0.17 * temp) *1000;
        //dust_pm = temp / 0.005;

        if(dust_pm < 0)
            dust_pm = 0;
		
        
		printf("Current DUST is:%f (ug/m3)\n", dust_pm);
		
        //usleep(INTERVAL1);
	}

   
   close(fd);
   return 0;
}


