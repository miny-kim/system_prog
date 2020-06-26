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

#define DUST_MAJOR_NUMBER 505
#define DUST_DEV_NAME "finedust"

#define GPIO_BASE_ADDR 0x3F200000
#define SPI0_BASE_ADDR 0x3F204000

#define GPFSEL0 0x00
#define GPFSEL1 0x04
#define GPFSEL2 0x08
#define GPSET0 0x1C
#define GPCLR0 0x28
#define GPLEV0 0x034


// Defines for SPI
// GPIO register offsets from BCM2835_SPI0_BASE.
// Offsets into the SPI Peripheral block in bytes per 10.5 SPI Register Map
#define SPI0_CS                      0x0000 ///< SPI Master Control and Status
#define SPI0_FIFO                    0x0004 ///< SPI Master TX and RX FIFOs
#define SPI0_CLK                     0x0008 ///< SPI Master Clock Divider
#define SPI0_DLEN                    0x000c ///< SPI Master Data Length
#define SPI0_LTOH                    0x0010 ///< SPI LOSSI mode TOH
#define SPI0_DC                      0x0014 ///< SPI DMA DREQ Controls

#define SPI0_CS_CSPOL0               0x200000 ///< Chip Select 0 Polarity
#define SPI0_CS_CLEAR                0x00000030 ///< Clear FIFO Clear RX and TX
#define SPI0_CS_CPOL                 0x00000008 ///< Clock Polarity
#define SPI0_CS_CPHA                 0x00000004 ///< Clock Phase
#define SPI0_CS_CS                   0x00000003 ///< Chip Select
#define SPI0_CS_TA                   0x00000080 ///< Transfer Active
#define SPI0_CS_TXD                  0x00040000 ///< TXD TX FIFO can accept Data
#define SPI0_CS_RXD                  0x00020000 ///< RXD RX FIFO contains Data
#define SPI0_CS_RX_MASK              0x0003FF
#define SPI0_CS_DONE                 0x00010000 ///< Done transfer Done

static void __iomem * gpio_base;
static void __iomem * spi0_base;

static uint8_t mode;
static uint8_t bits = 8;
static uint32_t speed = 500000;
static uint16_t delay;

volatile unsigned int * gpfsel0;
volatile unsigned int * gpfsel1;
volatile unsigned int * gpfsel2;
volatile unsigned int * gpfset0;
volatile unsigned int * gpfclr0;
volatile unsigned int * gpflev0;
volatile unsigned int * spi0_cs;
volatile unsigned int * spi0_fifo;


/************************* spi *************************/

void bcm2835_peri_set_bits(volatile uint32_t* paddr, uint32_t value, uint32_t mask)
{
    uint32_t v = *paddr;
    v = (v & ~mask) | (value & mask);
    *paddr = v;
}

void spi_setClockDivider(uint16_t divider)
{
    volatile uint32_t* paddr = spi0_cs;
    *paddr = divider;
}

void spi_setDataMode(uint8_t mode) // cpol, cpha(clock phase)
{
    volatile uint32_t* paddr = spi0_cs;
    // Mask in the CPO and CPHA bits of CS
    bcm2835_peri_set_bits(paddr, mode << 2, (SPI0_CS_CPOL | SPI0_CS_CPHA));
}

void spi_chipSelect(uint8_t cs) // cs0 or cs1
{
    volatile uint32_t* paddr = spi0_cs;
    // Mask in the CS bits of CS
    bcm2835_peri_set_bits(paddr, cs, SPI0_CS_CS);
}

void spi_setChipSelectPolarity(uint8_t cs, uint8_t active)
{
    volatile uint32_t* paddr = spi0_cs;
    uint8_t shift = 21 + cs;
    // Mask in the appropriate CSPOLn bit
    bcm2835_peri_set_bits(paddr, active << shift, 1 << shift);
}

void spi_begin(void){
    
    spi0_cs = (volatile unsigned int *)(spi0_base + SPI0_CS);
    spi0_fifo = (volatile unsigned int *)(spi0_base + SPI0_FIFO);
    
    *spi0_cs &= 0;
    *spi0_cs = SPI0_CS_CLEAR;
    
    spi_setClockDivider(32);
    spi_setDataMode(0); //cpol = 0, cpha = 0
    spi_chipSelect(0); //using cs0
    spi_setChipSelectPolarity(0, 0);
    
    printk(KERN_ALERT "spi init!!\n");
    
}


