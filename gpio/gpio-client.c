#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/io.h>
#include "../ti81xx-usmap/ti81xx-usmap.h"
//#include <gpio.h>

#define IMX6_GPIO_BASE 0x0209C000
#define IMX6_IOMUXX_BASE   0x020E0000
#define IMX6_GPIO_BANK_OFFSET 0x00004000

//6 * 0x400 = 
//SODIMM 127  is Gpio 38

/*
volatile IMX6_GPIO_BANK* bank;
pedef struct
{
    DWORD GPIO_DR;
    DWORD GPIO_GDIR;
    DWORD GPIO_PSR;
    DWORD GPIO_ICR1;
    DWORD GPIO_ICR2;
    DWORD GPIO_IMR;
    DWORD GPIO_ISR;
    DWORD GPIO_EDGE_SEL;
} IMX6_GPIO_BANK;
*/

#define IMX6_SO127_GPIO_DR      (IMX6_GPIO_BANK_OFFSET + 0)
#define IMX6_SO127_GPIO_GDIR    (IMX6_GPIO_BANK_OFFSET + 4)
#define IMX6_SO127_GPIO_PSR     (IMX6_GPIO_BANK_OFFSET + 8)
#define IMX6_SO127_GPIO_ICR1    (IMX6_GPIO_BANK_OFFSET + 0xC)
#define IMX6_SO127_GPIO_ICR2    (IMX6_GPIO_BANK_OFFSET + 0x10)
#define IMX6_SO127_GPIO_IMR     (IMX6_GPIO_BANK_OFFSET + 0x14)
#define IMX6_SO127_GPIO_ISR     (IMX6_GPIO_BANK_OFFSET + 0x18)
#define IMX6_SO127_GPIO_EDGE_SEL   (IMX6_GPIO_BANK_OFFSET + 0x1C)

#define BUFFER_MAX 50
int fd1 = 0;

int openGPIO(int pin, int direction);
int writeGPIO(int gpio, int value);

int openGPIO(int gpio, int direction)
{
	int len;
   	char buf[BUFFER_MAX];
    	if (direction < 0 || direction > 1)return -2;

 	fd1 = open("/sys/class/gpio/export", O_WRONLY);
    	len = snprintf(buf, BUFFER_MAX, "%d", gpio);
    	write(fd1, buf, len);
    	close(fd1);

    	len = snprintf(buf, BUFFER_MAX, "/sys/class/gpio/gpio%d/direction", gpio);
    	fd1 = open(buf, O_WRONLY);
    	if (direction == 1) {
        	write(fd1, "out", 4);
        	close(fd1);
        	len = snprintf(buf, BUFFER_MAX, "/sys/class/gpio/gpio%d/value", gpio);
        	fd1 = open(buf, O_WRONLY);
    	}
	else {
        	write(fd1, "in", 3);
        	close(fd1);
        	len = snprintf(buf, BUFFER_MAX, "/sys/class/gpio/gpio%d/value", gpio);
        	fd1 = open(buf, O_RDONLY);
    	}
    	return 0;
}

int writeGPIO(int gpio, int b)
{
    if (b == 0) {
        write(fd1, "0", 1);
    } else {
        write(fd1, "1", 1);
    }

    lseek(fd1, 0, SEEK_SET);
    return 0;
} 

