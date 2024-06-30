/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
** Copyright (c) 2014 - Technicolor Delivery Technologies, SAS          **
** All Rights Reserved                                                  **
**                                                                      **
** This program is free software; you can redistribute it and/or modify **
** it under the terms of the GNU General Public License version 2 as    **
** published by the Free Software Foundation.                           **
*************************************************************************/

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/mtd/ubi.h>
#include "bankmgr.h"
#include "legacy_upgrade_info.h"

#define DEBUG(fmt, args...)	printk(fmt, ##args)
#define ERROR(fmt, args...)	printk(fmt, ##args)

#define BANKINFO_VOLNAME "bank_info"
#define BANKINFO_MAGIC "BTA3"

/* btab_bank is a kernel symbol, filled in based on the kernel cmdline (btab_bootid) */
extern unsigned btab_bank;

typedef struct bank_info {
    char magic[4]; // "BTA3"
    uint16_t active_bank; // 0 or 1 (corresponding to bank A or B respectively)
} bank_info_t;

typedef struct {
    unsigned int size;

} InfoBlockHeader;


int bankmgr_init() {
	return 1;
}

void bankmgr_fini()
{
}


/*
 * bankmgr_set_active_bank_id -- sets the active bank of a certain type
 */
int bankmgr_set_active_bank_id(enum bank_type type, short bank_id)
{
	bank_info_t * bi = NULL;
	struct ubi_volume_desc * desc = NULL;
	struct ubi_volume_info info;
	char * buffer = NULL;
	int writesize = 0;

	desc = ubi_open_volume_nm(0, BANKINFO_VOLNAME, UBI_READWRITE);
	if (IS_ERR(desc)) {
		ERROR("Could not find UBI volume bank_info to determine active bank\n");
		return BANKMGR_ERR;
	}
	ubi_get_volume_info(desc, &info);

	writesize = info.usable_leb_size;
	buffer = kmalloc(writesize, GFP_KERNEL);
	if (buffer == NULL) {
		ERROR("Could not allocate memory\n");
		ubi_close_volume(desc);
		return BANKMGR_ERR;
	}
	bi = (bank_info_t * ) buffer;

	// get the latest bank table
	if (ubi_leb_read(desc, 0, buffer, 0, writesize, 0) != 0) {
		ERROR("Error while reading from bank_info to determine active bank\n");
		ubi_close_volume(desc);
		kfree(buffer);
		return BANKMGR_ERR;
	}

	if (strncmp(bi->magic, BANKINFO_MAGIC, sizeof(bi->magic))) {
		ERROR("Invalid bank_info magic\n");
		ubi_close_volume(desc);
		kfree(buffer);
		return -1;
	}
	bi->active_bank = htons(bank_id);

	if (ubi_leb_change(desc, 0, buffer, writesize)) {
		ERROR("Error while writing %d bytes to bank_info\n", writesize);
		ubi_close_volume(desc);
		kfree(buffer);
		return BANKMGR_ERR;
	}

	ubi_close_volume(desc);
	kfree(buffer);

	return BANKMGR_NO_ERR;
}




/*
 * bankmgr_get_bank -- returns the bank structure of a specific bank
 * The caller should pass a pointer to an allocated bank structure.
 */
int bankmgr_get_bank(short bank_id, struct bank *bank)
{
	if (bank_id < 0)
		return BANKMGR_ERR;

	sprintf(bank->name, "bank_%d", bank_id+1);

	return BANKMGR_NO_ERR;
}

short bankmgr_get_bank_id_by_name(char * bankname) {

	if (strcmp(bankname, "bank_1") == 0) {
		return 0;
	}
	else if (strcmp(bankname, "bank_2") == 0) {
		return 1;
	}
	else {
		return -1;
	}
}

/*
 * bankmgr_get_other_bank_id -- returns the "inverted" bank id
 */
static short get_other_bank_id(short exclude_id)
{
	switch (exclude_id) {
	case 0:
		return 1;
		break;
	case 1:
		return 0;
		break;
	default:
		return -1;
	}
}


