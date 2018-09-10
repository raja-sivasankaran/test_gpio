#include <linux/init.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/fcntl.h>
//#include <asm/system.h>
#include <asm/uaccess.h>
#include <asm/io.h>
//#include <plat/ti81xx.h>
#include "ti81xx-usmap.h"
#include <linux/device.h>         // Header to support the kernel Driver Model

#define  DEVICE_NAME "ti81xx-usmapchar"    ///< The device will appear at /dev/ebbchar using this value
#define  CLASS_NAME  "ti81xx-usmap"        ///< The device class -- this is a character device driver

static struct class*  ebbcharClass  = NULL; ///< The device-driver class struct pointer
static struct device* ebbcharDevice = NULL; ///< The device-driver device struct pointer

#define TI81XX_USMAP_DEBUG

#ifdef TI81XX_USMAP_DEBUG
#define dbglog(...) printk("<1> TI81xx debug: " __VA_ARGS__)
#else
#define dbglog(...)
#endif

#define errlog(...) printk("<1> TI81xx error: " __VA_ARGS__)


MODULE_LICENSE("GPL");            ///< The license type -- this affects available functionality
MODULE_AUTHOR("Derek Molloy");    ///< The author -- visible when you use modinfo
MODULE_DESCRIPTION("A simple Linux char driver for the BBB");  ///< The description -- see modinfo
MODULE_VERSION("0.1");            ///< A version number to inform users


static int ti81xx_usmap_open(struct inode* pNode, struct file* pFile);
static long ti81xx_usmap_ioctl(struct file* pFile, unsigned int cmd, unsigned long arg);
static int ti81xx_usmap_mmap(struct file* pFile, struct vm_area_struct* pVma);
static int ti81xx_usmap_release(struct inode* pNode, struct file* pFile);

static void ti81xx_usmap_vma_open(struct vm_area_struct* pVma);
static void ti81xx_usmap_vma_close(struct vm_area_struct* pVma);

static void ti81xx_usmap_test_gpio(void);

const int ti81xx_usmap_major = 0;

static int ti81xx_usmap_use_cache = 0;

struct file_operations ti81xx_usmap_file_ops =
{
   .open = ti81xx_usmap_open,
   .unlocked_ioctl = ti81xx_usmap_ioctl,
   .mmap = ti81xx_usmap_mmap,
   .release = ti81xx_usmap_release
};

struct vm_operations_struct ti81xx_usmap_vm_ops =
{
   .open = ti81xx_usmap_vma_open,
   .close = ti81xx_usmap_vma_close
};

int majorNumber;

