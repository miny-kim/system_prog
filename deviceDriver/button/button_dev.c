#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#include <asm/mach/map.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#define GPIO_BASE_ADDR 0x3F200000
#define GPFSEL2 0x08
#define GPSET0 0x1C
#define GPLEV0 0x34

#define BUTTON_MAJOR_NUMBER 504
#define BUTTON_DEV_NAME "button_dev"
#define BUTTON_MAGIC_NUMBER 'j'

#define BUTTON_GET_STATE _IOR(BUTTON_MAGIC_NUMBER, 0 , int)

static void __iomem *gpio_base;
volatile unsigned int *gpsel2;
volatile unsigned int *gpset0;
volatile unsigned int *gplev0;

int button_open(struct inode *inode, struct file *filp){
	printk(KERN_ALERT "Button - Open\n");
	
	gpio_base = ioremap(GPIO_BASE_ADDR, 0x60);
	gpsel2 = (volatile unsigned int *)(gpio_base + GPFSEL2);
	gpset0 = (volatile unsigned int *)(gpio_base + GPSET0);
	gplev0 = (volatile unsigned int *)(gpio_base + GPLEV0);

	return 0;
}

int button_release(struct inode *inode, struct file *filp){
	printk(KERN_ALERT "Button - Close\n");
	iounmap((void*)gpio_base);
	return 0;
}

long button_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
	int buffer = -1;
	//*gpset0 |= (1<<21); //see if I can change this with 3.3V
	
	switch(cmd){
		case BUTTON_GET_STATE:
			buffer = (*gplev0>>20) & 1;
			printk(KERN_ALERT "Button - Get State %d\n", buffer);
			copy_to_user((void*)arg, &buffer, 4);
			break;
		default:
			printk(KERN_ALERT "Unknown command!\n");
			return -1;
	}
	return 0;
}

static struct file_operations button_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = button_ioctl,
	.open = button_open,
	.release = button_release
};

int __init button_init(void){
	if(register_chrdev(BUTTON_MAJOR_NUMBER, BUTTON_DEV_NAME, &button_fops)<0)
		printk(KERN_ALERT "button driver initialization fail\n");
	else
		printk(KERN_ALERT "button driver initialization success\n");
	return 0;
}

void __exit button_exit(void){
	unregister_chrdev(BUTTON_MAJOR_NUMBER, BUTTON_DEV_NAME);
	printk(KERN_ALERT "button driver exit done\n");
}

module_init(button_init);
module_exit(button_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("JeongMin Choi");
MODULE_DESCRIPTION("Device Driver for Button & Switch");
