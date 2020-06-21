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

#define SWITCH_MAJOR_NUMBER 503
#define SWITCH_DEV_NAME "switch_ioctl"

#define GPIO_BASE_ADDR 0x3F200000
#define GPFSEL2 0x08
#define GPSET0 0x1C
#define GPCLR0 0x28
#define GPLEV0 0x034

static void __iomem * gpio_base;
volatile unsigned int * gpsel2;
volatile unsigned int * gpset1;
volatile unsigned int * gpclr1;
volatile unsigned int * gplev1;

static int switch_status = 0;

int switch_open(struct inode * inode, struct file * filp){
    printk(KERN_ALERT "switch driver open!!\n");
    
    gpio_base = ioremap(GPIO_BASE_ADDR, 0x60);
    gpsel2 = (volatile unsigned int *)(gpio_base + GPFSEL2);
    gpset1 = (volatile unsigned int *)(gpio_base + GPSET0);
    gpclr1 = (volatile unsigned int *)(gpio_base + GPCLR0);
    gplev1 = (volatile unsigned int *)(gpio_base + GPLEV0);
    
    *gpsel2 &= (0<<0);//#20 input
    return 0;
}

int switch_release(struct inode *inode, struct file *filp){
    printk(KERN_ALERT "switch driver closed!!\n");
    iounmap((void*)gpio_base);
    return 0;
}

ssize_t switch_read(struct file *filp, char * buf, size_t count, loff_t *f_pos){
    printk(KERN_ALERT "switch_ioclt read function called\n");
 
    switch_status = *gplev1 & (1<<20);
    printk(KERN_INFO "read switch status %d from kernel\n", switch_status);
    copy_to_user(buf, &switch_status, sizeof(int));
    
    return count;
}

static struct file_operations switch_fops = {
    .owner = THIS_MODULE,
    .open = switch_open,
    .read = switch_read,
    .release = switch_release,
};

int __init switch_init(void){
    if(register_chrdev(switch_MAJOR_NUMBER, switch_DEV_NAME, &switch_fops)<0)
        printk(KERN_ALERT "switch driver initialization fail\n");
    else
        printk(KERN_ALERT "switch driver initialization success\n");
    
    return 0;
}

void __exit switch_exit(void){
    unregister_chrdev(SWITCH_MAJOR_NUMBER, SWITCH_DEV_NAME);
    printk(KERN_ALERT "switch driver exit done\n");
}

module_init(switch_init);
module_exit(switch_exit);

MODULE_AUTHOR("Minyeong");

