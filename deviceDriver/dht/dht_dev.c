#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/sched.h>

#include <asm/mach/map.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#define GPIO_BASE_ADDR	0x3F200000
#define GPFSEL0			0x00
#define GPFSEL1			0x04
#define GPFSEL2			0x08
#define GPSET0			0x1C
#define GPCLR0			0x28
#define GPLEV0			0x34

#define DHT_MAJOR_NUMBER	506
#define DHT_DEV_NAME		"dht_dev"
#define DHT_MAGIC_NUMBER	'j'

#define DHT_START		    _IOW(DHT_MAGIC_NUMBER, 0, unsigned int)
#define DHT_GET_HUMIDITY	_IOR(DHT_MAGIC_NUMBER, 1 , int)

static void __iomem *gpio_base;
volatile unsigned int *gpsel0;
volatile unsigned int *gpsel1;
volatile unsigned int *gpsel2;
volatile unsigned int *gpset0;
volatile unsigned int *gpclr0;
volatile unsigned int *gplev0;

int dht_open(struct inode *inode, struct file *filp){
	printk(KERN_ALERT "DHT - Open\n");
	
	gpio_base = ioremap(GPIO_BASE_ADDR, 0x60);
	gpsel1 = (volatile unsigned int *)(gpio_base + GPFSEL1);
    gpset0 = (volatile unsigned int *)(gpio_base + GPSET0);
	gpclr0 = (volatile unsigned int *)(gpio_base + GPCLR0);
	gplev0 = (volatile unsigned int *)(gpio_base + GPLEV0);

	return 0;
}

int dht_release(struct inode *inode, struct file *filp){
	printk(KERN_ALERT "DHT - Close\n");
	iounmap((void*)gpio_base);
	return 0;
}

void dht_input(void){
    *gpsel1 &= ~(1<<6);
}

void dht_output(void){
    *gpsel1 != (1<<6);
}

int getTimeout(int res){
    int loopCount = 0;
    while((*gplev0>>12)&1==res){
        if(loopCount++ > 10000){
            set_default_priority();
            printk(KERN_ALERT"DHT - Timeout");
            return -1;
        }
    }
    return loopCount;
}

//got this code from internet from Adafruit

void set_max_priority(void) {
    struct sched_param sched;
    memset(&sched, 0, sizeof(sched));
    // Use FIFO scheduler with highest priority for the lowest chance of the kernel context switching.
    sched.sched_priority = sched_get_priority_max(SCHED_FIFO);
    sched_setscheduler(0, SCHED_FIFO, &sched);
}

void set_default_priority(void) {
    struct sched_param sched;
    memset(&sched, 0, sizeof(sched));
    // Go back to default scheduler with default 0 priority.
    sched.sched_priority = 0;
    sched_setscheduler(0, SCHED_OTHER, &sched);
}

//got this code from internet end from Adafruit

int dht_get_humidity(void){
    u_int8_t bytes[5];
    int i;
    int before[40];
    int after[40];

    for(i = 0; i<5; i++){
        bytes[i] = 0;
    }

    dht_output();
    set_max_priority();
    *gpclr0 |= (1<<12);
    msleep(20);//at least 18ms
    //*gpset0 |= (1<<gpio_pin);
    //actually need to wait for 20~40us but no microsecond sleep exist
    dht_input();
    if(getTimeout(0)<0) return -1;//lasts 80us
    if(getTimeout(1)<0) return -1;//lasts 80us

    for(i=0; i < 40; i++){
        if(before[i]=getTimeout(0)<0) return -1;//initiate send 1 bit 50us 
        if(after[i]=getTimeout(1)<0) return -1;//26-28us = 0, 70us = 1
    }
    printk(KERN_INFO"time : %d %d\n",before, after);
    for(i=0; i<40; i++){
        if(after > before){
            bytes[i/8] |= (1<<(7-(i%8)));
        }
    }
    printk(KERN_INFO"DHT - bytes %X %X %X %X %X\n", bytes[0],bytes[1],bytes[2],bytes[3],bytes[4]);
    if((bytes[0]+bytes[1]+bytes[2]+bytes[3]) & 0xFF != bytes[4]){
        set_default_priority();
        printk(KERN_ALERT"DHT - Checksum Error");
        return -1;
    }
    set_default_priority();
    return (int)(bytes[0]);
}

long dht_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
	int buffer = -1;
	switch(cmd){
		case DHT_START:
            copy_from_user(&gpio_pin, (void*)arg, 4);
			break;
        case DHT_GET_HUMIDITY:
			buffer = dht_get_humidity();
			copy_to_user((void*)arg, &buffer, 4);
			break;
		default:
			printk(KERN_ALERT "Unknown command!\n");
			return -1;
	}
	return 0;
}

static struct file_operations dht_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = dht_ioctl,
	.open = dht_open,
	.release = dht_release
};

int __init dht_init(void){
	if(register_chrdev(DHT_MAJOR_NUMBER, DHT_DEV_NAME, &dht_fops)<0)
		printk(KERN_ALERT "dht driver initialization fail\n");
	else
		printk(KERN_ALERT "dht driver initialization success\n");
	return 0;
}

void __exit dht_exit(void){
	unregister_chrdev(DHT_MAJOR_NUMBER, DHT_DEV_NAME);
	printk(KERN_ALERT "dht driver exit done\n");
}

module_init(dht_init);
module_exit(dht_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("JeongMin Choi");
MODULE_DESCRIPTION("Device Driver for DHT11 Temperature and Humidity Sensor GPIO 12");
