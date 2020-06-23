#include <stdio.h>
#include <stdlib.h>
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

#define BUTTON_MAJOR_NUMBER		504
#define BUTTON_DEV_PATH_NAME	"/dev/button_dev"
#define BUTTON_MAGIC_NUMBER		'j'

#define BUTTON_START		_IOW(BUTTON_MAGIC_NUMBER, 0, unsigned int)
#define BUTTON_GET_STATE	_IOR(BUTTON_MAGIC_NUMBER, 1 , int)

#define UART_MAJOR_NUMBER	502
#define UART_MINOR_NUMBER	100
#define UART_DEV_PATH_NAME	"/dev/uart_ioctl"
#define IOCTL_MAGIC_NUMBER	'u'

#define STRING_LENGTH	128
#define LCD_LENGTH		16

#define IOCTL_CMD_TRANSMIT	_IOWR(IOCTL_MAGIC_NUMBER, 0, int)
#define IOCTL_CMD_RECEIVE	_IOWR(IOCTL_MAGIC_NUMBER, 1, int)
#define IOCTL_CMD_ARRIVED	_IOR(IOCTL_MAGIC_NUMBER, 2, int)

#define LED_MAJOR_NUMBER		505
#define LED_DEV_PATH_NAME		"/dev/led_dev"
#define LED_MAGIC_NUMBER		'j'
#define LED_MINOR_NUMBER		100

#define LED_START	_IOW(LED_MAGIC_NUMBER, 0, unsigned int[3])
#define LED_CONTROL	_IOW(LED_MAGIC_NUMBER, 1, int)

#define BUTTON_MAJOR_NUMBER	504
#define BUTTON_DEV_NAME		"button_dev"
#define BUTTON_MAGIC_NUMBER	'j'
#define BUTTON_MINOR_NUMBER	100

#define BUTTON_START		_IOW(BUTTON_MAGIC_NUMBER, 0, unsigned int)
#define BUTTON_GET_STATE	_IOR(BUTTON_MAGIC_NUMBER, 1 , int)

#define CO2_GREAT	300
#define CO2_GOOD	400
#define CO2_BAD		500

#define AIR_GREAT	400
#define AIR_GOOD	500
#define AIR_BAD		700

#define CLOSE		0
#define OPEN		1
#define TOGGLE		2

int getCO2State(int co2_value){
	if(CO2_BAD < co2_value) return 1;
	if(CO2_GOOD < co2_value) return 2;
	return 3; 
}

int getDustState(int dust_value){
	if(AIR_BAD < dust_value) return 1;
	if(AIR_GOOD < dust_value) return 2;
	return 3; 
}

struct write_data{
	char input[LCD_LENGTH];
	int len;
};

