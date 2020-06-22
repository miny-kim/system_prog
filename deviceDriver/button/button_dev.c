#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#include <asm/mach/map.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#define GPIO_BASE_ADDR	0x3F200000
#define GPFSEL0			0x00
#define GPFSEL1			0x04
#define GPFSEL2			0x08
#define GPLEV0			0x34

#define BUTTON_MAJOR_NUMBER	504
#define BUTTON_DEV_NAME		"button_dev"
#define BUTTON_MAGIC_NUMBER	'j'

#define BUTTON_START		_IOW(BUTTON_MAGIC_NUMBER, 0, unsigned int)
#define BUTTON_GET_STATE	_IOR(BUTTON_MAGIC_NUMBER, 1 , int)

static void __iomem *gpio_base;
volatile unsigned int *gpsel0;
volatile unsigned int *gpsel1;
volatile unsigned int *gpsel2;
volatile unsigned int *gplev0;

unsigned int gpio_input;

int button_open(struct inode *inode, struct file *filp){
	printk(KERN_ALERT "Button - Open\n");
	
	gpio_base = ioremap(GPIO_BASE_ADDR, 0x60);
	gpsel0 = (volatile unsigned int *)(gpio_base + GPFSEL0);
	gpsel1 = (volatile unsigned int *)(gpio_base + GPFSEL1);
	gpsel2 = (volatile unsigned int *)(gpio_base + GPFSEL2);
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
	switch(cmd){
		case BUTTON_START:
			copy_from_user(&gpio_input, (void*)arg, 4);
			switch(gpio_input/10){ //set gpio_input to input
				case 0:
					*gpsel0 &= ~(111<<((gpio_input%10)*3));
					break;
				case 1:
					*gpsel1 &= ~(111<<(((gpio_input)%10)*3));
					break;
				case 2:
					*gpsel2 &= ~(111<<(((gpio_input)%10)*3));
					break;
				default:
					printk(KERN_ALERT"BUTTON - Invalid GPIO Port Number\n");
			}
			break;
		case BUTTON_GET_STATE:
			buffer = (*gplev0>>gpio_input) & 1;
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
