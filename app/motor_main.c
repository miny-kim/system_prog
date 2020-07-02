#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>

#define STEP_MAJOR_NUMBER 508
#define STEP_MINOR_NUMBER 103
#define STEP_DEV_PATH_NAME "stepmotor_dev"
#define STEP_MAGIC_NUMBER 'j'

#define STEP_START _IO(STEP_MAGIC_NUMBER, 0)
#define STEP_SPIN _IO(STEP_MAGIC_NUMBER, 1)
#define STEP_REWIND _IO(STEP_MAGIC_NUMBER, 2)
#define STEP_STOP _IO(STEP_MAGIC_NUMBER, 3)

#define N20_MAJOR_NUMBER 510
#define N20_MINOR_NUMBER 100
#define N20_DEV_PATH_NAME "N20motor_dev"
#define N20_MAGIC_NUMBER 'k'

#define N20_START _IO(N20_MAGIC_NUMBER, 0)
#define N20_SPIN _IO(N20_MAGIC_NUMBER, 1)
#define N20_STOP _IO(N20_MAGIC_NUMBER, 2)

#define DOOR_MAJOR_NUMBER 480
#define DOOR_MINOR_NUMBER 105
#define DOOR_DEV_PATH_NAME "doorsensor_dev"
#define DOOR_MAGIC_NUMBER 'l'

#define DOOR_START _IOW(DOOR_MAGIC_NUMBER, 0, unsigned int)
#define DOOR_GET_STATE _IOR(DOOR_MAGIC_NUMBER, 1, int)

#define DOOR_OPENED 0
#define DOOR_CLOSED 1

#define UART_MAJOR_NUMBER	502
#define UART_MINOR_NUMBER	100
#define UART_DEV_PATH_NAME	"uart_ioctl"
#define IOCTL_MAGIC_NUMBER	'u'

#define IOCTL_CMD_TRANSMIT	_IOWR(IOCTL_MAGIC_NUMBER, 0, int)
#define IOCTL_CMD_RECEIVE	_IOWR(IOCTL_MAGIC_NUMBER, 1, int)

#define INPUT_CMD_CLOSE	 '0'
#define INPUT_CMD_OPEN	 '1'
#define INPUT_CMD_TOGGLE '2'

#define CMD_DEFAULT 0
#define CMD_OPEN	1
#define CMD_CLOSE	2
#define CMD_STOP	3

int main(void)
{
	long cmd = CMD_DEFAULT;
	
	const int _step_timer = 5;
	
	int step_timer = 0;
	
	unsigned int step_cmd = STEP_STOP;
	
	dev_t stepmotor_dev = 0;
	int step_fd = 0;
  
	dev_t n20motor_dev = 0;
	int n20_fd = 0;
  
	dev_t doorsensor_dev = 0;
	int door_fd = 0;
	
	dev_t uart_dev = 0;
   	int uart_fd = 0;
  
	volatile int door_state = DOOR_CLOSED;
	volatile int new_door_state = DOOR_CLOSED;
  
	stepmotor_dev = makedev(STEP_MAJOR_NUMBER, STEP_MINOR_NUMBER);	
	n20motor_dev = makedev(N20_MAJOR_NUMBER, N20_MINOR_NUMBER);
	doorsensor_dev = makedev(DOOR_MAJOR_NUMBER, DOOR_MINOR_NUMBER);
	uart_dev = makedev(UART_MAJOR_NUMBER, UART_MINOR_NUMBER);
  
	if (mknod(STEP_DEV_PATH_NAME, S_IFCHR, stepmotor_dev)<0)
	{
		fprintf(stderr, "%d\n", errno);
	}
	if (mknod(N20_DEV_PATH_NAME, S_IFCHR, n20motor_dev)<0)
	{
		fprintf(stderr, "%d\n", errno);
	}
	if (mknod(DOOR_DEV_PATH_NAME, S_IFCHR|0600, doorsensor_dev)<0)
	{
		fprintf(stderr, "%d\n", errno);
	}
   	if(mknod(UART_DEV_PATH_NAME, S_IFCHR|0666, uart_dev)<0)
   	{
		fprintf(stderr, "%d\n",errno);
	}
  
	step_fd = open(STEP_DEV_PATH_NAME, O_RDWR);
	n20_fd = open(N20_DEV_PATH_NAME, O_RDWR);
	door_fd = open(DOOR_DEV_PATH_NAME, O_RDWR);
   	uart_fd = open(UART_DEV_PATH_NAME, O_RDWR | O_NOCTTY | O_NDELAY);
   	
	if (step_fd < 0)
	{
		printf("fail to open stepmotor_dev\n");
    
		return -1;
	}
	if (n20_fd < 0)
	{
		printf("fail to open n20 motor_dev\n");
    
		return -2;
	}
	if (door_fd < 0)
	{
		printf("fail to open doorsensor_dev\n");
    
		return -3;
	}
	if(uart_fd <0)
	{
      	printf("fail to open uart\n");
    	return -4;
   	}
  
	ioctl(step_fd, STEP_START);
	ioctl(n20_fd, N20_START);
	ioctl(door_fd, DOOR_START);
  
	while (1)
	{
		char temp = ioctl(uart_fd, IOCTL_CMD_RECEIVE, NULL);
		//char temp = INPUT_CMD_TOGGLE;	//for the test
		
		switch(temp)
		{
			case INPUT_CMD_CLOSE:
				cmd = CMD_CLOSE;
				
				break;
			case INPUT_CMD_OPEN:
				cmd = CMD_OPEN;
				break;
			case INPUT_CMD_TOGGLE:
				if (door_state == DOOR_CLOSED)
				{
					cmd = CMD_OPEN;
				}
				else
				{
					cmd = CMD_CLOSE;
				}
				break;
		}
		
		ioctl(door_fd, DOOR_GET_STATE, &new_door_state);
    
		if (door_state != new_door_state)
		{
			if (door_state == DOOR_OPENED)
			{
				printf("Door Closed\n");
				
				cmd = CMD_STOP;
			}
			else
			{
				printf("Door Opened\n");
			}
			
			door_state = new_door_state;
		}
		
		switch (cmd)
		{
			case CMD_OPEN:
				step_cmd = STEP_SPIN;
				
				ioctl(n20_fd, N20_SPIN);
				break;
			case CMD_CLOSE:
				if (door_state == DOOR_CLOSED)
				{
					step_cmd = STEP_STOP;
				
					ioctl(step_fd, step_cmd);
					ioctl(n20_fd, N20_STOP);
				
					step_timer = 0;
					
					break;
				}
			
				step_cmd = STEP_REWIND;
				
				ioctl(n20_fd, N20_STOP);
				break;
			case CMD_STOP:
				step_cmd = STEP_STOP;
				
				ioctl(step_fd, step_cmd);
				ioctl(n20_fd, N20_STOP);
				
				step_timer = 0;
				break;
			default:
				break;
		}
		
		
		if (step_cmd == STEP_SPIN)
		{
			step_timer++;
		
			if (step_timer >= _step_timer)
			{
				ioctl(step_fd, STEP_SPIN);
				step_timer = 0;
			}
		}
		else if (step_cmd == STEP_REWIND)
		{
			step_timer++;
		
			if (step_timer >= _step_timer)
			{
				ioctl(step_fd, STEP_REWIND);
				step_timer = 0;
			}
		}
		
		usleep(1000);
	}
  
	close(step_fd);
	close(n20_fd);
	close(door_fd);
	close(uart_fd);
  
	return 0;
}