int main(void){
	dev_t uart_dev;
   	int uart_fd;

	dev_t lcd_dev;
	int lcd_fd;

	dev_t led_dev;
	int led_fd;

	dev_t switch_dev;
	int switch_fd;

	u_int8_t slaveAddr = 0x27;
	unsigned int co2_gpio[3] = {17,27,22};
	unsigned int dust_gpio[3] = {16,19,21};
	unsigned int prio_gpio = 23;
	unsigned int on_gpio = 24;
	unsigned int button_gpio = 20;

   	long temp;
	char test_input[STRING_LENGTH]= "C12D34E";//for test delete later
	int i = 0;
	char result;
	char prev_result;
	int state = -1;
	struct write_data lcd_str;

	char co2_str[LCD_LENGTH];
	char dust_str[LCD_LENGTH];
	volatile int co2_value;
	volatile int dust_value;
	volatile int co2_state;
	volatile int dust_state;
	volatile int on_state;
	volatile int prio;
	volatile int button_state;
	volatile int prev_button_state;
   	uart_dev = makedev(UART_MAJOR_NUMBER, UART_MINOR_NUMBER);
   	if(mknod(UART_DEV_PATH_NAME, S_IFCHR|0666, uart_dev)<0){
		fprintf(stderr, "%d\n",errno);
	}
   
   	uart_fd = open(UART_DEV_PATH_NAME, O_RDWR | O_NOCTTY | O_NDELAY); //Open in non blocking read/write mode
   	if(uart_fd <0){
      	printf("fail to open\n");
    	return -1;
   	}

	lcd_dev = makedev(LCD_MAJOR_NUMBER, LCD_MINOR_NUMBER);
	if (mknod(LCD_DEV_PATH_NAME, S_IFCHR|0666, lcd_dev)<0){
		fprintf(stderr, "%d\n", errno);
	}
	
	lcd_fd = open(LCD_DEV_PATH_NAME, O_RDWR);
	if(lcd_fd < 0){
		printf("fail to open lcd_dev\n");
		return -1;
	}

	led_dev = makedev(LED_MAJOR_NUMBER, LED_MINOR_NUMBER);
	if (mknod(LED_DEV_PATH_NAME, S_IFCHR|0666, led_dev)<0){
		fprintf(stderr, "%d\n", errno);
	}
	
	led_fd = open(LED_DEV_PATH_NAME, O_RDWR);
	if(led_fd < 0){
		printf("fail to open led_dev\n");
		return -1;
	}

	switch_dev = makedev(BUTTON_MAJOR_NUMBER, BUTTON_MINOR_NUMBER);
	if (mknod(BUTTON_DEV_PATH_NAME, S_IFCHR|0666, switch_dev)<0){
		fprintf(stderr, "%d\n", errno);
	}
	
	switch_fd = open(BUTTON_DEV_PATH_NAME, O_RDWR);
	if(switch_fd < 0){
		printf("fail to open button_dev\n");
		return -1;
	}

	ioctl(lcd_fd, LCD_START, &slaveAddr);

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
				co2_str[i] = '\0';
				i = 0;
				state = 1; 
			}else if(temp == 'E'){ //end of message
				dust_str[i] = '\0';
				state = -1;
				ioctl(lcd_fd, LCD_CLEAR);
				sprintf(lcd_str.input, "CO2: %s", co2_str);
				lcd_str.len = strlen(lcd_str.input);
				printf("%s\n",lcd_str.input);
				ioctl(lcd_fd, LCD_WRITE, &lcd_str);
				usleep(1000000);
				ioctl(lcd_fd, LCD_SET_LINE, 1);
				usleep(1000000);
				sprintf(lcd_str.input, "Dust: %s", dust_str);
				lcd_str.len = strlen(lcd_str.input);
				printf("%s\n",lcd_str.input);
				ioctl(lcd_fd, LCD_WRITE, &lcd_str);
				usleep(1000000);

				co2_value = atoi(co2_str);
				dust_value = atoi(dust_str);
				co2_state = getCO2State(co2_value);
				dust_state = getDustState(dust_value);

				ioctl(led_fd, LED_START, &co2_gpio);
				ioctl(led_fd, LED_CONTROL, &co2_state);
				ioctl(led_fd, LED_START, &dust_gpio);
				ioctl(led_fd, LED_CONTROL, &dust_state);
				
				ioctl(switch_fd, BUTTON_START, &on_gpio);
				ioctl(switch_fd, BUTTON_GET_STATE, &on_state);

				if(on_state==1){
					if(co2_state > dust_state) result = OPEN;
					else if(co2_state < dust_state) result = CLOSE;
					else{
						ioctl(switch_fd, BUTTON_START, &prio_gpio);
						ioctl(switch_fd, BUTTON_GET_STATE, &prio);
						if(prio ==1) result = OPEN;
						else result = CLOSE;
					}
					if(prev_result != result){
						ioctl(uart_fd, IOCTL_CMD_TRANSMIT, &result);
						prev_result = result;
					}
					printf("result = %d", result);
				}
				usleep(100000000);
				break;//for test need to delete this
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
		ioctl(switch_fd, BUTTON_START, &on_gpio);
		ioctl(switch_fd, BUTTON_GET_STATE, &on_state);
		if(on_state == 0){
			ioctl(switch_fd, BUTTON_START, &button_gpio);
			ioctl(switch_fd, BUTTON_GET_STATE, &button_state);
			if(prev_button_state==0 && button_state==1){
				result = TOGGLE;
				printf("Toggle\n");
				//ioctl(uart_fd, IOCTL_CMD_TRANSMIT, &result);
			}
			prev_button_state = button_state;
		}
	}
   
   close(uart_fd);
   close(lcd_fd);
   close(led_fd);
   close(switch_fd);
   return 0;
}
