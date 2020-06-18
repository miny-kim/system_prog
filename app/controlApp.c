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

#define TNH_MAJOR_NUMBER	504
#define TNH_MINOR_NUMBER	100
#define TNH_DEV_PATH_NAME	"/dev/tnh_dev"
#define TNH_MAGIC_NUMBER	'j'
#define TNH_SLAVEADDR		0xB8

#define LCD_START		_IOW(LCD_MAGIC_NUMBER, 0 , u_int8_t)
#define LCD_WRITE		_IOW(LCD_MAGIC_NUMBER, 1, struct write_data)
#define LCD_SET_LINE	_IOW(LCD_MAGIC_NUMBER, 2, int)
#define LCD_CLEAR		_IO(LCD_MAGIC_NUMBER, 3)

struct write_data{
	char* input;
	int len;
};

int main(void){
	dev_t lcd_dev;
	int lcd_fd;
	
	lcd_dev = makedev(LCD_MAJOR_NUMBER, LCD_MINOR_NUMBER);
	if (mknod(LCD_DEV_PATH_NAME, S_IFCHR|0666, lcd_dev)<0){
		fprintf(stderr, "%d\n", errno);
	}
	
	lcd_fd = open(LCD_DEV_PATH_NAME, O_RDWR);
	if(lcd_fd < 0){
		printf("fail to open lcd_dev\n");
		return -1;
	}

	printf("opened LCD\n");
	
	//currently testing

	struct write_data test;
	u_int8_t lcdSlaveAddr = LCD_SLAVEADDR;
	test.input = "Hello World!";
	test.len = strlen(test.input);
	ioctl(lcd_fd, LCD_START, &lcdSlaveAddr);
	printf("set slave address\n");
	ioctl(lcd_fd, LCD_WRITE, &test);
	printf("does it work?\n");
	usleep(5000000); //5 sec
	ioctl(lcd_fd, LCD_CLEAR);
	
	close(lcd_fd);
	
	return 0;
}
