#
# 		Makefile
# To create shared modules or executable in the inViseo Node and Server.
# This makefile is also the same for each sub-directory in the project
# folder. We only have to modify few lines to make it work for each different
# modules.
#
# Invoke like:
#    make force RELEASE=yes
# will rebuild the target w/o the debug symble
#
# Author: Thomas Chang
# Date: 2004-07-07
#
#  
SHELL = /bin/sh

TARGET = libutils.so
TARGETTYPE = shared
#VERSION = .1.0.0
INSTALLDIR = /node/system/modules

SOURCE_C = $(wildcard *.c)
SOURCE_H = $(wildcard *.h)
SOURCE_A = $(wildcard *.asm)
ALLSOURCE = $(SOURCE_C) $(SOURCE_A) $(SOURCE_H)
OBJECTS = $(SOURCE_C:.c=.o) $(SOURCE_A:.asm=.o)

# modify following line if you need more libraries
#LIBS = -lnodecfg -lsrvutils -lpthread
.PHONY: arm pc
ARCH:=
ifeq ($(ARCH), arm)
	MARCH=arm-hisiv300-linux-
else
	MARCH=
endif

CC = $(MARCH)gcc
LD = $(MARCH)gcc
NASM = $(MARCH)nasm

LIBS = -lpthread
#LIBDIR = -L../lib
LIBDIR = -L./lib
INCLUDES = -I. -I../include

#CC = $(MARCH)gcc
#CC = gcc
ifeq ($(RELEASE), yes)
CFLAGS = -c -O3 -Wall -fPIC -DENABLE_LOG
else
CFLAGS = -c -g -Wall -fPIC -DENABLE_LOG
endif

#NASM = $(MARCH)nasm
ASMFLAGS = -f elf
#LD = $(MARCH)gcc
LDFLAGS = -shared

ifeq ($(TARGETTYPE), executable)
LINK = $(CC)
LINKOPTIONS = $(ULDFLAGS)
else
LINK = $(LD)
LINKOPTIONS = $(LDFLAGS) $(ULDFLAGS)
endif

.SUFFIXES:			# cancel all built-in suffix
.SUFFIXES:	.c .o .h .asm	# we only dealing with these types of file
.PHONY:		install clean force u2d

$(TARGET):	$(OBJECTS)
	$(LINK) $(LINKOPTIONS) -o $@ $(OBJECTS) $(LIBDIR) $(LIBS)

# bad idea as any one of .h update will fource all .c file to be the gaol
# to make. Just a rush out solution.
$(SOURCE_C): $(SOURCE_H)
	@touch $@

u2d:
	@touch $(SOURCE_C) $(SOURCE_A)

force: u2d $(TARGET)

clean:
	@rm *.o 
	@rm -f lib*

.c.o:
	$(CC) $(CFLAGS) $(UCFLAGS) $(INCLUDES) $<

.asm.o:
	$(NASM) $(ASMFLAGS) $<

install:
	cp $(TARGET) $(INSTALLDIR)/$(TARGET)$(VERSION) -f
