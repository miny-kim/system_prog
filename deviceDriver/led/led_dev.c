#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#include <asm/mach/map.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#define LED_MAJOR_NUMBER 502
#define LED_DEV_NAME "led_ioctl"
#define LED_MAGIC_NUMBER 'j'

#define GPIO_BASE_ADDR 0x3F200000
#define GPFSEL1 0x04
#define GPSET0 0x1C
#define GPCLR0 0x28

#define LED_CONTROL _IOW(LED_MAGIC_NUMBER, 0, struct control_data)

static void __iomem *gpio_base;
volatile unsigned int *gpsel1;
volatile unsigned int *gpset0;
volatile unsigned int *gpclr0;

struct control_data{
    int color;
    int on;
}

int led_open(struct inode *inode, struct file *filp){
	printk(KERN_ALERT "LED driver open!!\n");
	
	gpio_base = ioremap(GPIO_BASE_ADDR, 0x60);
	gpsel1 = (volatile unsigned int *)(gpio_base + GPFSEL1);
	gpset0 = (volatile unsigned int *)(gpio_base + GPSET0);
	gpclr0 = (volatile unsigned int *)(gpio_base + GPCLR0);
	
    *gpsel1 |= (1<<18); //gpio 16 = R
    *gpsel1 |= (1<<21); //gpio 17 = G
	*gpsel1 |= (1<<24); //gpio 18 = B
	
	return 0;
}

int led_release(struct inode *inode, struct file *filp){
	printk(KERN_ALERT "LED driver closed!!\n");
	iounmap((void*)gpio_base);
	return 0;
}

long led_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
	struct control_data c_data;
    int reg;
    switch(cmd){
		case LED_CONTROL:
			copy_from_user((void*)arg, &c_data, sizeof(struct control_data));
            if(0<=c_data.color && c_data.color<=2){
                reg = 16 + c_data.color;
            }else{
                printk(KERN_ALERT "LED - Invalid Color\n");
			    return -1;
            }
            if(c_data.on){
                gpset0 |= (1<<reg);
            }else{
                gpclr0 |= (1<<reg);
            }
			break;
		default:
			printk(KERN_ALERT "Unknown command!\n");
			return -1;
	}
	return 0;
}

static struct file_operations led_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = led_ioctl,
	.open = led_open,
	.release = led_release
};

int __init led_init(void){
	if(register_chrdev(LED_MAJOR_NUMBER, LED_DEV_NAME, &led_fops)<0)
		printk(KERN_ALERT "LED driver initialization fail\n");
	else
		printk(KERN_ALERT "LED driver initialization success\n");
	return 0;
}

void __exit led_exit(void){
	unregister_chrdev(LED_MAJOR_NUMBER, LED_DEV_NAME);
	printk(KERN_ALERT "LED driver exit done\n");
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("JeongMin Choi");
MODULE_DESCRIPTION("des");
