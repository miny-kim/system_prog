#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>

#define N20_MAJOR_NUMBER 510
#define N20_MINOR_NUMBER 100
#define N20_DEV_PATH_NAME "N20motor_dev"
#define N20_MAGIC_NUMBER 'k'

#define N20_START _IO(N20_MAGIC_NUMBER, 0)
#define N20_SPIN _IO(N20_MAGIC_NUMBER, 1)
#define N20_STOP _IO(N20_MAGIC_NUMBER, 2)

int main(void)
{
  dev_t n20motor_dev = 0;
  int n20_fd = 0;
  
  n20motor_dev = makedev(N20_MAJOR_NUMBER, N20_MINOR_NUMBER);
  
  if (mknod(N20_DEV_PATH_NAME, S_IFCHR, n20motor_dev)<0)
  {
    fprintf(stderr, "%d\n", errno);
  }
  
  n20_fd = open(N20_DEV_PATH_NAME, O_RDWR);
  
  if (n20_fd < 0)
  {
    printf("fail to open n20 motor_dev\n");
    
    return -1;
  }
  
  ioctl(n20_fd, N20_START);
  
  while (1)
  {
    ioctl(n20_fd, N20_SPIN);
    
    usleep(100 * 1000);
  }
  
  close(n20_fd);
  
  return 0;
}
