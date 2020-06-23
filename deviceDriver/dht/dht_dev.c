#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#include <sys/time.h>

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

#define DHT_MAJOR_NUMBER	504
#define DHT_DEV_NAME		"dht_dev"
#define DHT_MAGIC_NUMBER	'j'

#define DHT_START		_IOW(DHT_MAGIC_NUMBER, 0, unsigned int)
#define DHT_GET_HUMIDITY	_IOR(DHT_MAGIC_NUMBER, 1 , int)

static void __iomem *gpio_base;
volatile unsigned int *gpsel0;
volatile unsigned int *gpsel1;
volatile unsigned int *gpsel2;
volatile unsigned int *gplev0;

unsigned int gpio_pin;

int dht_open(struct inode *inode, struct file *filp){
	printk(KERN_ALERT "Button - Open\n");
	
	gpio_base = ioremap(GPIO_BASE_ADDR, 0x60);
	gpsel0 = (volatile unsigned int *)(gpio_base + GPFSEL0);
	gpsel1 = (volatile unsigned int *)(gpio_base + GPFSEL1);
	gpsel2 = (volatile unsigned int *)(gpio_base + GPFSEL2);
    gpset0 = (volatile unsigned int *)(gpio_base + GPSET0);
	gpclr0 = (volatile unsigned int *)(gpio_base + GPCLR0);
	gplev0 = (volatile unsigned int *)(gpio_base + GPLEV0);

	return 0;
}

int dht_release(struct inode *inode, struct file *filp){
	printk(KERN_ALERT "Button - Close\n");
	iounmap((void*)gpio_base);
	return 0;
}

void dht_input(){
    switch(gpio_pin/10){ //set gpio_pin to input
        case 0:
            *gpsel0 &= ~(1<<((gpio_pin%10)*3));
            break;
        case 1:
            *gpsel1 &= ~(1<<((gpio_pin%10)*3));
            break;
        case 2:
            *gpsel2 &= ~(1<<((gpio_pin%10)*3));
            break;
        default:
            printk(KERN_ALERT"DHT - Invalid GPIO Port Number\n");
    }
}

void dht_output(){
    switch(gpio_pin/10){ //set gpio_pin to input
        case 0:
            *gpsel0 |= (1<<((gpio_pin%10)*3));
            break;
        case 1:
            *gpsel1 |= (1<<((gpio_pin%10)*3));
            break;
        case 2:
            *gpsel2 |= (1<<((gpio_pin%10)*3));
            break;
        default:
            printk(KERN_ALERT"DHT - Invalid GPIO Port Number\n");
    }
}

int getTimeout(int res){
    int loopCount = 0;
    while((*gplev0>>gpio_pin)&1==res){
        if(loopCount++ > 10000){
            printk(KERN_ALERT"DHT - Timeout");
            return -1;
        }
    }
    return 0;
}

int dht_get_humidity(){
    u_int8_t bytes[5];
    int bitCount = 0;
    struct timeval before;
    struct timeval after;

    dht_output();
    *gpclr0 |= (1<<gpio_pin);
    msleep(18000);
    *gpset0 |= (1<<gpio_pin);
    msleep(40);
    dht_input();

    if(getTimeout(0)<0) return -1;
    if(getTimeout(1)<0) return -1;

    for(bitCount=0; bitCount < 40; bitCount++){
        if(getTimeout(0)<0) return -1;
        gettimeofday(&before, NULL);
        if(getTimeout(1)<0) return -1;
        gettimeofday(&after, NULL);
        if(after.tv_usec - before.tv_usec > 40){
            
        }
    }


}

long dht_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
	int buffer = -1;
	switch(cmd){
		case DHT_START:
			copy_from_user(&gpio_pin, (void*)arg, 4);
			break;
		case DHT_GET_HUMIDITY:
			buffer = (*gplev0>>gpio_pin) & 1;
			printk(KERN_ALERT "Button - Get State %d\n", buffer);
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
MODULE_DESCRIPTION("Device Driver for DHT11 Temperature and Humidity Sensor");
