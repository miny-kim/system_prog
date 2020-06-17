#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#include <asm/mach/map.h>
#include <asm/io.h>
#include <asm/uaccess.h>

//#include "i2c.h"

#define GPIO_BASE_ADDR  0x3F200000
#define BSC1_BASE_ADDR  0x3F804000

#define GPFSEL0 0x00

#define BSC 0x00
#define BSS 0x04
#define BSDLEN  0x08
#define BSA 0x0C
#define BSFIFO  0x10
#define BSDIV   0x14
#define BSDEL   0x18
#define BSCLKT  0x1C


static void __iomem *gpio_base;
static void __iomem *bsc1_base;

volatile unsigned int *gpfsel0;

volatile unsigned int *bsc1;
volatile unsigned int *bss1;
volatile unsigned int *bsdlen1;
volatile unsigned int *bsa1;
volatile unsigned int *bsfifo1;
volatile unsigned int *bsdiv1;
volatile unsigned int *bsdel1;
volatile unsigned int *bsclkt1;



void i2c_open(void){
    gpio_base = ioremap(GPIO_BASE_ADDR, 0x18);
    bsc1_base = ioremap(BSC1_BASE_ADDR, 0x20);

    gpfsel0 = (volatile unsigned int *)(gpio_base + GPFSEL0);
    
    bsc1 = (volatile unsigned int *)(bsc1_base + BSC); //control
    bss1 = (volatile unsigned int *)(bsc1_base + BSS); //status
    bsdlen1 = (volatile unsigned int *)(bsc1_base + BSDLEN); //data length
    bsa1 = (volatile unsigned int *)(bsc1_base + BSA); // slave address
    bsfifo1 = (volatile unsigned int *)(bsc1_base + BSFIFO); // data fifo

    *gpfsel0 |= (0b100<<6); //SDA to alt
	*gpfsel0 |= (0b100<<9); //SCL to alt
}

void i2c_release(void){
    *gpfsel0 &= ~(0b100<<6); //SDA to input
	*gpfsel0 &= ~(0b100<<9); //SCL to input

    iounmap((void*)gpio_base);
    iounmap((void*)bsc1_base);
}

void i2c_setSlave(u_int8_t addr){
    *bsa1 = addr & 0x7F; //set slave address
	printk(KERN_INFO"I2C Slave - Slave Set to %X\n", *bsa1);
}

int i2c_write(u_int8_t* buf, int len){
    int i = 0;
	int maxBuf = 16;
	printk(KERN_INFO"I2C Write - writing data %d len\n", len);
	*bsc1 |= 0x30; //clear fifo
    *bsc1 &= ~(0x1); //set to write packet transfer

    *bss1 &= ~(0x302); // clear status

    *bsdlen1 = (*bsdlen1 & (~0xFFFF)) | (len & 0xFFFF); //set length

    while(i<len && i < maxBuf){ //fill buffer before sending
        *bsfifo1 = buf[i];
        i++;
    }

    *bsc1 |= 0x8080; //start transfer


    while(!(*bss1 & 0x2)){//while not done
        while(i<len && *bss1&(0x10)){ //fill the rest into buffer
            *bsfifo1 = buf[i];
            i++;
        }
    }

    if(*bss1 & 0x200){ //clock stretch timeout
        printk(KERN_ALERT "I2C Write - Clock Stretch Timeout\n");
        return -1;
    }
    else if (*bss1 & 0x100){ //ACK error
        printk(KERN_ALERT "I2C Write - No Slave Acknowledged the Address\n");
        return -1;
    }
    else if (i<len){//part of data wasn't sent
        printk(KERN_ALERT "I2C Write - Part of Data Not Sent\n");
        return -1;
    }

    *bss1 |= 0x2; //set done

    return 0;
}

int i2c_write_one(u_int8_t buf){
    return i2c_write(&buf, 1);
}

