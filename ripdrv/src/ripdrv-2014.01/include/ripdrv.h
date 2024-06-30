/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
** Copyright (c) 2012-2017  -  Technicolor Delivery Technologies, SAS   **
** All Rights Reserved                                                  **
**
** This program is free software; you can redistribute it and/or modify **
** it under the terms of the GNU General Public License version 2 as    **
** published by the Free Software Foundation.                           **                                                       **
*************************************************************************/

/** @file
 *
 * File containing kernel API to access the Remote Inventory Parameters (RIP).
 */

#ifndef __RIPDRV_H__
#define __RIPDRV_H__

/* contains definitions for the linux ioctl routines for the rip
 * (Remote Inventory PROM)  
 *
 */
#if !defined(BUILDTYPE) || defined(BUILDTYPE_linux)
#include <linux/ioctl.h>
#endif

#define RIP_DEVICE			"/dev/rip"
#define RIP_IOC_MAGIC		'D' /* same as asm-s390/dasd.h but unlikely to be conflicting */

/* ioctl definitions for rip access */
#define RIP_IOCTL_READ		_IOWR(RIP_IOC_MAGIC, 1, int)
#define RIP_IOCTL_WRITE		_IOWR(RIP_IOC_MAGIC, 2, int)
#define RIP_IOCTL_FLAGS		_IOWR(RIP_IOC_MAGIC, 3, int)
#define RIP_IOCTL_LOCK		_IOWR(RIP_IOC_MAGIC, 4, int)

typedef struct rip_ioctl_data
{
	unsigned long len;
	unsigned short id;
	unsigned char *data;
	unsigned long attrHi;
	unsigned long attrLo;
} rip_ioctl_data_t;


#endif //__RIPDRV_H__