static int ti81xx_usmap_init(void)
{
   
   majorNumber = register_chrdev(ti81xx_usmap_major, "ti81xx-usmap", &ti81xx_usmap_file_ops);
   if (majorNumber < 0)
   {
      dbglog("Failed to create the device file\n");
      return majorNumber;
   }
   
   ebbcharClass = class_create(THIS_MODULE, CLASS_NAME);
   if (IS_ERR(ebbcharClass)){                // Check for error and clean up if there is
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to register device class\n");
      return PTR_ERR(ebbcharClass);          // Correct way to return an error on a pointer
   }
   printk(KERN_INFO "EBBChar: device class registered correctly\n");

   // Register the device driver
   ebbcharDevice = device_create(ebbcharClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
   if (IS_ERR(ebbcharDevice)){               // Clean up if there is an error
      class_destroy(ebbcharClass);           // Repeated code but the alternative is goto statements
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to create the device\n");
      return PTR_ERR(ebbcharDevice);
   }
   
   dbglog("TI81xx-USMap loaded major changed %d\n", majorNumber);
   
   return 0;
}

static int ti81xx_usmap_open(struct inode* pNode, struct file* pFile)
{
   dbglog("TI81xx-USMap opened\n");
   return 0;
}

static long ti81xx_usmap_ioctl(struct file* pFile, unsigned int cmd, unsigned long arg)
{
   switch (cmd)
   {
   case TI81XX_USMAP_IOCTL_TEST_GPIO:
      ti81xx_usmap_test_gpio();
      break;

   case TI81XX_USMAP_IOCTL_CACHE:
      ti81xx_usmap_use_cache = 1;
      break;

   case TI81XX_USMAP_IOCTL_NOCACHE:
      ti81xx_usmap_use_cache = 0;
      break;
      
   default:
      return -ENOTTY;
   }
   
   return 0;
}

static int ti81xx_usmap_mmap(struct file* pFile, struct vm_area_struct* pVma)
{
//   pVma->vm_flags |= VM_RESERVED;
   if (!ti81xx_usmap_use_cache)
      pVma->vm_page_prot = pgprot_noncached(pVma->vm_page_prot);
   
   if (io_remap_pfn_range(pVma, pVma->vm_start, pVma->vm_pgoff,
                           pVma->vm_end - pVma->vm_start, pVma->vm_page_prot))
      return -EAGAIN;

   pVma->vm_ops = &ti81xx_usmap_vm_ops;
   ti81xx_usmap_vma_open(pVma);
   return 0;
}

static void ti81xx_usmap_vma_open(struct vm_area_struct* pVma)
{
   dbglog("VMA open: %lx mapped to %lx\n", pVma->vm_start, pVma->vm_pgoff << PAGE_SHIFT);
}

static void ti81xx_usmap_vma_close(struct vm_area_struct* pVma)
{
   dbglog("VMA closed\n");
}

static int ti81xx_usmap_release(struct inode* pNode, struct file* pFile)
{
   dbglog("TI81xx-USMap released\n");
   return 0;
}

static void ti81xx_usmap_exit(void)
{
   device_destroy(ebbcharClass, MKDEV(majorNumber, 0));     // remove the device
   class_unregister(ebbcharClass);                          // unregister the device class
   class_destroy(ebbcharClass);                             // remove the device class
   unregister_chrdev(ti81xx_usmap_major, "ti81xx-usmap");
   dbglog("TI81xx-USMap exited\n");
}

module_init(ti81xx_usmap_init);
module_exit(ti81xx_usmap_exit);



/* Other functionalities */
/* #define TI81XX_GPIO0_BASE 		0x40000000

#define OMAP4_GPIO_REVISION		0x0000
#define OMAP4_GPIO_EOI			0x0020
#define OMAP4_GPIO_IRQSTATUSRAW0	0x0024
#define OMAP4_GPIO_IRQSTATUSRAW1	0x0028
#define OMAP4_GPIO_IRQSTATUS0		0x002c
#define OMAP4_GPIO_IRQSTATUS1		0x0030
#define OMAP4_GPIO_IRQSTATUSSET0	0x0034
#define OMAP4_GPIO_IRQSTATUSSET1	0x0038
#define OMAP4_GPIO_IRQSTATUSCLR0	0x003c
#define OMAP4_GPIO_IRQSTATUSCLR1	0x0040
#define OMAP4_GPIO_IRQWAKEN0		0x0044
#define OMAP4_GPIO_IRQWAKEN1		0x0048
#define OMAP4_GPIO_IRQENABLE1		0x011c
#define OMAP4_GPIO_WAKE_EN		0x0120
#define OMAP4_GPIO_IRQSTATUS2		0x0128
#define OMAP4_GPIO_IRQENABLE2		0x012c
#define OMAP4_GPIO_CTRL			0x0130
#define OMAP4_GPIO_OE			0x0134
#define OMAP4_GPIO_DATAIN		0x0138
#define OMAP4_GPIO_DATAOUT		0x013c
#define OMAP4_GPIO_LEVELDETECT0		0x0140
#define OMAP4_GPIO_LEVELDETECT1		0x0144
#define OMAP4_GPIO_RISINGDETECT		0x0148
#define OMAP4_GPIO_FALLINGDETECT	0x014c
#define OMAP4_GPIO_DEBOUNCENABLE	0x0150
#define OMAP4_GPIO_DEBOUNCINGTIME	0x0154
#define OMAP4_GPIO_CLEARIRQENABLE1	0x0160
#define OMAP4_GPIO_SETIRQENABLE1	0x0164
#define OMAP4_GPIO_CLEARWKUENA		0x0180
#define OMAP4_GPIO_SETWKUENA		0x0184
#define OMAP4_GPIO_CLEARDATAOUT		0x0190
#define OMAP4_GPIO_SETDATAOUT		0x0194 */

#define IMX6_GPIO_BASE 0x0209C000
#define IMX6_IOMUXX_BASE   0x020E0000
#define IMX6_GPIO_BANK_OFFSET 0x00004000

#define IMX6_SO127_GPIO_DR      (IMX6_GPIO_BANK_OFFSET + 0)
#define IMX6_SO127_GPIO_GDIR    (IMX6_GPIO_BANK_OFFSET + 4)
#define IMX6_SO127_GPIO_PSR     (IMX6_GPIO_BANK_OFFSET + 8)
#define IMX6_SO127_GPIO_ICR1    (IMX6_GPIO_BANK_OFFSET + 0xC)
#define IMX6_SO127_GPIO_ICR2    (IMX6_GPIO_BANK_OFFSET + 0x10)
#define IMX6_SO127_GPIO_IMR     (IMX6_GPIO_BANK_OFFSET + 0x14)
#define IMX6_SO127_GPIO_ISR     (IMX6_GPIO_BANK_OFFSET + 0x18)
#define IMX6_SO127_GPIO_EDGE_SEL   (IMX6_GPIO_BANK_OFFSET + 0x1C)

static void ti81xx_usmap_test_gpio(void)
{
   u32* pGpIoRegisters = ioremap_nocache(IMX6_GPIO_BASE, 7 * 0x4000); //phys_to_virt(TI81XX_GPIO0_BASE);
   const u32 pin = 1 << 6;
   int i;
  
   dbglog("ti81xx_usmap_test_gpio calling\n");
	
   pGpIoRegisters[IMX6_SO127_GPIO_GDIR >> 2] |= pin;    /* Set pin as input*/
   //pGpIoRegisters[OMAP4_GPIO_OE >> 2] &= ~pin;    /* Set pin as output*/

   for (i = 0; i < 200000000; ++i)
   {
      pGpIoRegisters[IMX6_SO127_GPIO_DR >> 2] = pin;
      pGpIoRegisters[IMX6_SO127_GPIO_DR >> 2] &= ~pin;
   }
   
   pGpIoRegisters[IMX6_SO127_GPIO_GDIR >> 2] &= ~pin;
   

   iounmap(pGpIoRegisters);
}
