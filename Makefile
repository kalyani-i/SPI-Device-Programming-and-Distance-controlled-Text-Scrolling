CROSS_COMPILE = arm-linux-gnueabihf-
ARCH=arm

APP = assignment3

obj-m:= ht438_drv.o

KDIR=/lib/modules/$(shell uname -r)/build

ccflags-m += -Wall

all:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(PWD) modules
	gcc -Wall -o $(APP) assignment3.c -lpthread

clean:
	rm -f *.ko
	rm -f *.o
	rm -f Module.symvers
	rm -f modules.order
	rm -f *.mod.c
	rm -rf .tmp_versions
	rm -f *.mod.c
	rm -f *.mod.o
	rm -f \.*.cmd
	rm -f *.mod
	rm -f Module.markers
	rm -f $(APP)