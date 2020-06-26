#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#include <asm/mach/map.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#define STEP_MAJOR_NUMBER 508
#define STEP_DEV_NAME "stepmotor_dev"
#define STEP_MAGIC_NUMBER 'j'

#define STEP_START _IO(STEP_MAGIC_NUMBER, 0)
#define STEP_SPIN _IO(STEP_MAGIC_NUMBER, 1)
#define STEP_REWIND _IO(STEP_MAGIC_NUMBER, 2)
#define STEP_STOP _IO(STEP_MAGIC_NUMBER, 3)

#define GPIO_BASE_ADDR 0x3f200000
#define GPFSEL0 0x00
#define GPFSEL1 0x04
#define GPFSEL2 0x08
#define GPSET0 0x1C
#define GPCLR0 0x28

static void __iomem *gpio_base;
volatile unsigned int *gpsel0;
volatile unsigned int *gpsel1;
volatile unsigned int *gpsel2;
volatile unsigned int *gpset0;
volatile unsigned int *gpclr0;

int phase = 0;

int step_run(void)
{
	if (phase == 0)
	{
		*gpclr0 |= (1<<4);
		*gpclr0 |= (1<<17);
		*gpset0 |= (1<<27);
		*gpset0 |= (1<<22);
	}
	else if (phase == 1)
	{
		*gpclr0 |= (1<<4);
		*gpset0 |= (1<<17);
		*gpset0 |= (1<<27);
		*gpclr0 |= (1<<22);
	}
	else if (phase == 2)
	{
		*gpset0 |= (1<<4);
		*gpset0 |= (1<<17);
		*gpclr0 |= (1<<27);
		*gpclr0 |= (1<<22);
	}	
	else
	{
		*gpset0 |= (1<<4);
		*gpclr0 |= (1<<17);
		*gpclr0 |= (1<<27);
		*gpset0 |= (1<<22);
	}
	
	return 0;
}

int step_stop(void)
{
	*gpclr0 |= (1<<4);
	*gpclr0 |= (1<<17);
	*gpclr0 |= (1<<27);
	*gpclr0 |= (1<<22);
	
	return 0;
}

int step_open(struct inode *inode, struct file *filp)
{
	printk(KERN_ALERT "Step - Open\n");
	
	gpio_base = ioremap(GPIO_BASE_ADDR, 0x60);
	gpsel0 = (volatile unsigned int *)(gpio_base + GPFSEL0);
	gpsel1 = (volatile unsigned int *)(gpio_base + GPFSEL1);
	gpsel2 = (volatile unsigned int *)(gpio_base + GPFSEL2);
	gpset0 = (volatile unsigned int *)(gpio_base + GPSET0);
	gpclr0 = (volatile unsigned int *)(gpio_base + GPCLR0);
	
	return 0;
}

int step_release(struct inode *inode, struct file *filp)
{
	printk(KERN_ALERT "Step - Close\n");
	
	step_stop();
	
	iounmap((void*)gpio_base);
	
	return 0;
}

long step_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	switch(cmd)
	{
		case STEP_START :
			*gpsel0 |= (1<<12);
			*gpsel1 |= (1<<21);
			*gpsel2 |= (1<<21);
			*gpsel2 |= (1<<6);
		
			phase = 0;
			break;
		case STEP_SPIN :
			phase++;
	
			if (phase > 3)
				phase = 0;
		
			step_run();
			break;
		case STEP_REWIND :
			phase--;
		
			if (phase < 0)
				phase = 3;
		
			step_run();
			break;
		case STEP_STOP :
			step_stop();
			break;
		default : printk(KERN_ALERT "Unknown command!\n");
			return -1;
	}
	
	return 0;
}

static struct file_operations step_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = step_ioctl,
	.open = step_open,
	.release = step_release
};

int __init step_init(void)
{
	if(register_chrdev(STEP_MAJOR_NUMBER, STEP_DEV_NAME, &step_fops)<0)
		printk(KERN_ALERT "step driver initialization fail\n");
	else
		printk(KERN_ALERT "step driver initialization success\n");
	return 0;
}

void __exit step_exit(void)
{
	unregister_chrdev(STEP_MAJOR_NUMBER, STEP_DEV_NAME);
	printk(KERN_ALERT "step driver exit done\n");
}

module_init(step_init);
module_exit(step_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yoonjae Seong");
MODULE_DESCRIPTION("Device Driver for Step Motor");
