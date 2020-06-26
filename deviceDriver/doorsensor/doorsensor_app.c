#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>

#define DOOR_MAJOR_NUMBER 480
#define DOOR_MINOR_NUMBER 105
#define DOOR_DEV_PATH_NAME "doorsensor_dev"
#define DOOR_MAGIC_NUMBER 'l'

#define DOOR_START _IOW(DOOR_MAGIC_NUMBER, 0, unsigned int)
#define DOOR_GET_STATE _IOR(DOOR_MAGIC_NUMBER, 1, int)

int main(void)
{
  dev_t doorsensor_dev = 0;
  int door_fd = 0;
  
  volatile int door_state = 0;
  volatile int new_door_state = 0;
  
  doorsensor_dev = makedev(DOOR_MAJOR_NUMBER, DOOR_MINOR_NUMBER);
  
  if (mknod(DOOR_DEV_PATH_NAME, S_IFCHR|0600, doorsensor_dev)<0)
  {
    fprintf(stderr, "%d\n", errno);
  }
  
  door_fd = open(DOOR_DEV_PATH_NAME, O_RDWR);
  
  if (door_fd < 0)
  {
    printf("fail to open doorsensor_dev\n");
    
    return -1;
  }
  
  ioctl(door_fd, DOOR_START);
  
  while (1)
  {
    ioctl(door_fd, DOOR_GET_STATE, &new_door_state);
    
    if (door_state != new_door_state)
    {
      if (door_state == 0)
      {
        printf("Door Closed\n");
      }
      else
      {
        printf("Door Opened\n");
      }
    }
    
    door_state = new_door_state;
    
    usleep(1 * 1000);
  }
  
  close(door_fd);
  
  return 0;
}
