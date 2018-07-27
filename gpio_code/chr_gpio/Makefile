
CC = arm-hisiv100nptl-linux-gcc
KERNELDIR:=/work/tools/Hi3520D_SDK_V1.0.5.0/osdrv/kernel/linux-3.0.y
PWD := $(shell pwd)


obj-m:=chr_gpio.o

modules:
	@$(MAKE) -C $(KERNELDIR) M=$(PWD) modules ARCH=arm CROSS_COMPILE=arm-hisiv100nptl-linux-


clean:
	@rm -rf *.o *.ko
