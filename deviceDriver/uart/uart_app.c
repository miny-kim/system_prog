#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/select.h>

#include <stdio.h>
#include <sys/select.h>
#include <termios.h>
#include <stropts.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define KBHIT_ENTER 10
#define STRING_LENGTH 128
#define UART_MINOR_NUMBER 100
#define UART_MAJOR_NUMBER 502
#define UART_DEV_NAME "uart_ioctl"
#define UART_DEV_PATH_NAME "/dev/uart_ioctl"

#define IOCTL_MAGIC_NUMBER 'u'
#define IOCTL_CMD_TRANSMIT _IOWR(IOCTL_MAGIC_NUMBER, 0, int)
#define IOCTL_CMD_RECEIVE _IOWR(IOCTL_MAGIC_NUMBER, 1, int)


int _kbhit() {
    static const int STDIN = 0;
    static int initialized = 0;
    
    if (! initialized) {
        // Use termios to turn off line buffering
        struct termios term;
        tcgetattr(STDIN, &term);
        term.c_lflag &= ~ICANON;
        tcsetattr(STDIN, TCSANOW, &term);
        setbuf(stdin, NULL);
        initialized = 1;
    }
    
    int bytesWaiting;
    ioctl(STDIN, FIONREAD, &bytesWaiting);
    return bytesWaiting;
}

int main()
{
    dev_t uart_dev;
    int fd;
    int i = 0;
    int string_index = 0;
    int b_async_input = 0;
    char char_temp;
    char string[STRING_LENGTH];
    long result;
    
    uart_dev = makedev(UART_MAJOR_NUMBER, UART_MINOR_NUMBER);
    mknod(UART_DEV_PATH_NAME, S_IFCHR|0666, uart_dev);
    
    fd = open(UART_DEV_PATH_NAME, O_RDWR | O_NOCTTY | O_NDELAY); //Open in non blocking read/write mode
    
    if(fd <0)
    {
        printf("fail to open\n");
        return -1;
    }
    
    while(1)
    {
        b_async_input = _kbhit();
        if(b_async_input > 0)
        {
            char_temp = getchar();
            if(char_temp == KBHIT_ENTER)
            {
                for(i = 0 ; i < string_index; i++)
                {
                    ioctl(fd, IOCTL_CMD_TRANSMIT, &string[i]);
                    usleep(100);
                }
                string_index = 0;
            }
            else
            {
                string[string_index++] = char_temp;
            }
        }
        
        result = ioctl(fd, IOCTL_CMD_RECEIVE, NULL);
        if(result >= 0 )
        {
            printf(ANSI_COLOR_GREEN);
            printf("%c", result);
            printf(ANSI_COLOR_RESET);
            fflush(stdout);
        }
        
    }
    
    close(fd);
    return 0;
}
