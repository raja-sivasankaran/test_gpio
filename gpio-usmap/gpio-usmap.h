/* 
 * File:   ti81xx-usmap.h
 * Author: jj
 *
 * Created on June 6, 2012, 10:31 AM
 */

#ifndef _GPIO_USMAP_H
#define _GPIO_USMAP_H

#include <linux/ioctl.h>

#define GPIO_USMAP_IOCTL_MAGIC 'j'

#define GPIO_USMAP_IOCTL_TEST_GPIO _IO(GPIO_USMAP_IOCTL_MAGIC, 0)
#define GPIO_USMAP_IOCTL_CACHE _IO(GPIO_USMAP_IOCTL_MAGIC, 1)
#define GPIO_USMAP_IOCTL_NOCACHE _IO(GPIO_USMAP_IOCTL_MAGIC, 2)

#endif	/* _GPIO_USMAP_H */

