#include <linux/init.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/fcntl.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include "gpio-usmap.h"
#include <linux/device.h>         // Header to support the kernel Driver Model

#define  DEVICE_NAME "gpio-usmapchar"    ///< The device will appear at /dev/ebbchar using this value
#define  CLASS_NAME  "gpio-usmap"        ///< The device class -- this is a character device driver

static struct class*  ebbcharClass  = NULL; ///< The device-driver class struct pointer
static struct device* ebbcharDevice = NULL; ///< The device-driver device struct pointer

#define GPIO_USMAP_DEBUG

#ifdef GPIO_USMAP_DEBUG
#define dbglog(...) printk("<1> GPIO debug: " __VA_ARGS__)
#else
#define dbglog(...)
#endif

#define errlog(...) printk("<1> GPIO error: " __VA_ARGS__)

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

static int gpio_usmap_open(struct inode* pNode, struct file* pFile);
static long gpio_usmap_ioctl(struct file* pFile, unsigned int cmd, unsigned long arg);
static int gpio_usmap_mmap(struct file* pFile, struct vm_area_struct* pVma);
static int gpio_usmap_release(struct inode* pNode, struct file* pFile);

static void gpio_usmap_vma_open(struct vm_area_struct* pVma);
static void gpio_usmap_vma_close(struct vm_area_struct* pVma);

static void gpio_usmap_test_gpio(void);

const int gpio_usmap_major = 0;

static int gpio_usmap_use_cache = 0;

struct file_operations gpio_usmap_file_ops =
{
   	.open = gpio_usmap_open,
   	.unlocked_ioctl = gpio_usmap_ioctl,
   	.mmap = gpio_usmap_mmap,
   	.release = gpio_usmap_release
};

struct vm_operations_struct gpio_usmap_vm_ops =
{
   	.open = gpio_usmap_vma_open,
   	.close = gpio_usmap_vma_close
};

int majorNumber;

static int gpio_usmap_init(void)
{
  	majorNumber = register_chrdev(gpio_usmap_major, "gpio-usmap", &gpio_usmap_file_ops);
   	if (majorNumber < 0){
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
   
   	dbglog("gpio-usmap loaded major changed %d\n", majorNumber);
   
  	 return 0;
}

static int gpio_usmap_open(struct inode* pNode, struct file* pFile)
{
   	dbglog("gpio-usmap opened\n");
   	return 0;
}

static long gpio_usmap_ioctl(struct file* pFile, unsigned int cmd, unsigned long arg)
{
   	switch (cmd)
   	{
   		case GPIO_USMAP_IOCTL_TEST_GPIO:
      			gpio_usmap_test_gpio();
      		break;

   		case GPIO_USMAP_IOCTL_CACHE:
      			gpio_usmap_use_cache = 1;
      		break;

   		case GPIO_USMAP_IOCTL_NOCACHE:
      			gpio_usmap_use_cache = 0;
      		break;
      
   		default:
      			return -ENOTTY;
	 }
   	return 0;
}

static int gpio_usmap_mmap(struct file* pFile, struct vm_area_struct* pVma)
{
	// pVma->vm_flags |= VM_RESERVED;
   	if (!gpio_usmap_use_cache)
      		pVma->vm_page_prot = pgprot_noncached(pVma->vm_page_prot);
   
    	if (io_remap_pfn_range(pVma, pVma->vm_start, pVma->vm_pgoff,
                           pVma->vm_end - pVma->vm_start, pVma->vm_page_prot))
      		return -EAGAIN;

   	pVma->vm_ops = &gpio_usmap_vm_ops;
   		gpio_usmap_vma_open(pVma);
   	return 0;
}

static void gpio_usmap_vma_open(struct vm_area_struct* pVma)
{
   	dbglog("VMA open: %lx mapped to %lx\n", pVma->vm_start, pVma->vm_pgoff << PAGE_SHIFT);
}

static void gpio_usmap_vma_close(struct vm_area_struct* pVma)
{
   	dbglog("VMA closed\n");
}

static int gpio_usmap_release(struct inode* pNode, struct file* pFile)
{
   	dbglog("gpio-usmap released\n");
   	return 0;
}

static void gpio_usmap_exit(void)
{
   	device_destroy(ebbcharClass, MKDEV(majorNumber, 0));     // remove the device
   	class_unregister(ebbcharClass);                          // unregister the device class
   	class_destroy(ebbcharClass);                             // remove the device class
   	unregister_chrdev(gpio_usmap_major, "gpio-usmap");
   	dbglog("gpio-usmap exited\n");
}

module_init(gpio_usmap_init);
module_exit(gpio_usmap_exit);


static void gpio_usmap_test_gpio(void)
{
   	u32* pGpIoRegisters = ioremap_nocache(IMX6_GPIO_BASE, 7 * 0x4000); //phys_to_virt(GPIO_GPIO0_BASE);
   	const u32 pin = 1 << 6;
   	int i;
  
   	dbglog("gpio_usmap_test_gpio calling\n");
	
   	pGpIoRegisters[IMX6_SO127_GPIO_GDIR >> 2] |= pin;    /* Set pin as input*/

   	for (i = 0; i < 200000000; ++i)
   	{
      		pGpIoRegisters[IMX6_SO127_GPIO_DR >> 2] = pin;
      		pGpIoRegisters[IMX6_SO127_GPIO_DR >> 2] &= ~pin;
   	}
   
  	 pGpIoRegisters[IMX6_SO127_GPIO_GDIR >> 2] &= ~pin;
   

   	iounmap(pGpIoRegisters);
}

MODULE_LICENSE("GPL");            ///< The license type -- this affects available functionality
MODULE_AUTHOR("Derek Molloy");    ///< The author -- visible when you use modinfo
MODULE_DESCRIPTION("A simple Linux char driver for the BBB");  ///< The description -- see modinfo
MODULE_VERSION("0.1");            ///< A version number to inform users

