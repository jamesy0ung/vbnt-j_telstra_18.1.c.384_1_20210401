/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
**                                                                      **
** Copyright (c) 2014  -  Technicolor Delivery Technologies, SAS        **
** All Rights Reserved                                                **
**
** This program is free software; you can redistribute it and/or modify **
** it under the terms of the GNU General Public License version 2 as    **
** published by the Free Software Foundation.                           ** 
*************************************************************************/

#include "efu_common.h"

/* Store a new tag */
unsigned int    EFU_verifyStoredTag(void);
unsigned int    EFU_storeTemporalTag(unsigned char * unlockTag_a, unsigned long unlockTag_size);

/* Get information */
EFU_CHIPID_TYPE EFU_getChipid(void);
unsigned int    EFU_getOSIK(unsigned char * pubkey, unsigned long pubkey_length);
char*           EFU_getSerialNumber(void); // WARNING: returns string that needs to be freed after use!!
unsigned int    EFU_getBitmask(EFU_BITMASK_TYPE *bitmask_p, int tag_location);

