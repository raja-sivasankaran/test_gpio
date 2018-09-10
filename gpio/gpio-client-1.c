#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/io.h>
#include "../ti81xx-usmap/ti81xx-usmap.h"
#include <gpio.h>


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
FILE *fd[32] = {};

int openGPIO(int pin, int direction);
int writeGPIO(int gpio, int value);

int openGPIO(int gpio, int direction) {

    
   int len;
   char buf[BUFFER_MAX];
    
    if (direction < 0 || direction > 1)return -2;

    if (fd[0] != NULL) {
        close(fd[0]);
        fd[0] = open("/sys/class/gpio/unexport", O_WRONLY);
        len = snprintf(buf, BUFFER_MAX, "%d", gpio);
        write(fd[0], buf, len);
        close(fd[0]);
    }

    fd[0] = open("/sys/class/gpio/export", O_WRONLY);
    len = snprintf(buf, BUFFER_MAX, "%d", gpio);
    write(fd[0], buf, len);
    close(fd[0]);

    len = snprintf(buf, BUFFER_MAX, "/sys/class/gpio/gpio%d/direction", gpio);
    fd[0] = open(buf, O_WRONLY);
    if (direction == 1) {
        write(fd[0], "out", 4);
        close(fd[0]);
        len = snprintf(buf, BUFFER_MAX, "/sys/class/gpio/gpio%d/value", gpio);
        fd[0] = open(buf, O_WRONLY);

    } else {
        write(fd[0], "in", 3);
        close(fd[0]);
        len = snprintf(buf, BUFFER_MAX, "/sys/class/gpio/gpio%d/value", gpio);
        fd[0] = open(buf, O_RDONLY);
    }
    return 0;
} 

