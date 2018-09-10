KDIR := /home/raja/Documents/linux-toradex-imx6
CROSS_COMPILE := /home/raja/Downloads/gcc-linaro/bin/arm-linux-gnueabihf-
obj-m := gpio-usmap.o
PWD := $(shell pwd)
EXTRA_CFLAGS=-O2

all: gpio-usmap.c
	$(MAKE) -C $(KDIR) M=$(PWD) modules CFLAGS=$(CFLAGS) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE)
	
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE)
	
