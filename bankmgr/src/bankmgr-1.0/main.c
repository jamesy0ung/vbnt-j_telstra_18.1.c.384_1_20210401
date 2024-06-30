/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
** All Rights Reserved                                                  **
**                                                                      **
** This program is free software; you can redistribute it and/or modify **
** it under the terms of the GNU General Public License version 2 as    **
** published by the Free Software Foundation.                           **
*************************************************************************/

#include <linux/module.h>

#include "bankmgr_proc.h"

static int __init bankmgr_init(void)
{
	return bankmgr_proc_init();
};

static void __exit bankmgr_exit(void)
{
	bankmgr_proc_cleanup();
}

module_init(bankmgr_init);
module_exit(bankmgr_exit);
MODULE_LICENSE("GPL");
