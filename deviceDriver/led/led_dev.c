#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#include <asm/mach/map.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#define LED_MAJOR_NUMBER 505
#define LED_DEV_NAME "led_dev"
#define LED_MAGIC_NUMBER 'j'

#define GPIO_BASE_ADDR 0x3F200000
#define GPFSEL1 0x04
#define GPSET0 0x1C
#define GPCLR0 0x28

#define LED_CONTROL _IOW(LED_MAGIC_NUMBER, 0, int)

static void __iomem *gpio_base;
volatile unsigned int *gpsel1;
volatile unsigned int *gpset0;
volatile unsigned int *gpclr0;

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
	*gpclr0 |= (1<<16); //off
    *gpclr0|= (1<<17); //off
    *gpclr0 |= (1<<18); //off
	printk(KERN_ALERT "LED driver closed!!\n");
	iounmap((void*)gpio_base);
	return 0;
}

long led_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
	int kbuf = -1;
    switch(cmd){
		case LED_CONTROL:
			copy_from_user(&kbuf, (const void*)arg, 4);
			if(kbuf == 0){
				printk(KERN_INFO "LED - Black\n");
                *gpclr0 |= (1<<16); //off
                *gpclr0|= (1<<17); //off
                *gpclr0 |= (1<<18); //off
			}else if(kbuf == 1){
                //set state great
                printk(KERN_INFO "LED - Red\n");
                *gpset0 |= (1<<16); //on
                *gpclr0|= (1<<17); //off
                *gpclr0 |= (1<<18); //off
            }else if(kbuf == 2){
                //set state good
                printk(KERN_INFO "LED - Green\n");
                *gpclr0 |= (1<<16); //off
                *gpset0 |= (1<<17); //on
                *gpclr0 |= (1<<18); //off
            }else if(kbuf == 3){
                //set state bad
                printk(KERN_INFO "LED - Blue\n");
                *gpclr0 |= (1<<16); //off
                *gpclr0 |= (1<<17); //off
                *gpset0 |= (1<<18); //on
            }
            else{
                //error
                printk(KERN_ALERT "ERROR state parameter error\n");
                return -1;
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
