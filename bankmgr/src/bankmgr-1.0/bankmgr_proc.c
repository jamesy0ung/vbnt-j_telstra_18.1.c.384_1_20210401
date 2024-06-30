/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
** Copyright (c) 2014 - Technicolor Delivery Technologies, SAS          **
** All Rights Reserved                                                  **
**                                                                      **
** This program is free software; you can redistribute it and/or modify **
** it under the terms of the GNU General Public License version 2 as    **
** published by the Free Software Foundation.                           **
*************************************************************************/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/proc_fs.h>
#include "bankmgr_proc.h"
#include "bankmgr.h"
#include "legacy_upgrade_info.h"
#include <linux/mtd/mtd.h>

/* Names of proc entries */
#define PROC_BANKMGR "banktable"
#define PROC_BANKMGR_BOOTED "booted"
#define PROC_BANKMGR_NOTBOOTED "notbooted"
#define PROC_BANKMGR_ACTIVE "active"
#define PROC_BANKMGR_INACTIVE "inactive"
#define PROC_BANKMGR_BANKTABLE "dumpbanktable"
#define PROC_BANKMGR_ACTIVEVERSION "activeversion"
#define PROC_BANKMGR_PASSIVEVERSION "passiveversion"
#define PROC_BANKMGR_ACTIVEOID "bootedoid"
#define PROC_BANKMGR_PASSIVEOID "notbootedoid"


typedef struct {
	int IB_Criterion;
	unsigned int IB_Itemname;
}T_IB_DESCRIPTION;

static int bank_booted = BANK_BOOTED;
static int bank_notbooted = BANK_NOTBOOTED;
static int bank_active = BANK_ACTIVE;
static int bank_inactive = BANK_INACTIVE;
static T_IB_DESCRIPTION version_active = { BANK_ACTIVE, INFO_BLOCK_ITEM_NAME_VRSS };
static T_IB_DESCRIPTION version_inactive = { BANK_INACTIVE, INFO_BLOCK_ITEM_NAME_VRSS };
static T_IB_DESCRIPTION oid_booted = { BANK_BOOTED, INFO_BLOCK_ITEM_NAME_TOID };
static T_IB_DESCRIPTION oid_notbooted = { BANK_NOTBOOTED, INFO_BLOCK_ITEM_NAME_TOID };


// #define BANKTABLE_DEBUG

/* proc root directory */
struct proc_dir_entry *bankmgr_proc_dir = 0;

static int bankmgr_criterion_proc_show(struct seq_file *m, void *v)
{
	struct bank req_bank;
	int * criterion = (int *)(m->private);
	short bankid;

	bankid = bankmgr_get_bank_id(BOOTABLE, *criterion);
	if (bankid < 0) return 0;

	bankmgr_get_bank(bankid, &req_bank);
	seq_printf(m,"%.16s\n", req_bank.name);

	return 0;
}

typedef struct __attribute__ ((__packed__)) {
	u32 IB_Size;
	u32 IB_ID;
	void * Data;
}T_IB_H;

// Parse info from info blocks in buffer
static int bankmgr_read_info_block(struct seq_file *m, char * buffer, size_t bufferlength, unsigned int item_name)
{
	T_IB_H * pIB_H;

	size_t ib_item_size_bytes = sizeof(((T_IB_H *)0x0)->IB_Size);
	size_t ib_item_id_bytes = sizeof(((T_IB_H *)0x0)->IB_ID);

	pIB_H = (T_IB_H *)(buffer);
	while (bufferlength >= ib_item_size_bytes + ib_item_id_bytes)  {
		if(ntohl(pIB_H->IB_Size) < (ib_item_size_bytes + ib_item_id_bytes) || ntohl(pIB_H->IB_Size) > bufferlength) {
			printk(KERN_ERR"Invalid infoblock length: %d\n", ntohl(pIB_H->IB_Size));
			break;
		}
		if(ntohl(pIB_H->IB_ID) == item_name) {
			/* Found */
			seq_printf(m,"%.*s\n", ntohl(pIB_H->IB_Size) - ib_item_size_bytes - ib_item_id_bytes, &pIB_H->Data);
			return 0;
		}
		bufferlength -= ntohl(pIB_H->IB_Size);
		pIB_H = (T_IB_H *)((char *)pIB_H + ntohl(pIB_H->IB_Size));
	};

	return -1;
}


