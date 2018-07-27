SHELL = /bin/sh

TARGET = gpio_test_3
CC = arm-hisiv100nptl-linux-gcc
#CC = gcc


OBJS = main.o



$(TARGET):$(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(CFLAGS) -lpthread


.PHONY: clean
clean:
	-rm $(TARGET) $(OBJS)


