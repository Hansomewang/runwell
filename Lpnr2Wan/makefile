SHELL = /bin/sh

TARGET = demo
CFLAGS = -I. -I./lpnr -I./utils
#CC = arm-hisiv100nptl-linux-gcc
CC = gcc


OBJS = demo.o lpnr/rwlpnrapi.o utils/utils_net.o 



$(TARGET):$(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(CFLAGS) -lpthread


.PHONY: clean
clean:
	-rm $(TARGET) $(OBJS)