static int bankmgr_info_proc_show(struct seq_file *m, void *v)
{
	T_IB_DESCRIPTION * info_block_description = (T_IB_DESCRIPTION *)(m->private);
	short bankid;
	char * buffer;
	size_t infoblock_length;

	bankid = bankmgr_get_bank_id(BOOTABLE, info_block_description->IB_Criterion);
	if (bankid < 0)
		return 0;

	buffer = bankmgr_get_infoblockdata(bankid, &infoblock_length);

	if (buffer == NULL) {
		seq_printf(m, "Unknown");
		return 0;
	}

	// If requested info is not found in info block, then try to get the corresponding legacy info, which is stored in a different place.
	if (bankmgr_read_info_block(m, buffer, infoblock_length, info_block_description->IB_Itemname)) {
		bankmgr_get_fallback_info(m, bankid, info_block_description->IB_Itemname);
	}

	kfree(buffer);
	return 0;
}

static ssize_t bankmgr_proc_write( struct file *filp, const char __user *buff, size_t count, loff_t *ppos )
{
	char *buffer =  NULL;
	short new_bankid;
	int stringlen = count;

	buffer = kmalloc(count+1, GFP_KERNEL);
	if  (buffer == NULL) {
		return -ENOMEM;
	}
	if  (0 != copy_from_user( buffer, buff, count )) {
		kfree(buffer);
		return -EFAULT;
	}

	/* Convert to a normal string */
	buffer[stringlen] = 0;
	while ((stringlen>0) && (buffer[stringlen-1] == '\n' || buffer[stringlen-1] == '\r')) buffer[--stringlen] = 0;

	new_bankid = bankmgr_get_bank_id_by_name(buffer);

	if (new_bankid < 0) {
		printk("Invalid bankname: %s\n", buffer);
	}
	else {
		bankmgr_set_active_bank_id(BOOTABLE, new_bankid);
	}
	kfree(buffer);

	return count;
}

static void handle_legacy_upgrade_info(void)
{
	int ret = 0;
	ret = load_legacy_upgrade_info();
	if (ret)
		return;

	publish_legacy_upgrade_info();
	return;
}

static void remove_legacy_upgrade_info(void)
{
	unpublish_legacy_upgrade_info();
	return;
}

#ifdef BANKTABLE_DEBUG
static int bankmgr_debuginfo_proc_show(struct seq_file *m, void *v)
{
	struct bank_table* bt = load_btab();
	int i;

	if (bt == NULL) {
		printk("Error while loading banktable\n");
		return 0;
	}

	printk(" checksum: %04X\n", (int)ntohl(bt->checksum));
	printk(" version: %d\n", (int)ntohs(bt->version));
	printk(" num_banks: %d\n", (int)ntohs(bt->num_banks));
	printk(" reserved1: %d\n", (int)ntohs(bt->reserved1));
	printk(" reserved2: %d\n", (int)ntohs(bt->reserved2));

	for (i = 0; i < (int)ntohs(bt->num_banks); ++i) {
		struct bank * b = &(bt->banks[i]);
		printk(" bank %d:\n", i);
		printk("  name: %.16s\n", b->name);
		printk("  type: ");
		switch (ntohl(b->type)) {
		case BOOTABLE:
			printk("bootable\n");
			break;
		case USERFS:
			printk("userfs\n");
			break;
		case CUSTOM:
			printk("custom\n");
			break;
		case FEMTO_AP:
			printk("femto_ap\n");
			break;
		default:
			printk("unknown (%d)\n", (int)ntohl(b->type));
			break;

		}
		printk("  size: %08lX\n", ntohl(b->size));
		printk("  bank-offset: %08lX\n", ntohl(b->bank_offset));
		printk("  boot-offset: %08lX\n", ntohl(b->boot_offset));
		printk("  FVP: %04hX\n", ntohs(b->FVP));
		printk("  flags: %04hX\n", ntohs(b->flags));
	}
	free_btab(bt);

	return 0;
}
#endif