/*
 * bankmgr_get_active_bank_id -- returns the id of the currently active bank
 */
short bankmgr_get_active_bank_id(enum bank_type type)
{
	bank_info_t bi;
	struct ubi_volume_desc * desc;

	desc = ubi_open_volume_nm(0, BANKINFO_VOLNAME, UBI_READONLY);
	if (IS_ERR(desc)) {
		ERROR("Could not find UBI volume bank_info to determine active bank\n");
		return -1;
	}

	// get the latest bank table
	if (ubi_leb_read(desc, 0, (char *)&bi, 0, sizeof(bi), 0) != 0) {
		ERROR("Error while reading from bank_info to determine active bank\n");
		ubi_close_volume(desc);
		return -1;
	}
	ubi_close_volume(desc);
	if (strncmp(bi.magic, BANKINFO_MAGIC, sizeof(bi.magic))) {
		ERROR("Invalid bank_info magic\n");
		return -1;
	}
	return ntohs(bi.active_bank);
}

/*
 * bankmgr_get_booted_bank_id -- returns the id of the currently booted bank
 */
short bankmgr_get_booted_bank_id(void)
{
	return btab_bank;
}

/*
 * bankmgr_get_bank_id -- retrieves the id for the (in)active/booted bank
 */
short bankmgr_get_bank_id(enum bank_type type, enum criterion criterion)
{
	short id = -1;

	if (type != BOOTABLE)
		return id;

	switch (criterion) {
	case BANK_ACTIVE:
		id = bankmgr_get_active_bank_id(type);
		break;
	case BANK_INACTIVE:
		id = get_other_bank_id(bankmgr_get_active_bank_id(type));
		break;
	case BANK_BOOTED:
		id = bankmgr_get_booted_bank_id();
		break;
	case BANK_NOTBOOTED:
		id = get_other_bank_id(bankmgr_get_booted_bank_id());
		break;
	default:
		break;
	}

	return id;
}

/*
 * bankmgr_get_infoblockdata -- returns the infoblockdata for a specific bank
 * The caller should free the pointer when done
 */
char * bankmgr_get_infoblockdata(short bank_id, size_t * length) {
	char volume_name[] = "kernelX";
	InfoBlockHeader ibHeader;
	char * buffer = NULL;
	struct ubi_volume_desc * desc;
	int leb = 0;
	*length = 0;
	volume_name[6] = 'A' + bank_id;

	desc = ubi_open_volume_nm(0, volume_name, UBI_READONLY);
	if (IS_ERR(desc)) {
		return NULL;
	}

	/* Infoblocks are stored in the last leb */
	while (ubi_is_mapped(desc, leb) == 1) leb++;

	if (leb == 0) {
		ubi_close_volume(desc);
		return NULL;
	}

	if (ubi_leb_read(desc, leb - 1, (char *)&ibHeader, 0, sizeof(ibHeader), 0) != 0) {
		ubi_close_volume(desc);
		return NULL;
	}
	ibHeader.size = ntohl(ibHeader.size);

	*length = ibHeader.size - 4;

	buffer = kmalloc(*length, GFP_KERNEL);
	if (buffer == NULL) {
		ubi_close_volume(desc);
		return NULL;
	}

	if (ubi_leb_read(desc, leb - 1, buffer, 4, *length, 0) != 0) {
		kfree(buffer);
		ubi_close_volume(desc);
		return NULL;
	}


	ubi_close_volume(desc);

	return buffer;
}


/* Not relevant on UBI platforms; no "legacy" has run before */
int load_legacy_upgrade_info(void)
{
	return -1;
}

/* Not relevant on UBI platforms; no "legacy" has run before */
void publish_legacy_upgrade_info(void) {}

/* Not relevant on UBI platforms; no "legacy" has run before */
void unpublish_legacy_upgrade_info(void) {}

/* Not relevant on UBI platforms; no "legacy" has run before */
void bankmgr_get_fallback_info(struct seq_file *m, short bank_id, unsigned int item_name) {}