void spi_transfernb(char* tbuf, char* rbuf, uint32_t len)
{
    volatile uint32_t* paddr = spi0_cs;
    volatile uint32_t* fifo = spi0_fifo;
    uint32_t TXCnt=0;
    uint32_t RXCnt=0;
    
    // This is Polled transfer as per section 10.6.1
    
    // Clear TX and RX fifos
    bcm2835_peri_set_bits(paddr, SPI0_CS_CLEAR, SPI0_CS_CLEAR);
    
    // Set TA = 1
    bcm2835_peri_set_bits(paddr, SPI0_CS_TA, SPI0_CS_TA);
    
    // Use the FIFO's to reduce the interbyte times
    while((TXCnt < len)||(RXCnt < len))
    {
        // TX fifo not full, so add some more bytes
        while(((*paddr) & SPI0_CS_TXD)&&(TXCnt < len ))
        {
            *fifo = tbuf[TXCnt];
            TXCnt++;
        }
        //Rx fifo not empty, so get the next received bytes
        while(((*paddr) & SPI0_CS_RXD)&&( RXCnt < len ))
        {
            rbuf[RXCnt] = *fifo;
            RXCnt++;
        }
    }
    // Wait for DONE to be set
    while (!((*paddr) & SPI0_CS_DONE))
        ;
    
    // Set TA = 0, and also set the barrier
    bcm2835_peri_set_bits(paddr, 0, SPI0_CS_TA);
    
    printk(KERN_ALERT "spi trasnfer!!\n");
}

/************************* end of spi *************************/

int dust_open(struct inode * inode, struct file * filp){
    printk(KERN_ALERT "dust driver open!!\n");
    
    
    gpio_base = ioremap(GPIO_BASE_ADDR, 0x18);
    spi0_base = ioremap(SPI0_BASE_ADDR, 0x20);
    
    gpfsel0 = (volatile unsigned int *)(gpio_base + GPFSEL0);
    gpfsel1 = (volatile unsigned int *)(gpio_base + GPFSEL1);
    gpfsel2 = (volatile unsigned int *)(gpio_base + GPFSEL2);
    gpfset0 = (volatile unsigned int *)(gpio_base + GPSET0);
    gpfclr0 = (volatile unsigned int *)(gpio_base + GPCLR0);
    gpflev0 = (volatile unsigned int *)(gpio_base + GPLEV0);

    *gpfsel0 |= (7<<(3*9)); //GPIO9
    *gpfsel0 &= ~(3<<(3*9)); //ALT0 -> MISO
    
    *gpfsel1 |= (7); //GPIO10
    *gpfsel1 &= ~(3); //ALT0 -> MOSI
    
    *gpfsel1 |= (7<<3); //GPIO11
    *gpfsel1 &= ~(3<<3); //ALT0 -> CLK
    
    *gpfsel0 |= (7<<24); //GPIO8
    *gpfsel0 &= ~(3<<24); //ALT0 -> CE0

    *gpfsel2 |= (1<<0); //out


    
    spi_begin();
    
    
    return 0;
}

int dust_release(struct inode *inode, struct file *filp){
    
    printk(KERN_ALERT "dust driver closed!!\n");
    
     *gpfsel0 |= (7<<(3*9)); 
    *gpfsel0 &= ~(7<<(3*9)); 

    *gpfsel1 |= (7); 
    *gpfsel1 &= ~(7); 

    *gpfsel1 |= (7<<3); 
    *gpfsel1 &= ~(7<<3); 

    *gpfsel0 |= (7<<24); 
    *gpfsel0 &= ~(7<<24); 

    *gpfsel0 |= (7<<21); 
    *gpfsel0 &= ~(7<<21); 
    
    iounmap((void*)gpio_base);
    iounmap((void*)spi0_base);
    
    return 0;
}


ssize_t dust_read(struct file *filp, char * buf, size_t count, loff_t *f_pos){
    printk(KERN_ALERT "dust read function called\n");
    
    int channel = 2;
    unsigned char tbuf[3];
    unsigned char rbuf[3];
    unsigned int aValue = 0;

    tbuf[0] = 0x06;
    tbuf[1] = (channel & 0x07) << 6;
    tbuf[2] = 0x00;

    *gpfset0 = (1<<20);
    msleep(280);
    //msleep(200);
    spi_transfernb(tbuf, rbuf, 3);
    msleep(20);
    *gpfclr0 = (1<<20);
    msleep(200);

    
     aValue = (((rbuf[1] & 0x0F) << 8) | rbuf[2]);

    printk(KERN_ALERT "adc transfer : %d!!\n", aValue);
   
    
    copy_to_user(buf, &aValue, sizeof(float));
    
    return count;
    
}

static struct file_operations dust_fops = {
    .owner = THIS_MODULE,
    .open = dust_open,
    .read = dust_read,
    .release = dust_release,
};

int __init dust_init(void){
    if(register_chrdev(DUST_MAJOR_NUMBER, DUST_DEV_NAME, &dust_fops)<0)
        printk(KERN_ALERT "dust driver initialization fail\n");
    else
        printk(KERN_ALERT "dust driver initialization success\n");
    
    return 0;
}

void __exit dust_exit(void){
    unregister_chrdev(DUST_MAJOR_NUMBER, DUST_DEV_NAME);
    printk(KERN_ALERT "dust driver exit done\n");
}

module_init(dust_init);
module_exit(dust_exit);

MODULE_AUTHOR("Minyeong");