struct proc_dir_entry *bankmgr_proc_get_proc_root_dir(void)
{
	return bankmgr_proc_dir;
}

BANKMGR_SEQ_FOPS(bankmgr_criterion_proc_ro_fops, bankmgr_criterion_proc_show, NULL);
BANKMGR_SEQ_FOPS(bankmgr_criterion_proc_rw_fops, bankmgr_criterion_proc_show, bankmgr_proc_write);
#ifdef BANKTABLE_DEBUG
BANKMGR_SEQ_FOPS(bankmgr_debuginfo_proc_fops, bankmgr_debuginfo_proc_show, NULL);
#endif
BANKMGR_SEQ_FOPS(bankmgr_info_proc_fops, bankmgr_info_proc_show, NULL);

int bankmgr_proc_init(void)
{
	struct proc_dir_entry *entry;

	if( !bankmgr_init() ) {
		printk(KERN_ERR "Failed to find rawstorage\n");
		return 0;
	}

	bankmgr_proc_dir = proc_mkdir(PROC_BANKMGR, NULL);
	if (!bankmgr_proc_dir) {
		printk(KERN_ERR "Error: Could not initialize /proc interface\n");
		return -EFAULT;
	}

	entry = proc_create_data(PROC_BANKMGR_BOOTED, 0444, bankmgr_proc_dir, &bankmgr_criterion_proc_ro_fops, &bank_booted);
	entry = proc_create_data(PROC_BANKMGR_NOTBOOTED, 0444, bankmgr_proc_dir, &bankmgr_criterion_proc_ro_fops, &bank_notbooted);
	entry = proc_create_data(PROC_BANKMGR_ACTIVE, 0644, bankmgr_proc_dir, &bankmgr_criterion_proc_rw_fops, &bank_active);
	entry = proc_create_data(PROC_BANKMGR_INACTIVE, 0444, bankmgr_proc_dir, &bankmgr_criterion_proc_ro_fops, &bank_inactive);
#ifdef BANKTABLE_DEBUG
	entry = proc_create_data(PROC_BANKMGR_BANKTABLE, 0444, bankmgr_proc_dir, &bankmgr_debuginfo_proc_fops, NULL);
#endif
	entry = proc_create_data(PROC_BANKMGR_ACTIVEVERSION, 0444, bankmgr_proc_dir, &bankmgr_info_proc_fops, &version_active);
	entry = proc_create_data(PROC_BANKMGR_PASSIVEVERSION, 0444, bankmgr_proc_dir, &bankmgr_info_proc_fops, &version_inactive);
	entry = proc_create_data(PROC_BANKMGR_ACTIVEOID, 0444, bankmgr_proc_dir, &bankmgr_info_proc_fops, &oid_booted);
	entry = proc_create_data(PROC_BANKMGR_PASSIVEOID, 0444, bankmgr_proc_dir, &bankmgr_info_proc_fops, &oid_notbooted);

	handle_legacy_upgrade_info();

	return 0;
}

int bankmgr_proc_cleanup(void)
{
	if ( bankmgr_proc_dir ) {;
		remove_proc_entry(PROC_BANKMGR_BOOTED, bankmgr_proc_dir);
		remove_proc_entry(PROC_BANKMGR_NOTBOOTED, bankmgr_proc_dir);
		remove_proc_entry(PROC_BANKMGR_ACTIVE, bankmgr_proc_dir);
		remove_proc_entry(PROC_BANKMGR_INACTIVE, bankmgr_proc_dir);
#ifdef BANKTABLE_DEBUG
		remove_proc_entry(PROC_BANKMGR_BANKTABLE, bankmgr_proc_dir);
#endif
		remove_proc_entry(PROC_BANKMGR_ACTIVEVERSION, bankmgr_proc_dir);
		remove_proc_entry(PROC_BANKMGR_PASSIVEVERSION, bankmgr_proc_dir);
		remove_proc_entry(PROC_BANKMGR_ACTIVEOID, bankmgr_proc_dir);
		remove_proc_entry(PROC_BANKMGR_PASSIVEOID, bankmgr_proc_dir);

		remove_legacy_upgrade_info();

		remove_proc_entry(PROC_BANKMGR, NULL);
	}
	bankmgr_fini();
	return 0;
}