bool i2c_read(u_int8_t* buf, int len){
	int i =0;
    *bsc1 |= 0x30; //clear fifo
    *bsc1 |= 0x1; //set to read packet transfer

    *bss1 &= ~(0x302); // clear status

    *bsdlen1 = (*bsdlen1 & (~0xFFFF)) | (len & 0xFFFF); //set length

    *bsc1 |= 0x8081; //start transfer

    while(!(*bss1 & 0x2)){//while not done
        while(i<len && *bss1&(0x20)){ //empty the buffer
            buf[i] = *bsfifo1 & 0xFF;
            i++;
        }
    }

    while(i<len && *bss1&(0x20)){ //empty rest of the buffer
        buf[i] = *bsfifo1 & 0xFF;
        i++;
    }

    if(*bss1 & 0x200){ //clock stretch timeout
        printk(KERN_ALERT "I2C Read - Clock Stretch Timeout\n");
        return false;
    }
    else if (*bss1 & 0x100){ //ACK error
        printk(KERN_ALERT "I2C Read - No Slave Acknowledged the Address\n");
        return false;
    }
    else if (i<len){//part of data wasn't received
        printk(KERN_ALERT "I2C Read - Part of Data Not Received\n");
        return false;
    }

    *bss1 |= 0x2; //set done

    return true;
}


#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include <asm/mach/map.h>
#include <asm/io.h>
#include <asm/uaccess.h>

//#include "../i2c/i2c.h"

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
};

#define LCD_START		_IOW(LCD_MAGIC_NUMBER, 0 , u_int8_t)
#define LCD_WRITE		_IOW(LCD_MAGIC_NUMBER, 1, struct write_data)
#define LCD_SET_LINE	_IOW(LCD_MAGIC_NUMBER, 2, int)
#define LCD_CLEAR		_IO(LCD_MAGIC_NUMBER, 3)

u_int8_t* slaveAddr = NULL;

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
	i2c_write_one(value | LCD_BACKLIGHT);
	i2c_write_one(value | En | LCD_BACKLIGHT);
	msleep(1);
	i2c_write_one( (value & (~En)) | LCD_BACKLIGHT);
	msleep(50);
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
	int i;
	printk(KERN_INFO "LCD DD - Write\n");
	for(i =0; i<len; i++){
		lcd_send((u_int8_t)str[i], Rs);
	}
	return 0;
}

int lcd_set_line(int line){ //line = 0 : first line, line = 1 : second line
	int offset[2] = {0x00,0x40};
	printk(KERN_INFO "LCD DD - Set line\n");
	lcd_send(LCD_SETDDRAMADDR | offset[line],0);
	return 0;
}

int lcd_clear(void){
	printk(KERN_INFO"LCD DD - Clear\n");
	lcd_send(LCD_CLEARDISPLAY,0); //clear display and autimatically set cursor to upper left
	msleep(2000);
	return 0;
}

int lcd_start(void){
	//change 8bit mode to 4 bit mode
	printk(KERN_INFO"LCD DD - Start\n");
	lcd_send_4bits(0x03<<4);
	msleep(4500);
	lcd_send_4bits(0x03<<4);
	msleep(4500);
	lcd_send_4bits(0x03<<4);
	msleep(150);
	lcd_send_4bits(0x02<<4);

	//initiate
	lcd_send(LCD_FUNCTIONSET | LCD_4BITMODE | LCD_2LINE | LCD_5x8DOTS,0);
	lcd_send(LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF,0);
	lcd_send(LCD_ENTRYMODESET | LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT,0);

	//clear
	lcd_clear();
	return 0;
}


long lcd_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
	char* str;
	int len;
	struct write_data* in;
	
	switch(cmd){
		case LCD_START:
			slaveAddr = (u_int8_t*) arg;
			i2c_setSlave(*slaveAddr);
			lcd_start();
			break;
		case LCD_WRITE:
			in = (struct write_data *)arg;
			str = in->input;
			len = in->len;
			printk(KERN_INFO"LCD_WRITE : Writing %s, len %d\n",str,len);
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
