#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>

#define LED_MAJOR_NUMBER 502
#define LED_MINOR_NUMBER 100
#define LED_DEV_PATH_NAME     "/dev/led_ioctl"

#define SWITCH_MAJOR_NUMBER 503
#define SWITCH_MINOR_NUMBER 101
#define SWITCH_DEV_PATH_NAME "/dev/switch_ioctl"

#define IOCTL_MAGIC_NUMBER 'j'
#define IOCTL_CMD_CHANGE_STATE     _IOWR(IOCTL_MAGIC_NUMBER, 0, int)

#define CO2_GREAT   300
#define CO2_GOOD    400
#define CO2_BAD 500

#define AIR_GREAT   400
#define AIR_GOOD    500
#define AIR_BAD 700


#define INTERVAL 500000

int main(void){
    
    dev_t led_dev, switch_dev;
    int fd_led, fd_switch;
    int current_switch_value = 0;
    int aValue;

    led_dev = makedev(LED_MAJOR_NUMBER, LED_MINOR_NUMBER);
    switch_dev = makedev(SWITCH_MAJOR_NUMBER, SWITCH_MINOR_NUMBER);
    mknod(LED_DEV_PATH_NAME, S_IFCHR|0666, led_dev);//make node
    mknod(SWITCH_DEV_PATH_NAME, S_IFCHR|0666, switch_dev);
    
    fd_led = open(LED_DEV_PATH_NAME, O_RDWR);
    fd_switch = open(SWITCH_DEV_PATH_NAME, O_RDWR);
    
    if(fd_led<0 || fd_switch<0){
        printf("fail to open\n");
        return -1;
    }

    while(1){
        usleep(INTERVAL);
        printf("checking...\n");

        read(fd_switch, &current_switch_value, sizeof(int));

        if(current_switch_value != 0){
            //use auto mode

            //co2 mode
            if(aValue <= CO2_GREAT)
                ioctl(fd_led, IOCTL_CMD_CHANGE_STATE, 0);
            else if(aValue <= CO2_GOOD)
                ioctl(fd_led, IOCTL_CMD_CHANGE_STATE, 1);
            else if(aValue <= CO2_BAD)
                ioctl(fd_led, IOCTL_CMD_CHANGE_STATE, 2);

            //air mode

       
        }else{
            //use manual? mode
        }
        
        
    }
    
    close(fd_led);
    close(fd_switch);
    
    return 0;
}

