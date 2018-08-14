#主版本号
MAJOR_VERSION=0
#次版本号
PATCHLEVEL=0
#子版本号
SUBLEVEL=1
NAME= Sneaky Weasel
PLATFORM=CHIP_3520
#PLATFORM=CHIP_3520

TIME_YEAR=TIME_YEAR$(shell date "+%Y")
TIME_MON=TIME_MON$(shell date "+%m")
TIME_DAY=TIME_DAY$(shell date "+%d")

TARGET=simserver
ReleaseFile=./Release/$(TARGET)-$(MAJOR_VERSION)-$(PATCHLEVEL)-$(SUBLEVEL)

SRC=$(wildcard *.c)
SRC+=$(wildcard ./utils/*.c)

OBJ=$(SRC:%.c=%.o)
# add libs here
LD_LIB=-L. -lpthread -lrt
# add 
HEAD_DIR=-I../utils -g -Wall
TIME_FLAGS=
CC=arm-hisiv100nptl-linux-gcc

all: $(TARGET)
	
$(TARGET) : $(OBJ)
	$(CC) -o $@ $^ $(LD_LIB)
	@echo $(ReleaseFile)
	@cp -vf $(TARGET) $(ReleaseFile)
	
$(OBJ) : %.o:%.c
	$(CC) $(HEAD_DIR) $(TIME_FLAGS) -D$(PLATFORM) -o $@ -c $^
	
cc:
	@rm $(OBJ)
	@rm $(TARGET)
	
	


