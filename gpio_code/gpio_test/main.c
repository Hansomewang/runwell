#include<sys/types.h>
#include <stdio.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<sys/ioctl.h> 
#include <time.h>

#include "gpio.h"
 

typedef struct GPIO_DATA
{
    Uint8 gp_out;
    Uint8 gp_in;
}Sgpio;

Sgpio gpio_data;
int main()
{
	int fd;
    int ret;

    GpioS *arg = (GpioS *)malloc(sizeof(GpioS));
    GpioS *arg_0 = (GpioS *)malloc(sizeof(GpioS));
    GpioS *arg_1 = (GpioS *)malloc(sizeof(GpioS));
    GpioS *arg_2 = (GpioS *)malloc(sizeof(GpioS));
    GpioS *arg_3 = (GpioS *)malloc(sizeof(GpioS));
	
    arg_0->do_pin = 0;
    arg_0->output_val = 0xff;
    arg_0->do_period = 100;

    arg_1->do_pin = 1;
    arg_1->output_val = 0xff;
    arg_1->do_period = 200;

    arg_2->do_pin = 2;
    arg_2->output_val = 0xff;
    arg_2->do_period = 500;

    arg_3->do_pin = 3;
    arg_3->output_val = 0xff;
    arg_3->do_period = 1000;

	fd = open("/dev/cdev_gpio", O_RDWR);//可读可写打开文件
    printf("fd = %d\n",fd);
    if(fd == -1 )
    {
        printf("open /dev/cdev_gpio filed!\n");
    }
	
    while(1)
    {
/*
	    ioctl(fd, CMD_SET_GPIO3_0, 0xff);
	    ioctl(fd, CMD_SET_GPIO3_1, 0xff);
	    ioctl(fd, CMD_SET_GPIO3_2, 0xff);
	    ioctl(fd, CMD_SET_GPIO3_3, 0xff);
*/
        //ioctl(fd,CMD_SET_VALUE,arg_0);
        //ioctl(fd,CMD_SET_VALUE,arg_1);
        //ioctl(fd,CMD_SET_VALUE,arg_2);
        ioctl(fd,CMD_SET_VALUE,arg_3);

        ioctl(fd,CMD_GET_VALUE,arg);
        printf("di_value = 0x%x\n",arg->di_value);
        printf("do_value = 0x%x\n",arg->do_value);

        sleep(2);
/*
	    ioctl(fd, CMD_SET_GPIO3_0, 0);
	    ioctl(fd, CMD_SET_GPIO3_1, 0);
	    ioctl(fd, CMD_SET_GPIO3_2, 0);
	    ioctl(fd, CMD_SET_GPIO3_3, 0);
*/	
        ioctl(fd,CMD_GET_VALUE,arg);
        printf("di_value = 0x%x\n",arg->di_value);
        printf("do_value = 0x%x\n",arg->do_value);
        sleep(2);
    }
	
	close(fd);
}

