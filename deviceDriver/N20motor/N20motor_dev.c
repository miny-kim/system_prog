#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#include <asm/mach/map.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#define N20_MAJOR_NUMBER 510
#define N20_MINOR_NUMBER 100
#define N20_DEV_NAME "N20motor_dev"
#define N20_MAGIC_NUMBER 'k'

#define N20_START _IO(N20_MAGIC_NUMBER, 0)
#define N20_SPIN _IO(N20_MAGIC_NUMBER, 1)
#define N20_STOP _IO(N20_MAGIC_NUMBER, 2)

#define GPIO_BASE_ADDR 0x3f200000
#define GPFSEL1 0x04
#define GPFSEL2 0x08
#define GPSET0 0x1C
#define GPCLR0 0x28

static void __iomem *gpio_base;
volatile unsigned int *gpsel1;
volatile unsigned int *gpsel2;
volatile unsigned int *gpset0;
volatile unsigned int *gpclr0;


int n20_run(void)
{
	*gpclr0 |= (1<<18);
	*gpset0 |= (1<<23);
	
	return 0;
}

int n20_stop(void)
{
	*gpclr0 |= (1<<18);
	*gpclr0 |= (1<<23);
	
	return 0;
}

int n20_open(struct inode *inode, struct file *filp)
{
	printk(KERN_ALERT "N20 - Open\n");
	
	gpio_base = ioremap(GPIO_BASE_ADDR, 0x60);
	gpsel1 = (volatile unsigned int *)(gpio_base + GPFSEL1);
	gpsel2 = (volatile unsigned int *)(gpio_base + GPFSEL2);
	gpset0 = (volatile unsigned int *)(gpio_base + GPSET0);
	gpclr0 = (volatile unsigned int *)(gpio_base + GPCLR0);
	
	return 0;
}

int n20_release(struct inode *inode, struct file *filp)
{
	printk(KERN_ALERT "N20 - Close\n");
	
	n20_stop();
	
	iounmap((void*)gpio_base);
	
	return 0;
}

long n20_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	switch(cmd)
	{
		case N20_START :
			*gpsel1 |= (1<<24);
			*gpsel2 |= (1<<9);
			break;
		case N20_SPIN :
			n20_run();
			break;
		case N20_STOP :
			n20_stop();
			break;
		default : printk(KERN_ALERT "Unknown command!\n");
			return -1;
	}
	
	return 0;
}

static struct file_operations n20_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = n20_ioctl,
	.open = n20_open,
	.release = n20_release
};

int __init n20_init(void)
{
	if(register_chrdev(N20_MAJOR_NUMBER, N20_DEV_NAME, &n20_fops)<0)
		printk(KERN_ALERT "n20 driver initialization fail\n");
	else
		printk(KERN_ALERT "n20 driver initialization success\n");
	return 0;
}

void __exit n20_exit(void)
{
	unregister_chrdev(N20_MAJOR_NUMBER, N20_DEV_NAME);
	printk(KERN_ALERT "n20 driver exit done\n");
}

module_init(n20_init);
module_exit(n20_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yoonjae Seong");
MODULE_DESCRIPTION("Device Driver for N20 Motor");
