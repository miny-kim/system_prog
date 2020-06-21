#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/syscalls.h>

#include <asm/mach/map.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#include <linux/unistd.h>         //Used for UART
#include <linux/fcntl.h>         //Used for UART
#include <linux/termios.h>      //Used for UART

#define UART_MAJOR_NUMBER 502
#define UART_DEV_NAME "uart_ioctl"

#define IOCTL_MAGIC_NUMBER 'u'
#define IOCTL_CMD_TRANSMIT _IOWR(IOCTL_MAGIC_NUMBER, 0, int)
#define IOCTL_CMD_RECEIVE _IOWR(IOCTL_MAGIC_NUMBER, 1, int)

#define BASE_ADDR 0x3F000000
#define GPIO_BASE_ADDR 0x200000
#define mUART_BASE_ADDR 0x215000

#define GPFSEL1 0x04
#define EN 0x04
#define MUIO 0x40
#define MUIER 0x44
#define MUIIR 0x48
#define MULCR 0x4C
#define MUMCR 0x50
#define MULSR 0x54
#define MUCNTL 0x60
#define MUBAUD 0x68

static void __iomem *gpio_base;
static void __iomem *muart_base;

volatile unsigned int *en;
volatile unsigned int *muio;
volatile unsigned int *muier;
volatile unsigned int *muiir;
volatile unsigned int *mulcr;
volatile unsigned int *mumcr;
volatile unsigned int *mulsr;
volatile unsigned int *mucntl;
volatile unsigned int *mubaud;
volatile unsigned int *gpfsel1;

void Setup_MINIUART(void)
{
   muart_base = ioremap(BASE_ADDR + mUART_BASE_ADDR, 0x60);
   gpio_base = ioremap(BASE_ADDR + GPIO_BASE_ADDR, 0x60);

   en = (volatile unsigned int *)(muart_base + EN);
   muio = (volatile unsigned int *)(muart_base + MUIO);
   muier = (volatile unsigned int *)(muart_base + MUIER);
   muiir = (volatile unsigned int *)(muart_base + MUIIR);
   mulcr = (volatile unsigned int *)(muart_base + MULCR);
   mumcr = (volatile unsigned int *)(muart_base + MUMCR);
   mulsr = (volatile unsigned int *)(muart_base + MULSR);
   mucntl = (volatile unsigned int *)(muart_base + MUCNTL);
   mubaud = (volatile unsigned int *)(muart_base + MUBAUD);
   
   gpfsel1 = (volatile unsigned int *)(muart_base + GPFSEL1);
   
   *en |= 1;
   *mucntl = 0;
   *mulcr = 3;
   *mumcr = 0;
   *muier = 0;
   *muiir = 0xC1;
   *mubaud = 270;
   
   *gpfsel1 |= (2 << 12); //change function 
   *gpfsel1 |= (2 << 15);
   *mucntl = 3;
   
   printk(KERN_INFO "CLEAR? %d",(*mulsr&(1<<6)));

}

void Send_MINIUART(unsigned int data){
   while (!(*mulsr & (1 << 5)));
   printk(KERN_ALERT "data send %c\n", data);
   *muio = data;
   //printk(KERN_ALERT "UART buffer send %d\n", (*muio));
}

int Receive_MINIUART(void)
{
   int temp;
   if(!(*mulsr & 1))
   {
      return -1;
   }
   temp = (*muio);
   printk(KERN_ALERT "UART buffer receive %c\n", temp);
   return temp;
}

long uart_ioctl(struct file *flip, unsigned int cmd, unsigned long arg)
{
   int kbuf = -1;
   
   switch(cmd) {         
      case IOCTL_CMD_TRANSMIT:
         copy_from_user(&kbuf, (const void*)arg, sizeof(char));
         Send_MINIUART(kbuf);
         break;
         
      case IOCTL_CMD_RECEIVE:
         kbuf = Receive_MINIUART();
         return kbuf;
         break;
      }
  
   return 0;
}

int uart_open(struct inode *inode, struct file *flip){
   printk(KERN_ALERT "UART driver open!!\n");
   Setup_MINIUART();
   return 0;
}

int uart_release(struct inode *inode, struct file *flip){
   printk(KERN_ALERT "UART driver closed!!\n");
   return 0;
}

static struct file_operations uart_fops = {
   .owner = THIS_MODULE,
   .open = uart_open,
   .release = uart_release,
   .unlocked_ioctl = uart_ioctl
};

int __init uart_init(void){
   if(register_chrdev(UART_MAJOR_NUMBER, UART_DEV_NAME, &uart_fops) <0)
      printk(KERN_ALERT "UART driver init fail\n");
   else
   {
      printk(KERN_ALERT "UART driver init success\n");
   }
   return 0;
}

void __exit uart_exit(void){
   unregister_chrdev(UART_MAJOR_NUMBER, UART_DEV_NAME);
   printk(KERN_ALERT "UART driver exit done\n");
}

module_init(uart_init);
module_exit(uart_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("des");
