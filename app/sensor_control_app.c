#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>


#define STRING_LENGTH 128

#define DUST_MINOR_NUMBER 102
#define DUST_MAJOR_NUMBER 505
#define DUST_DEV_PATH_NAME "/dev/finedust"

#define CO2_MINOR_NUMBER 101
#define CO2_MAJOR_NUMBER 504
#define CO2_DEV_PATH_NAME "/dev/try_co2"

#define UART_MINOR_NUMBER 100
#define UART_MAJOR_NUMBER 502
#define UART_DEV_NAME "uart_ioctl"
#define UART_DEV_PATH_NAME "/dev/uart_ioctl"

#define IOCTL_MAGIC_NUMBER 'u'
#define IOCTL_CMD_TRANSMIT _IOWR(IOCTL_MAGIC_NUMBER, 0, int)

#define TNH_MAJOR_NUMBER	507
#define TNH_MINOR_NUMBER	103
#define TNH_DEV_PATH_NAME	"/dev/tnh_dev"
#define TNH_MAGIC_NUMBER	'j'

#define TNH_READ_HUMIDITY   _IOR(TNH_MAGIC_NUMBER, 1, int*)

#define INTERVAL 15000000 //15초

dev_t co2_dev, uart_dev, dust_dev, tnh_dev;
int co2_fd, uart_fd, fineDust_fd, tnh_fd;

int init(){
   //co2
   co2_dev = makedev(CO2_MAJOR_NUMBER, CO2_MINOR_NUMBER);
   mknod(CO2_DEV_PATH_NAME, S_IFCHR|0666, co2_dev);
   
   co2_fd = open(CO2_DEV_PATH_NAME, O_RDWR | O_NOCTTY | O_NDELAY); //Open in non blocking read/write mode

   //dust
   dust_dev = makedev(DUST_MAJOR_NUMBER, DUST_MINOR_NUMBER);
   mknod(DUST_DEV_PATH_NAME, S_IFCHR|0666, dust_dev);
   
   fineDust_fd = open(DUST_DEV_PATH_NAME, O_RDWR | O_NOCTTY | O_NDELAY); //Open in non blocking read/write mode
   
   //uart
   uart_dev = makedev(UART_MAJOR_NUMBER, UART_MINOR_NUMBER);
   mknod(UART_DEV_PATH_NAME, S_IFCHR|0666, uart_dev);
   
   uart_fd = open(UART_DEV_PATH_NAME, O_RDWR | O_NOCTTY | O_NDELAY); //Open in non blocking read/write mode

   //tnh
   tnh_dev = makedev(TNH_MAJOR_NUMBER, TNH_MINOR_NUMBER);
   mknod(TNH_DEV_PATH_NAME, S_IFCHR|0666, tnh_dev);

	tnh_fd = open(TNH_DEV_PATH_NAME, O_RDWR);

   if((co2_fd || uart_fd || fineDust_fd || tnh_fd) <0)
   {
      printf("fail to open\n");
      return -1;
   }

}

int main()
{
   int ppm, string_index = 0, pm, i, buf, num = 0, sum = 0;
   float temp, rs, ratio, co2_ppm, para = 116.6020682, parab = 2.769034857, rzero = 76.63, rload = 5, dust_pm, pre_dust_pm;
   char string[STRING_LENGTH];
 
   init();
   
	while(1){
		printf("read...\n");
      usleep(INTERVAL);
      //co2
		read(co2_fd, &ppm, sizeof(int));
		
		temp = (float)ppm * 3.3 / 1024.0;
        
      if(temp <1)
		 	temp = 1;
		
      rs = rload * (5.0 - temp) / temp;
		ratio = rs / rzero;
	   co2_ppm = (146.15*(2.868-ratio)+10);
            
		printf("Current CO2 ppm is:%f ppm\n", co2_ppm);


      //dust

        for(i = 0; i<5; i++)
        {
           read(fineDust_fd, &pm, sizeof(int));
           if(pm == 0)
               num++;
           sum += pm;
        }
		
      if(num == 5)
         num = 1;
      sum /= (5-num);
	   temp = (float)sum * (5.0/1024.0);

      dust_pm = (0.17 * temp) *1000;
      pre_dust_pm = dust_pm;

      if(dust_pm < 5.00)
         dust_pm = pre_dust_pm;

      printf("Current DUST is:%f (ug/m3)\n", dust_pm);

      sum = 0;
      num = 0;

      printf("opened TNH\n");

	   ioctl(tnh_fd, TNH_READ_HUMIDITY, &buf);
	   printf("Humidity : %.2f\n", (double)buf/10);


      //send uart
      if(((double)buf/10 > 90.00))
      {
         sprintf(string, "C%.2fD%.2fT", co2_ppm, dust_pm);
      }
      else
      {
         sprintf(string, "C%.2fD%.2fF", co2_ppm, dust_pm);
      }
      
      for(i = 0 ; i < strlen(string); i++){
        ioctl(uart_fd, IOCTL_CMD_TRANSMIT, &string[i]);
       usleep(100);
    }
    
		usleep(INTERVAL);
	}

   
   close(co2_fd);
   close(fineDust_fd);
   close(uart_fd);
   close(tnh_fd);

   return 0;
}