int main(int argc, char** argv)
{
	int file, i;
   	ulong* pGpIoRegisters = NULL;
     	ulong pin = 1 << 6;
    	int gpio = 38;
  	char buf[100];
  	FILE* fd;
  	int len = 0;

  	/* Method 1 */
	file = open("/dev/ti81xx-usmapchar", O_RDWR | O_SYNC);
    	if (file < 0){
 		printf("open failed (%d)\n", errno);
 		return 1;
  	}
	printf("Square from kernel space...");
 	fflush(stdout);

	ioctl(file, TI81XX_USMAP_IOCTL_TEST_GPIO);

	printf(" done, Press any key to contine next method\n");
   	getchar();

        /* Methode 2 */
	ioctl(file, TI81XX_USMAP_IOCTL_NOCACHE);
   	pGpIoRegisters = mmap(NULL, 7 * 0x4000, PROT_READ | PROT_WRITE, MAP_SHARED, file, IMX6_GPIO_BASE);
   	printf("Square from user space (no cache, without msync)...");
   	fflush(stdout);

   	pGpIoRegisters[IMX6_SO127_GPIO_GDIR >> 2] |= pin;
   	for (i = 0; i < 30000000; ++i){
      		pGpIoRegisters[IMX6_SO127_GPIO_DR >> 2] = pin;
     		pGpIoRegisters[IMX6_SO127_GPIO_DR >> 2] &= ~pin;
   	}
   	pGpIoRegisters[IMX6_SO127_GPIO_GDIR >> 2] &= ~pin;

   	munmap(pGpIoRegisters, 7 * 0x4000);
	fflush(stdout);
	printf(" Done, press any key to continue next method\n");
	getchar();

	/* Methode 3 */
	ioctl(file, TI81XX_USMAP_IOCTL_CACHE);
   	pGpIoRegisters = mmap(NULL, 7 * 0x4000, PROT_READ | PROT_WRITE, MAP_SHARED, file, IMX6_GPIO_BASE);
   	printf("Square from user space (with cache, using msync)...");
   	fflush(stdout);

  	pGpIoRegisters[IMX6_SO127_GPIO_GDIR >> 2] |= pin;
   	msync(&pGpIoRegisters[IMX6_SO127_GPIO_GDIR >> 2], sizeof(ulong), MS_SYNC);

   	for (i = 0; i < 5000000; ++i){
      		pGpIoRegisters[IMX6_SO127_GPIO_DR >> 2] = pin;
      		msync(&pGpIoRegisters[IMX6_SO127_GPIO_DR >> 2], sizeof(ulong), MS_SYNC);
      		pGpIoRegisters[IMX6_SO127_GPIO_DR >> 2] &= ~pin;
      		msync(&pGpIoRegisters[IMX6_SO127_GPIO_DR >> 2], sizeof(ulong), MS_SYNC);
   	}

	pGpIoRegisters[IMX6_SO127_GPIO_GDIR >> 2] &= ~pin;
	msync(&pGpIoRegisters[IMX6_SO127_GPIO_GDIR >> 2], sizeof(ulong), MS_SYNC);

        munmap(pGpIoRegisters, 7 * 0x4000);
	printf(" done, Press any key to contine next method\n");
   	fflush(stdout);
        getchar(); 
   	close(file);

	/* Methode 4 */
        printf("Square from sysfs...");
    	fd = fopen("/sys/class/gpio/export", "w");
    	fprintf(fd, "%d", gpio);
    	fclose(fd);

    	sprintf(buf, "/sys/class/gpio/gpio%d/direction", gpio);
    	fd = fopen(buf, "w");
    	fprintf(fd, "out");
    	fclose(fd);

    	sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio);
    	fd = fopen(buf, "w");
    	fclose(fd);

    	for (i = 0; i < 5000 * 10; i++) {
        	fd = fopen(buf, "w");
        	fprintf(fd, "1");
        	fclose(fd);
        	fd = fopen(buf, "w");
        	fprintf(fd, "0");
        	fclose(fd);
    	}
	printf(" done, Press any key to contine next method\n");
   	getchar();

        /* methode 5 */
    	fd = fopen(buf, "w");
    	for (i = 0; i < 5000 * 10; i++) {
        	fprintf(fd, "1");
        	fflush(fd);
        	fprintf(fd, "0");
        	fflush(fd);
   	}
    	fclose(fd); 

	fd = fopen("/sys/class/gpio/unexport", "w");
    	fprintf(fd, "%d", gpio);
    	fclose(fd);

	printf(" done, Press any key to contine next method\n");
   	getchar();

       /* methode 6*/
   	printf("Gpio state write methode...");
    	openGPIO(38, 1);
   	for (i = 0; i < 5000 * 10; i++){
        	writeGPIO(38, 1);
        	writeGPIO(38, 0);
    	} 
       
	if (fd1 != 0) {
	  close(fd1);
	  fd1 = open("/sys/class/gpio/unexport", O_WRONLY);
	  len = snprintf(buf, BUFFER_MAX, "%d", gpio);
	  write(fd1, buf, len);
	  close(fd1);
    	}

	printf(" done, Press any key to exit\n");
   	getchar();

	return 0;
}
