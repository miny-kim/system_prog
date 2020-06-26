#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#include <asm/mach/map.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#define DOOR_MAJOR_NUMBER 480
#define DOOR_MINOR_NUMBER 105
#define DOOR_DEV_NAME "doorsensor_dev"
#define DOOR_MAGIC_NUMBER 'l'

#define DOOR_START _IOW(DOOR_MAGIC_NUMBER, 0, unsigned int)
#define DOOR_GET_STATE _IOR(DOOR_MAGIC_NUMBER, 1, int)

#define GPIO_BASE_ADDR 0x3f200000
#define GPFSEL2 0x08
#define GPLEV0 0x34

static void __iomem *gpio_base;
volatile unsigned int *gpsel2;
volatile unsigned int *gplev0;

int door_open(struct inode *inode, struct file *filp)
{
	printk(KERN_ALERT "Door - Open\n");
	
	gpio_base = ioremap(GPIO_BASE_ADDR, 0x60);
	gpsel2 = (volatile unsigned int *)(gpio_base + GPFSEL2);
	gplev0 = (volatile unsigned int *)(gpio_base + GPLEV0);
	
	return 0;
}

int door_release(struct inode *inode, struct file *filp)
{
	printk(KERN_ALERT "Door - Close\n");
	
	iounmap((void*)gpio_base);
	
	return 0;
}

long door_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int buffer = -1;
	
	switch(cmd)
	{
		case DOOR_START :
			*gpsel2 &= ~(111<<12);
			break;
		case DOOR_GET_STATE :
			buffer = (*gplev0>>24) & 1;
			copy_to_user((void*)arg, &buffer, 4);
			break;
		default : printk(KERN_ALERT "Unknown command!\n");
			return -1;
	}
	
	return 0;
}

static struct file_operations door_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = door_ioctl,
	.open = door_open,
	.release = door_release
};

int __init door_init(void)
{
	if(register_chrdev(DOOR_MAJOR_NUMBER, DOOR_DEV_NAME, &door_fops)<0)
		printk(KERN_ALERT "door driver initialization fail\n");
	else
		printk(KERN_ALERT "door driver initialization success\n");
	return 0;
}

void __exit door_exit(void)
{
	unregister_chrdev(DOOR_MAJOR_NUMBER, DOOR_DEV_NAME);
	printk(KERN_ALERT "door driver exit done\n");
}

module_init(door_init);
module_exit(door_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yoonjae Seong");
MODULE_DESCRIPTION("Device Driver for Door Sensor");
