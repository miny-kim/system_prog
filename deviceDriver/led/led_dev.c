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

#define GPIO_BASE_ADDR	0x3F200000
#define GPFSEL0			0x00
#define GPFSEL1			0x04
#define GPFSEL2			0x08
#define GPSET0			0x1C
#define GPCLR0			0x28

#define LED_START _IOW(LED_MAGIC_NUMBER, 0, unsigned int[3])
#define LED_CONTROL _IOW(LED_MAGIC_NUMBER, 1, int)

static void __iomem *gpio_base;
volatile unsigned int *gpsel0;
volatile unsigned int *gpsel1;
volatile unsigned int *gpsel2;
volatile unsigned int *gpset0;
volatile unsigned int *gpclr0;

unsigned int gpio_color[3];


int led_open(struct inode *inode, struct file *filp){
	printk(KERN_ALERT "LED driver open!!\n");
	
	gpio_base = ioremap(GPIO_BASE_ADDR, 0x60);
	gpsel0 = (volatile unsigned int *)(gpio_base + GPFSEL0);
	gpsel1 = (volatile unsigned int *)(gpio_base + GPFSEL1);
	gpsel2 = (volatile unsigned int *)(gpio_base + GPFSEL2);
	gpset0 = (volatile unsigned int *)(gpio_base + GPSET0);
	gpclr0 = (volatile unsigned int *)(gpio_base + GPCLR0);
	
	*gpsel1 |= (1<<18); //gpio 16 = R
    *gpsel1 |= (1<<21); //gpio 17 = G
	*gpsel1 |= (1<<24); //gpio 18 = B
	return 0;
}

int led_release(struct inode *inode, struct file *filp){
	int i;
	for(i=0; i<3; i++){
		*gpclr0 |=  (1<<gpio_color[i]);
	}
	printk(KERN_ALERT "LED driver closed!!\n");
	iounmap((void*)gpio_base);
	return 0;
}

long led_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
	int kbuf = -1;
	int i;
    switch(cmd){
		case LED_START:
			copy_from_user(&gpio_color, (const void*)arg, sizeof(gpio_color));
			for(i=0; i<3; i++){
				switch(gpio_color[i]/10){ //set gpio_color to output
				case 0:
					*gpsel0 |= (1<<(((gpio_color[i])%10)*3));
					break;
				case 1:
					*gpsel1 |= (1<<(((gpio_color[i])%10)*3));
					break;
				case 2:
					*gpsel2 |= (1<<(((gpio_color[i])%10)*3));
					break;
				default:
					printk(KERN_ALERT"BUTTON - Invalid GPIO Port Number\n");
				}
			}
			break;
		case LED_CONTROL:
			copy_from_user(&kbuf, (const void*)arg, 4);
			if(0<=kbuf && kbuf <=3){
				for(i=0; i<3; i++){
					if(kbuf-1==i) *gpset0 |= (1<<gpio_color[i]);
					else *gpclr0 |=  (1<<gpio_color[i]);
				}
				switch(kbuf){
					case 0:
						printk(KERN_INFO "LED - Black\n");
						break;
					case 1:
						printk(KERN_INFO "LED - Red\n");
						break;
					case 2:
						printk(KERN_INFO "LED - Green\n");
						break;
					case 3:
						printk(KERN_INFO "LED - Green\n");
						break;
				}
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
