#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#include <asm/mach/map.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#include "i2c.h"

#define GPIO_BASE_ADDR  0x3F200000
#define BSC1_BASE_ADDR  0x3F804000

#define GPFSEL0 0x00

#define BSC 0x00
#define BSS 0x04
#define BSDLEN  0x08
#define BSA 0x0C
#define BSFIFO  0x10
#define BSDIV   0x14
#define BSDEL   0x18
#define BSCLKT  0x1C


static void __iomem *gpio_base;
static void __iomem *bsc1_base;

volatile unsigned int *gpfsel0;

volatile unsigned int *bsc1;
volatile unsigned int *bss1;
volatile unsigned int *bsdlen1;
volatile unsigned int *bsa1;
volatile unsigned int *bsfifo1;
volatile unsigned int *bsdiv1;
volatile unsigned int *bsdel1;
volatile unsigned int *bsclkt1;

void i2c_open(){
    gpio_base = ioremap(GPIO_BASE_ADDR, 0x18);
    bsc1_base = ioremap(BCS1_BASE_ADDR, 0x20);

    gpfsel0 = (volatile unsigned int *)(gpio_base + GPFSEL0);
    
    bsc1 = (volatile unsigned int *)(bsc1_base + BSC); //control
    bss1 = (volatile unsigned int *)(bsc1_base + BSS); //status
    bsdlen1 = (volatile unsigned int *)(bsc1_base + BSDLEN); //data length
    bsa1 = (volatile unsigned int *)(bsc1_base + BSA); // slave address
    bsfifo1 = (volatile unsigned int *)(bsc1_base + BSFIFO); // data fifo

    *gpfsel0 |= (0b100<<6); //SDA to alt
	*gpfsel0 |= (0b100<<9); //SCL to alt
}

void i2c_release(){
    *gpfsel0 &= ~(0b100<<6); //SDA to input
	*gpfsel0 &= ~(0b100<<9); //SCL to input

    iounmap((void*)gpio_base);
    iounmap((void*)bsc1_base);
}

void i2c_setSlave(u_int8_t addr){
    *bsa1 = (*bsa & 0x8F) | (addr & 0x8F); //set slave address
}

int i2c_write_one(u_int8_t buf){
    return i2c_write(&buf, 1);
}

int i2c_write(u_int8_t* buf, int len){
    *bsc1 |= 0x30; //clear fifo
    *bsc1 &= ~(0x1); //set to write packet transfer

    *bss1 &= ~(0x302); // clear status

    *bsdlen1 = (*bsdlen1 & 0xFFFF) | (len & 0xFFFF); //set length

    int i = 0;
    while(i<len && *bss1&(0x10)){ //fill buffer before sending
        *bsfifo1 = (*bsfifo1 & 0xFF) & buf[i];
        i++;
    }

    *bsc1 |= 0x8080; //start transfer


    while(!(*bss1 & 0x2)){//while not done
        while(i<len && *bss1&(0x10)){ //fill the rest into buffer
            *bsfifo1 = (*bsfifo1 & 0xFF) & buf[i];
            i++;
        }
    }

    if(*bss1 & 0x200){ //clock stretch timeout
        printk(KERN_ALERT "I2C Write - Clock Stretch Timeout\n");
        return -1;
    }
    else if (*bss1 & 0x100){ //ACK error
        printk(KERN_ALERT "I2C Write - No Slave Acknowledged the Address\n");
        return -1;
    }
    else if (i<len){//part of data wasn't sent
        printk(KERN_ALERT "I2C Write - Part of Data Not Sent\n");
        return -1;
    }

    *bss1 |= 0x2; //set done

    return 0;
}

bool i2c_read(u_int8_t* buf, int len){
    *bsc1 |= 0x30; //clear fifo
    *bsc1 |= 0x1; //set to read packet transfer

    *bss1 &= ~(0x302); // clear status

    *bsdlen1 = (*bsdlen1 & 0xFFFF) | (len & 0xFFFF); //set length

    *bsc1 |= 0x8081; //start transfer

    while(!(*bss1 & 0x2)){//while not done
        while(i<len && *bss1&(0x20)){ //empty the buffer
            buf[i] = *bsfifo1 | 0xFF;
            i++;
        }
    }

    while(i<len && *bss1&(0x20)){ //empty rest of the buffer
        buf[i] = *bsfifo1 | 0xFF;
        i++;
    }

    if(*bss1 & 0x200){ //clock stretch timeout
        printk(KERN_ALERT "I2C Read - Clock Stretch Timeout\n");
        return false;
    }
    else if (*bss1 & 0x100){ //ACK error
        printk(KERN_ALERT "I2C Read - No Slave Acknowledged the Address\n");
        return false;
    }
    else if (i<len){//part of data wasn't received
        printk(KERN_ALERT "I2C Read - Part of Data Not Received\n");
        return false;
    }

    *bss1 |= 0x2; //set done

    return true;
}
