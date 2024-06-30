#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/mtd/mtd.h>
#include <linux/seq_file.h>
#include "bankmgr.h"
#include "bankmgr_p.h"

#include "storage_core.h"

#define DEBUG(fmt, args...)	printk(fmt, ##args)
#define ERROR(fmt, args...)	printk(fmt, ##args)

#define FII_LENGTH 8
#define FII_LEGACY_VERSION_OFFSET 3
#define FII_TO_VERSION(fii)  (fii <= 0x09 ? fii + 0x30 : fii + 0x37)

struct storage_context * storage_ctx;

#ifdef CONFIG_MTD_SPEEDTOUCH_MAP_RAW_FLASH
int partitions_invalidate(void);
int partitions_are_invalid(void);
int partitions_reload(void* btab);
#endif

int bankmgr_init() {
	storage_ctx = rawstorage_open();
#ifdef CONFIG_MTD_SPEEDTOUCH_MAP_RAW_FLASH
	if( partitions_are_invalid()) {
		if( storage_ctx ) {
			struct bank_table *btab = load_btab();
			if( btab ) {
				/* release the storage context, otherwise the partition can
				 * not be deleted.
				 */
				rawstorage_close();
				partitions_reload(btab);
				free_btab(btab);
				/* reopen the context now */
				storage_ctx = rawstorage_open();
			}
		} else {
			/* no rawstorage available, used as partitions reload trigger */
				partitions_reload(NULL);
		}
	}
#endif
	return storage_ctx != 0;
}

void bankmgr_fini()
{
#ifdef CONFIG_MTD_SPEEDTOUCH_MAP_RAW_FLASH
	partitions_invalidate();
#endif
	rawstorage_close();
}

struct storage_context *get_storage_ctx(void)
{
	return storage_ctx;
}

struct bank_table* load_btab(void)
{
	struct bank_table *bt = NULL;
	int size;

	size = storage_get_param(storage_ctx, BTAB_ID, NULL, 0);
	if (size == 0)
		goto out;

	bt = kmalloc(size, GFP_KERNEL);
	if (bt) {

		if (storage_get_param(storage_ctx, BTAB_ID, bt, size) != size) {
			kfree(bt);
			bt = NULL;
		}

	}

out:

	return bt;
}

int store_btab(struct bank_table *bt)
{
	int size, ret = BANKMGR_NO_ERR;
	unsigned long checksum;
	struct bank_table *bt_tmp = NULL;
	int maxversion;


	/* get version of banktable currently in rawstorage, because we cant trust the version we get in bt */
	bt_tmp = load_btab();
	if (!bt_tmp) {
		ERROR("load btab failed\n");
		ret = -1;
		goto out;
	}
	
	if (bt_tmp->magic[3]=='B')
		maxversion = 2;
	else if (bt_tmp->magic[3]=='2')
		maxversion = 29999;
	else
	{
		ERROR("no btab magic found\n");
		ret = -1;
		goto out;
	}

	if (ntohs(bt_tmp->version) < maxversion)
		bt->version = htons(ntohs(bt_tmp->version) + 1);
	else
		bt->version = 0;

	if (bt_tmp) free_btab(bt_tmp);

	if (bankmgr_checksum(bt, &checksum)) {
		ERROR("bankmgr_checksum() failed\n");
		ret = -1;
		goto out;
	}
	bt->checksum = htonl(checksum);

	size = sizeof(struct bank_table) + ntohs(bt->num_banks) * sizeof(struct bank);

	if (storage_set_param(storage_ctx, BTAB_ID, bt, size) != size) {
		ERROR("rs_set_param() failed\n");
		ret = BANKMGR_ERR;
	}


out:

	return ret;
}

void free_btab(struct bank_table *bt)
{
	kfree(bt);
}

/*
 * bankmgr_find_mtd_offset -- find mtd and infoblock offset for a given bank
 */
static int bankmgr_find_mtd_offset(short bank_id, struct mtd_info **mtd, unsigned long * off)
{
	struct bank req_bank;
	int retlen;
	u32 fvp;
	unsigned long fvp_start;

	size_t fvp_bytes = sizeof(fvp);

	bankmgr_get_bank(bank_id, &req_bank);

	*mtd = get_mtd_device_nm(req_bank.name);
	if (IS_ERR_OR_NULL(*mtd)) {
		return -1;
	}

	fvp_start = ntohl(req_bank.boot_offset) - ntohl(req_bank.bank_offset);

	mtd_read(*mtd, fvp_start, fvp_bytes, &retlen, &fvp);
	if (fvp != 0)
	{
		return -1;
	}


	*off = fvp_start + fvp_bytes + FII_LENGTH;

	return 0;
}

/* ----------------------------------------------------------------------------
 * 'Public' bank table operations
 * --------------------------------------------------------------------------*/

