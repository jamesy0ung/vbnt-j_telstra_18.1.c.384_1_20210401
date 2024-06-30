/************** COPYRIGHT AND CONFIDENTIALITY INFORMATION ********************
**                                                                          **
** Copyright (c) 2012-2017  -  Technicolor Delivery Technologies, SAS       **
** All Rights Reserved                                                      **
**
** This program is free software; you can redistribute it and/or modify **
** it under the terms of the GNU General Public License version 2 as    **
** published by the Free Software Foundation.                           **                                                       **
*****************************************************************************/

#include "rip2.h"
#include <linux/errno.h>


int rip_drv_get_flags (unsigned short id, unsigned long *attrHi, unsigned long *attrLo)
{
	T_RIP2_ID rip2ID = (T_RIP2_ID)id;
	T_RIP2_ITEM  item;
	//printk("***** rip2_drv_get_flags of id%x\r\n", id);

	if (RIP2_SUCCESS == rip2_get_idx(rip2ID, &item)) {
		*attrLo = item.attr[ATTR_LO];
		*attrHi = item.attr[ATTR_HI];
		return 0;
	}

	return(-EINVAL);
}

int rip_drv_lock_item (unsigned short id)
{
	T_RIP2_ID rip2ID = (T_RIP2_ID)id;
	//printk("***** rip2_drv_lock_item of id%x\r\n", id);

	if (RIP2_SUCCESS == rip2_lock(rip2ID)) {
		 //printk("***** successfully locked\r\n");
		return 0;
	}

    return (-EINVAL);
}
