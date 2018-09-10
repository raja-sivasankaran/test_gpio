#EZSDK := /home/jj/ti-ezsdk_dm816x-evm_5_04_00_11
#CROSS_COMPILE := /home/raja/Downloads/gcc-linaro/bin/arm-linux-gnueabihf-

#include $(EZSDK)/Rules.make

#CFLAGS := -I$(EZSDK)/board-support/linux-2.6.37-psp04.04.00.01/arch/arm/plat-omap/include -Wall -march=armv7-a -O2
#LDFLAGS := -L$(EZSDK)/component-sources/gst-openmax_GST_DM81XX_00_05_00_00/ext/lib -lOMX_Core

CC := $(CROSS_COMPILE)gcc

default: gpio-client

gpio-client: gpio-client.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ gpio-client.c

clean:
	rm gpio-client

asm: gpio-client.c
	$(CC) $(CFLAGS) $(LDFLAGS) -S -o gpio-client.s gpio-client.c