/*
 * bankmgr_set_active_bank_id -- sets the active bank of a certain type
 */
int bankmgr_set_active_bank_id(enum bank_type type, short bank_id)
{
	struct bank_table *bt = NULL;
	short act_id;
	int ret = BANKMGR_ERR;

	if ((bt = load_btab()) == NULL)
		return BANKMGR_ERR;

	if (ntohl(bt->banks[bank_id].type) != type) {
		ERROR("bankmgr_set_active_bank_id: bank %hd is not of type %d\n", bank_id, type);
		goto out;
	}

	act_id = __bankmgr_get_active_bank_id(bt, type);

	if (bank_id < 0) {
		ERROR("invalid bank_id %d\n", bank_id);
		goto out;
	}

	if (act_id != bank_id) {
		if( act_id>=0 ) {
			bt->banks[act_id].flags &= htons(~(BANK_FLAGS_ACTIVE_MASK));
		}
		bt->banks[bank_id].flags |= htons(BANK_FLAGS_ACTIVE_MASK);
		bt->banks[bank_id].FVP = 0x0000;
		ret = store_btab(bt);
		//DEBUG("New active bank %d\n", bank_id);
	} else {
		/* If the requested bank was already active, then return success */
		ret = BANKMGR_NO_ERR;
		//DEBUG("Bank %d was already active\n", bank_id);
	}

out:
	free_btab(bt);

	return ret;
}

int bankmgr_set_bank_specific(short bank_id, void *data, int size)
{
	return 0;
}

extern unsigned int btab_bank;

/*
 * bankmgr_get_booted_bank_id -- returns the bank id of the booted bank.
 * NOTE: This is not necessarily the same as the active bank, in case of
 * software start failures.
 */
short bankmgr_get_booted_bank_id(void)
{
	return btab_bank;
}


/*
 * bankmgr_get_infoblockdata -- returns the infoblockdata for a specific bank
 * The caller should free the pointer when done
 */
char * bankmgr_get_infoblockdata(short bank_id, size_t * length) {

	struct mtd_info *mtd;

	int retlen;
	unsigned long off;
	u32 ib_addr_mtdraw, ib_length_mtdraw, InfoBlkOff, InfoLength;
	char *IB_buf = NULL;

	size_t ib_addr_bytes = sizeof(ib_addr_mtdraw);
	size_t ib_length_bytes = sizeof(ib_length_mtdraw);

	*length = 0;

	if (bankmgr_find_mtd_offset(bank_id, &mtd, &off)) {
		return NULL;
	}

	mtd_read(mtd, off, ib_addr_bytes, &retlen, &ib_addr_mtdraw);
	InfoBlkOff = ntohl(ib_addr_mtdraw);

	/* Build doesn't contain infoblock! */
	if(InfoBlkOff == 0xFFFFFFFF)
		return NULL;


	off = InfoBlkOff;
	mtd_read(mtd, off, ib_length_bytes, &retlen, &ib_length_mtdraw);
	InfoLength = ntohl(ib_length_mtdraw);
	if(InfoLength < ib_length_bytes)
		return NULL;



	IB_buf = kmalloc(InfoLength - ib_length_bytes, GFP_KERNEL);
	if(IB_buf == NULL)
		return NULL;
	mtd_read(mtd, InfoBlkOff + ib_length_bytes, InfoLength - ib_length_bytes, &retlen, IB_buf);

	*length = InfoLength - ib_length_bytes;

	return IB_buf;
}


/*
 * Try to read out legacy info for requested item
 */
void bankmgr_get_fallback_info(struct seq_file *m, short bank_id, unsigned int item_name)
{
	int retlen;
	unsigned long off;
	char fii[FII_LENGTH];
	struct mtd_info *mtd;
	unsigned long start_offset;

	if (bankmgr_find_mtd_offset(bank_id, &mtd, &start_offset)) {
		return;
	}

	switch(item_name) {
	case INFO_BLOCK_ITEM_NAME_VRSS:
		off = start_offset - FII_LENGTH;
		mtd_read(mtd, off, FII_LENGTH, &retlen, fii);
		if (FII_LEGACY_VERSION_OFFSET + 4 >= FII_LENGTH) {
			printk(KERN_ERR"Version stored outside the array boundary\n");
			break;
		}
		seq_printf(m, "%d.%c.%c.%c.%c\n", fii[FII_LEGACY_VERSION_OFFSET+0], FII_TO_VERSION(fii[FII_LEGACY_VERSION_OFFSET+1]), FII_TO_VERSION(fii[FII_LEGACY_VERSION_OFFSET+2]), FII_TO_VERSION(fii[FII_LEGACY_VERSION_OFFSET+3]), FII_TO_VERSION(fii[FII_LEGACY_VERSION_OFFSET+4]));
		break;
	default:
		break;
	}
	return;
}
