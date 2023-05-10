PROJECTROOT=/home/student/pputvios1/toolchain

SDK_ROOT=$(PROJECTROOT)/marvell/marvell-sdk-1046
SDK_ROOTFS=$(SDK_ROOT)/rootfs
SDK_GALOIS=$(SDK_ROOTFS)/home/galois/include
SDK_INCLUDE=$(SDK_ROOTFS)/usr/include/ 

TOOLCHAIN_CROSS_COMPILE=$(PROJECTROOT)/marvell/armv5-marvell-linux-gnueabi-softfp/bin/arm-marvell-linux-gnueabi

CROSS_COMPILE=$(TOOLCHAIN_CROSS_COMPILE)

CC_PREFIX=$(CROSS_COMPILE)-
CC=$(CC_PREFIX)gcc
CXX=$(CC_PREFIX)g++
LD=$(CC_PREFIX)ld
ROOTFS_PATH=$(SDK_ROOTFS)

CFLAGS= -I. \
        -I./include/ \
        -I$(ROOTFS_PATH)/usr/include/ \
        -I$(ROOTFS_PATH)/usr/include/directfb/
        
CFLAGS += -D__LINUX__ -O0 -Wno-psabi 

CXXFLAGS = $(CFLAGS)

LIBS_PATH += -L$(ROOTFS_PATH)/home/galois/lib/directfb-1.4-6-libs

LIBS := $(LIBS_PATH) -ldirectfb -ldirect -lfusion -lrt

SRC= vezba_4.c

all: dfb_example

dfb_example:
	$(CC) -o vezba_4 $(SRC) $(CFLAGS) $(LIBS)
	cp vezba_4 /home/student/pputvios1/ploca/
	cp dfblogo_alpha.png /home/student/pputvios1/ploca/
	cp krug.png /home/student/pputvios1/ploca/

clean:
	rm -f vezba_4
