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

SYSROOT=$(SDK_ROOTFS)
GALOIS_INCLUDE=$(SDK_GALOIS)

INCS =	-I$(PROJECTROOT)/tdp_api
INCS += -I./include/ 							\
		-I$(SYSROOT)/usr/include/         \
		-I$(GALOIS_INCLUDE)/Common/include/     \
		-I$(GALOIS_INCLUDE)/OSAL/include/		\
		-I$(GALOIS_INCLUDE)/OSAL/include/CPU1/	\
		-I$(GALOIS_INCLUDE)/PE/Common/include/

LIBS_PATH = -L$(PROJECTROOT)/tdp_api

LIBS_PATH += -L$(SYSROOT)/home/galois/lib/

LIBS := $(LIBS_PATH) -ltdp

LIBS += $(LIBS_PATH) -lOSAL	-lshm -lPEAgent

CFLAGS += -D__LINUX__ -O0 -Wno-psabi --sysroot=$(SYSROOT)

CXXFLAGS = $(CFLAGS)

all: parser_playback_sample

SRCS =  ./vezba_3.c

parser_playback_sample:
	$(CC) -o vezba_3 $(INCS) $(SRCS) $(CFLAGS) $(LIBS)
	cp vezba_3 /home/student/pputvios1/ploca/
    
clean:
	rm -f vezba_3
