#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#include <asm/mach/map.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#include "../i2c/i2c.h"

#define LCD_MAJOR_NUMBER	503
#define LCD_DEV_NAME		"lcd_dev"
#define LCD_MAGIC_NUMBER	'j'

#define LCD_CLEARDISPLAY	0x01
#define LCD_RETURNHOME		0x02
#define LCD_ENTRYMODESET	0x04
#define LCD_DISPLAYCONTROL	0x08
#define LCD_CURSORSHIFT		0x10
#define LCD_FUNCTIONSET		0x20
#define LCD_SETCGRAMADDR	0x40
#define LCD_SETDDRAMADDR	0x80

// entry mode set
#define LCD_ENTRYRIGHT			0x00
#define LCD_ENTRYLEFT			0x02
#define LCD_ENTRYSHIFTINCREMENT	0x01
#define LCD_ENTRYSHIFTDECREMENT	0x00

// display control
#define LCD_DISPLAYON	0x04
#define LCD_DISPLAYOFF	0x00
#define LCD_CURSORON	0x02
#define LCD_CURSOROFF	0x00
#define LCD_BLINKON		0x01
#define LCD_BLINKOFF	0x00

// cursor shift
#define LCD_DISPLAYMOVE	0x08
#define LCD_CURSORMOVE	0x00
#define LCD_MOVERIGHT	0x04
#define LCD_MOVELEFT	0x00

// function set
#define LCD_8BITMODE	0x10
#define LCD_4BITMODE	0x00
#define LCD_2LINE		0x08
#define LCD_1LINE		0x00
#define LCD_5x10DOTS	0x04
#define LCD_5x8DOTS		0x00

// backlight
#define LCD_BACKLIGHT	0x08
#define LCD_NOBACKLIGHT	0x00

#define En	0b00000100  // Enable bit
#define Rw	0b00000010  // Read/Write bit
#define Rs	0b00000001  // Register select bit

struct write_data{
	char* input;
	int len;
}

#define I2C_SET_SLAVE	_IOW(LCD_MAGIC_NUMBER, 0 , u_int8_t)
#define LCD_WRITE		_IOW(LCD_MAGIC_NUMBER, 1, struct write_data)
#define LCD_SET_LINE	_IOW(LCD_MAGIC_NUMBER, 2, int)
#define LCD_CLEAR		_IO(LCD_MAGIC_NUMBER, 3)

u_int8_t slaveAddr = -1;

int lcd_open(struct inode *inode, struct file *filp){
	printk(KERN_ALERT "LCD DD -  Open\n");
	i2c_open();
	return 0;
}

int lcd_release(struct inode *inode, struct file *filp){
	printk(KERN_ALERT "LCD DD -  Release\n");
	i2c_release();
	return 0;
}

int lcd_send_4bits(u_int8_t value){
	i2c_write(value, 1);
	i2c_write(value | En, 1);
	usleep(1000);
	i2c_write(value & ~En, 1);
	usleep(50000);
	return 0;
}

int lcd_send(u_int8_t value, u_int8_t mode){
	u_int8_t high = value & 0xf0;
	u_int8_t low = (value<<4) &0xf0;
	lcd_send_4bits(high | mode);
	lcd_send_4bits(low | mode);
	return 0;
}

int lcd_write(char *str, int len){
	printk(KERN_INFO "LCD DD - Write\n");
	char* c = str;
	for(int i =0; i<len; i++){
		lcd_send((u_int8_t)c[i], Rs);
	}
	return 0;
}

int lcd_set_line(int line){ //line = 0 : first line, line = 1 : second line
	printk(KERN_INFO "LCD DD - Set line\n");
	int offset[2] = {0x00,0x40};
	lcd_send(LCD_SETDDRAMADDR | offset[line],0);
	return 0;
}

int lcd_clear(){
	printk(KERN_INFO"LCD DD - Clear\n");
	lcd_write(LCD_CLEARDISPLAY,0); //clear display and autimatically set cursor to upper left
	usleep(2000000);
	return 0;
}

int lcd_init(){
	//change 8bit mode to 4 bit mode
	lcd_send_4bits(0x03<<4);
	usleep(4500000);
	lcd_send_4bits(0x03<<4);
	usleep(4500000);
	lcd_send_4bits(0x03<<4);
	usleep(150000);
	lcd_send_4bits(0x02<<4);

	//initiate
	lcd_send(LCD_FUNCTIONSET | LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS,0);
	lcd_send(LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF,0);
	lcd_send(LCD_ENTRYMODESET | LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT,0);

	//clear
	lcd_clear();
	return 0;
}


long lcd_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
	switch(cmd){
		case I2C_SET_SLAVE:
			slaveAddr = (u_int8_t)arg;
			i2c_setSlave(slaveAddr);
			break;
		case LCD_INIT:
			if(slaveAddr == -1){
				printk(KERN_ALERT"LCD DD - Slave Not Set");
				return -1;
			}
			lcd_init();
			break;
		case LCD_WRITE:
			char* str = arg.input;
			int len = arg.len;
			lcd_write(str, len);
			break;
		case LCD_SET_LINE:
			lcd_set_line((int)arg);
			break;
		case LCD_CLEAR:
			lcd_clear();
			break;
		default:
			printk(KERN_ALERT "LCD DD - Unknown Command\n");
			return -1;
	}
	return 0;
}

static struct file_operations lcd_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = lcd_ioctl,
	.open = lcd_open,
	.release = lcd_release
};

int __init lcd_init(void){
	if(register_chrdev(LCD_MAJOR_NUMBER, LCD_DEV_NAME, &lcd_fops)<0)
		printk(KERN_ALERT "LCD DD - Initialization Fail\n");
	else
		printk(KERN_ALERT "LCD DD - Initialization Success\n");
	return 0;
}

void __exit lcd_exit(void){
	unregister_chrdev(LCD_MAJOR_NUMBER, LCD_DEV_NAME);
	printk(KERN_ALERT "LCD DD -  Exit\n");
}

module_init(lcd_init);
module_exit(lcd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("JeongMin Choi");
MODULE_DESCRIPTION("Device Driver for Text LCD with I2C Adapter");