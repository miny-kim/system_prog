#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include <asm/mach/map.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#define LED_MAJOR_NUMBER 502
#define LED_DEV_NAME "led_ioctl"

#define GPIO_BASE_ADDR 0x3F200000
#define GPFSEL1 0x04
#define GPSET0 0x1C
#define GPCLR0 0x28

#define IOCTL_MAGIC_NUMBER 'j'
#define IOCTL_CMD_CHANGE_STATE     _IOWR(IOCTL_MAGIC_NUMBER, 0, int)

static void __iomem * gpio_base;
volatile unsigned int * gpsel1;
volatile unsigned int * gpset1;
volatile unsigned int * gpclr1;

int led_open(struct inode * inode, struct file * filp){
    printk(KERN_ALERT "LED driver open!!\n");
    
    gpio_base = ioremap(GPIO_BASE_ADDR, 0x60);
    gpsel1 = (volatile unsigned int *)(gpio_base + GPFSEL1);
    gpset1 = (volatile unsigned int *)(gpio_base + GPSET0);
    gpclr1 = (volatile unsigned int *)(gpio_base + GPCLR0);

    *gpsel1 |= (1<<6) //#2 pin output mode
    *gpsel1 |= (1<<9) //#3 pin output mode
    *gpsel1 |= (1<<12) //#4 pin output mode

    return 0;
}

int led_release(struct inode *inode, struct file *filp){
    printk(KERN_ALERT "LED driver closed!!\n");
    iounmap((void*)gpio_base);
    return 0;
}

long led_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
    
    int kbuf = -1;
    
    
    switch(cmd){
        case IOCTL_CMD_CHANGE_STATE:
            copy_from_user(&kbuf, (const void*)arg, 4);
            if(kbuf == 0){
                //set state great
                printk(KERN_ALERT "set state great!!\n");
                *gpset1 |= (1<<2); //on
                *gpclr1 |= (1<<3); //off
                *gpclr1 |= (1<<4); //off

            }else if(kbuf == 1){
                //set state good
                printk(KERN_ALERT "set state good!!\n");
                 *gpset1 |= (1<<3); //on
                *gpclr1 |= (1<<2); //off
                *gpclr1 |= (1<<4); //off
         
            }else if(kbuf == 2){
                //set state bad
                printk(KERN_ALERT "set state bad!!\n");
                *gpset1 |= (1<<4); //on
                *gpclr1 |= (1<<2); //off
                *gpclr1 |= (1<<3); //off
            }
            else{
                //error
                printk(KERN_ALERT "ERROR state parameter error\n");
                return -1;
            }
            break;
     
    }
    return 0;
}


static struct file_operations led_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .release = led_release,
    .unlocked_ioctl = led_ioctl
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

MODULE_AUTHOR("Minyeong");