int writeGPIO(int gpio, int b) {
    if (b == 0) {
        write(fd[0], "0", 1);
    } else {
        write(fd[0], "1", 1);
    }

    lseek(fd[0], 0, SEEK_SET);
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

  /* file = open("/dev/ti81xx-usmapchar", O_RDWR | O_SYNC);

   if (file < 0)
   {
      printf("open failed (%d)\n", errno);
      return 1;
   }


   printf("Square from kernel space...");
   fflush(stdout);

   ioctl(file, TI81XX_USMAP_IOCTL_TEST_GPIO);

   printf(" done\n");
   
   getchar();

   ioctl(file, TI81XX_USMAP_IOCTL_NOCACHE);
   pGpIoRegisters = mmap(NULL, 7 * 0x4000, PROT_READ | PROT_WRITE, MAP_SHARED, file, IMX6_GPIO_BASE);
   printf("Square from user space (no cache, without msync)...");
   fflush(stdout);

   pGpIoRegisters[IMX6_SO127_GPIO_GDIR >> 2] |= pin;   
	
   for (i = 0; i < 30000000; ++i)
   {
      pGpIoRegisters[IMX6_SO127_GPIO_DR >> 2] = pin;
      pGpIoRegisters[IMX6_SO127_GPIO_DR >> 2] &= ~pin;
   }

   pGpIoRegisters[IMX6_SO127_GPIO_GDIR >> 2] &= ~pin;

   printf(" done\n");
   fflush(stdout);
   munmap(pGpIoRegisters, 7 * 0x4000);
   
   getchar();

   ioctl(file, TI81XX_USMAP_IOCTL_CACHE);
   pGpIoRegisters = mmap(NULL, 7 * 0x4000, PROT_READ | PROT_WRITE, MAP_SHARED, file, IMX6_GPIO_BASE);
   printf("Square from user space (with cache, using msync)...");
   fflush(stdout);
    

   //bank=(volatile IMX6_GPIO_BANK*)(Imx6Gpio->gpioVa+((io.Gpio.Nr>>5)*IMX6_GPIO_BANK_OFFSET));
   //index=io.Gpio.Nr&0x1F;
   
   //if (dir != ioInput)
   // 	bank->GPIO_GDIR|=(1<<index);
   //else
   //	bank->GPIO_GDIR&=~(1<<index);

   
   //    bank=(volatile IMX6_GPIO_BANK*)(Imx6Gpio->gpioVa+((io.Gpio.Nr>>5)*IMX6_GPIO_BANK_OFFSET));
   //    index=io.Gpio.Nr&0x1F;
   //    return (bank->GPIO_DR & (1<<index))?ioHigh:ioLow;

    //bank=(volatile IMX6_GPIO_BANK*)(Imx6Gpio->gpioVa+((io.Gpio.Nr>>5)*IMX6_GPIO_BANK_OFFSET));
    //index=io.Gpio.Nr&0x1F;

    //if (val==ioHigh)
    //    bank->GPIO_DR|=(1<<index);
    //else
    //    bank->GPIO_DR&=~(1<<index);

	//Imx6Gpio_SetDaisyChain find address in the tdxlibs
   pGpIoRegisters[IMX6_SO127_GPIO_GDIR >> 2] |= pin;
   //pGpIoRegisters[OMAP4_GPIO_OE >> 2] &= ~pin;
   msync(&pGpIoRegisters[IMX6_SO127_GPIO_GDIR >> 2], sizeof(ulong), MS_SYNC);

   for (i = 0; i < 5000000; ++i)
   {
      //pGpIoRegisters[OMAP4_GPIO_SETDATAOUT >> 2] = pin;
      pGpIoRegisters[IMX6_SO127_GPIO_DR >> 2] = pin;
      //msync(&pGpIoRegisters[OMAP4_GPIO_SETDATAOUT >> 2], sizeof(ulong), MS_SYNC);
      msync(&pGpIoRegisters[IMX6_SO127_GPIO_DR >> 2], sizeof(ulong), MS_SYNC);
      pGpIoRegisters[IMX6_SO127_GPIO_DR >> 2] &= ~pin;
      msync(&pGpIoRegisters[IMX6_SO127_GPIO_DR >> 2], sizeof(ulong), MS_SYNC);
   }

   pGpIoRegisters[IMX6_SO127_GPIO_GDIR >> 2] &= ~pin;
   msync(&pGpIoRegisters[IMX6_SO127_GPIO_GDIR >> 2], sizeof(ulong), MS_SYNC);

   printf(" done\n");
   fflush(stdout);
   munmap(pGpIoRegisters, 7 * 0x4000); */

  
    
   printf("Square from sysfs...");
   
   /* getchar();
   close(file);

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

    for (i = 0; i < 5000 * 10; i++) 
   {
        fd = fopen(buf, "w");
        fprintf(fd, "1");
        fclose(fd);
        fd = fopen(buf, "w");
        fprintf(fd, "0");
        fclose(fd);
    }
    

   printf("Gpio state change fflush...");
       getchar();

    fd = fopen(buf, "w");
    for (i = 0; i < 5000 * 10; i++) 
    {
        fprintf(fd, "1");
        fflush(fd);
        fprintf(fd, "0");
        fflush(fd);
    }
    fclose(fd); */

   //Sysfs GPIO Interface
   /* int fd = file_open ("/sys/class/gpio/export", O_SYNC | O_WRONLY);
  
   if (fd < 0)
	return NULL;

   sprintf (tmp_str, "/sys/class/gpio/gpio%d/value", gpio_id);
   
   if (file_valid (tmp_str))
   {
	libsoc_gpio_debug (__func__, gpio_id, "GPIO already exported");
   }
   else
   {
	int fd = file_open ("/sys/class/gpio/export", O_SYNC | O_WRONLY);
  
	if (fd < 0)
		return NULL;
	sprintf (tmp_str, "%d", gpio_id);
	
	if (file_write (fd, tmp_str, STR_BUF) < 0)
		return NULL;
	if (file_close (fd))
	return NULL;

      sprintf (tmp_str, "/sys/class/gpio/gpio%d", gpio_id);

      if (!file_valid (tmp_str))
	{
	  libsoc_gpio_debug (__func__, gpio_id,
			     "gpio did not export correctly");
	  perror ("libsoc-gpio-debug");
	  return NULL;
	}
   }
  
   sprintf (tmp_str, "/sys/class/gpio/gpio%d/value", gpio_id);
   
   value_fd = file_open (tmp_str, O_SYNC | O_RDWR);
    
   
   if (new_gpio->value_fd < 0)
   {
  	return NULL;
   }
   //Set direction
   sprintf (path, "/sys/class/gpio/gpio%d/direction", current_gpio->gpio);

  fd = file_open (path, O_SYNC | O_WRONLY);

  if (fd < 0)
    return EXIT_FAILURE;

  if (file_write (fd, gpio_direction_strings[direction], STR_BUF) < 0)
    return EXIT_FAILURE;

  if (file_close (fd) < 0)
    return EXIT_FAILURE;
   
   //Set level
    if (file_write (current_gpio->value_fd, gpio_level_strings[level], 1) < 0)
    return EXIT_FAILURE;

   //Free GPIO

    if (file_close (gpio->value_fd) < 0)
    return EXIT_FAILURE;
   
    fd = file_open ("/sys/class/gpio/unexport", O_SYNC | O_WRONLY);

  if (fd < 0)
    return EXIT_FAILURE;

  sprintf (tmp_str, "%d", gpio->gpio);

  if (file_write (fd, tmp_str, STR_BUF) < 0)
    return EXIT_FAILURE;

  if (file_close (fd) < 0)
    return EXIT_FAILURE;

  sprintf (tmp_str, "/sys/class/gpio/gpio%d", gpio->gpio);

  if (file_valid (tmp_str))
    {
      libsoc_gpio_debug (__func__, gpio->gpio, "freeing failed");
      return EXIT_FAILURE;
    }

   free (gpio); */
    
    printf("Gpio state write methode...");
       getchar();
    //another methode
    openGPIO(38, 1);
   for (i = 0; i < 5000 * 10; i++) {
        writeGPIO(4, 1);
        writeGPIO(4, 0);
    } 
    
    //return 0;

   getchar();
   close(file);

   return 0;
}
