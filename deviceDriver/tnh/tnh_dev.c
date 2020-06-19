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

#define TNH_MAJOR_NUMBER	504
#define TNH_DEV_NAME		"tnh_dev"
#define TNH_MAGIC_NUMBER	'j'

#define TNH_READ        0x03
#define TNH_HUMID       0x00

#define TNH_START		    _IOW(TNH_MAGIC_NUMBER, 0 , u_int8_t)
#define TNH_READ_HUMIDITY   _IOR(TNH_MAGIC_NUMBER, 1, int*)

u_int8_t* slaveAddr = NULL;

int tnh_open(struct inode *inode, struct file *filp){
	printk(KERN_ALERT "TNH DD -  Open\n");
	i2c_open();
	return 0;
}

int tnh_release(struct inode *inode, struct file *filp){
	printk(KERN_ALERT "TNH DD -  Release\n");
	i2c_release();
	return 0;
}

unsigned short crc16(unsigned char *ptr, unsigned char len) {//copied from AM2320-Aosong Datasheet
    unsigned short crc =0xFFFF;
    unsigned char i;
    while(len--){
        crc ^=*ptr++;
        for(i=0;i<8;i++){
            if(crc & 0x01){
                crc>>=1;
                crc^=0xA001;
            }else{
                crc>>=1;
            }
        }
    }
    return crc;
} 

int tnh_read_humidity(int* humidity){
    u_int8_t msg[3];
    u_int8_t buf[6];
    msg[0] = TNH_READ;
    msg[1] = TNH_HUMID;
    msg[2] = 0x02;
    i2c_write_one(0x00); //wake up
    msleep(10);
    i2c_write(msg, 3);
    msleep(2);

    i2c_read(buf, 6);

    if(buf[0] != 0x03){ //wrong function code
        printk(KERN_ALERT "TNH Read Humidity - Wrong Function Code\n");
        return -1;
    }
    if(buf[1] != 0x02){//length of data isn't two bytes
        printk(KERN_ALERT "TNH Read Humidity - Wrong Data Length\n");
        return -1;
    }

    if(((u_int16_t)buf[5]<<8 | buf[4]) != crc16(buf, 4)){
        printk(KERN_ALERT "TNH Read Humidity - Checksum Error\n");
        return -1;
    }

    *humidity = (buf[2]<<8) | buf[3];
    return 0;
}

long tnh_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
	int* humidity;
    switch(cmd){
		case TNH_START:
            slaveAddr = (u_int8_t*) arg;
			i2c_setSlave(*slaveAddr);
            break;
        case TNH_READ_HUMIDITY:
            if(tnh_read_humidity(humidity)<0){
                printk(KERN_ALERT "TNH DD - Read Error\n");
			    return -1;
            }
            copy_to_user((void*)arg, humidity, 4);
            break;
        default:
			printk(KERN_ALERT "TNH DD - Unknown Command\n");
			return -1;
	}
	return 0;
}

static struct file_operations tnh_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = tnh_ioctl,
	.open = tnh_open,
	.release = tnh_release
};

int __init tnh_init(void){
	if(register_chrdev(TNH_MAJOR_NUMBER, TNH_DEV_NAME, &tnh_fops)<0)
		printk(KERN_ALERT "TNH DD - Initialization Fail\n");
	else
		printk(KERN_ALERT "TNH DD - Initialization Success\n");
	return 0;
}

void __exit tnh_exit(void){
	unregister_chrdev(TNH_MAJOR_NUMBER, TNH_DEV_NAME);
	printk(KERN_ALERT "TNH DD -  Exit\n");
}

module_init(tnh_init);
module_exit(tnh_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("JeongMin Choi");
MODULE_DESCRIPTION("Device Driver for Temperature and Humidity Sensor");