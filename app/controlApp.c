#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>

#define LCD_MAJOR_NUMBER	503
#define LCD_MINOR_NUMBER	100
#define LCD_DEV_PATH_NAME	"/dev/lcd_dev"
#define LCD_MAGIC_NUMBER	'j'
#define LCD_SLAVEADDR		0x27

#define LCD_START		_IOW(LCD_MAGIC_NUMBER, 0 , u_int8_t)
#define LCD_WRITE		_IOW(LCD_MAGIC_NUMBER, 1, struct write_data)
#define LCD_SET_LINE	_IOW(LCD_MAGIC_NUMBER, 2, int)
#define LCD_CLEAR		_IO(LCD_MAGIC_NUMBER, 3)

#define BUTTON_MAJOR_NUMBER 504
#define BUTTON_DEV_PATH_NAME "/dev/button_dev"
#define BUTTON_MAGIC_NUMBER 'j'

#define BUTTON_START		_IOW(BUTTON_MAGIC_NUMBER, 0, unsigned int)
#define BUTTON_GET_STATE	_IOR(BUTTON_MAGIC_NUMBER, 1 , int)

#define UART_MAJOR_NUMBER 502
#define UART_MINOR_NUMBER 100
#define UART_DEV_PATH_NAME "/dev/uart_ioctl"
#define IOCTL_MAGIC_NUMBER 'u'

#define STRING_LENGTH 128
#define LCD_LENGTH 16

#define IOCTL_CMD_TRANSMIT _IOWR(IOCTL_MAGIC_NUMBER, 0, int)
#define IOCTL_CMD_RECEIVE _IOWR(IOCTL_MAGIC_NUMBER, 1, int)
#define IOCTL_CMD_ARRIVED _IOR(IOCTL_MAGIC_NUMBER, 2, int)

#define LED_MAJOR_NUMBER 505
#define LED_DEV_NAME "led_dev"
#define LED_MAGIC_NUMBER 'j'

#define LED_START _IOW(LED_MAGIC_NUMBER, 0, unsigned int[3])
#define LED_CONTROL _IOW(LED_MAGIC_NUMBER, 1, int)

struct write_data{
	char* input;
	int len;
};

int main(void){
	dev_t uart_dev;
   	int uart_fd;
   	long temp;
   	char in[STRING_LENGTH];
	char co2_str[LCD_LENGTH];
	char dust_str[LCD_LENGTH];
	char test_input[STRING_LENGTH]= "C12D34E";
	int i = 0;
	char result;
	int state = -1;

   
   	uart_dev = makedev(UART_MAJOR_NUMBER, UART_MINOR_NUMBER);
   	if(mknod(UART_DEV_PATH_NAME, S_IFCHR|0666, uart_dev)<0){
		fprintf(stderr, "%d\n",errno);
	}
   
   	uart_fd = open(UART_DEV_PATH_NAME, O_RDWR | O_NOCTTY | O_NDELAY); //Open in non blocking read/write mode
   
   	if(uart_fd <0){
      	printf("fail to open\n");
    	return -1;
   	}

	//test
   	for(i = 0 ; i < strlen(test_input); i++){
        ioctl(uart_fd, IOCTL_CMD_TRANSMIT, &test_input[i]);
    	usleep(100);
    }
	//test end

   	while(1){
		temp = ioctl(uart_fd, IOCTL_CMD_RECEIVE, NULL);
      	if(temp >= 0){
			if(temp == 'C'){
				i=0;
				state = 0;
			}else if(temp == 'D'){
				i = 0;
				state = 1; 
			}else if(temp == 'E'){ //end of message
				state = -1;
				//ioctl(uart_fd, IOCTL_CMD_TRANSMIT, &result);
				break;
			}else{
				switch(state){
					case 0:
						co2_str[i++] = temp;
						break;
					case 1:
						dust_str[i++] = temp;
						break;
				}
			}
		}
	}
   
   close(uart_fd);
   return 0;
	// see if data arrives from uart
	// print data in l
	// calculate whether to open or close the window
	// send data with uart 
}
