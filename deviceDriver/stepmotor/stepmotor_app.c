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

int main(void)
{
  dev_t stepmotor_dev = 0;
  int step_fd = 0;
  
  stepmotor_dev = makedev(STEP_MAJOR_NUMBER, STEP_MINOR_NUMBER);
  
  if (mknod(STEP_DEV_PATH_NAME, S_IFCHR, stepmotor_dev)<0)
  {
    fprintf(stderr, "%d\n", errno);
  }
  
  step_fd = open(STEP_DEV_PATH_NAME, O_RDWR);
  
  if (step_fd < 0)
  {
    printf("fail to open stepmotor_dev\n");
    
    return -1;
  }
  
  ioctl(step_fd, STEP_START);
  
  while (1)
  {
    ioctl(step_fd, STEP_SPIN);
    //ioctl(step_fd, STEP_REWIND);
    
    usleep(5 * 1000);
  }
  
  close(step_fd);
  
  return 0;
}
