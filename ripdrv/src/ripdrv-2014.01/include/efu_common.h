/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
**                                                                      **
** Copyright (c) 2014  -  Technicolor Delivery Technologies, SAS        **
** All Rights Reserved                                                **
**
** This program is free software; you can redistribute it and/or modify **
** it under the terms of the GNU General Public License version 2 as    **
** published by the Free Software Foundation.                           ** 
*************************************************************************/

#ifndef __EFU_COMMON_H
#define __EFU_COMMON_H

#if defined(BUILDTYPE_cfe_bootloader)
#include "lib_types.h"
#else
#include <linux/types.h>
#endif

#define true    1
#define false   0

#define EFU_RET_SUCCESS       0
#define EFU_RET_ERROR         1
#define EFU_RET_BADSIG        2
#define EFU_RET_RIPERR        3
#define EFU_RET_BADTAG        4
#define EFU_RET_NOTIMPL       5
#define EFU_RET_PARSEERROR    6
#define EFU_RET_NOMEM         7
#define EFU_RET_DRVERR        8
#define EFU_RET_BUFTOOSMALL   9
#define EFU_RET_NOTFOUND     10

typedef uint64_t EFU_BITMASK_TYPE;
#define EFU_BITMASK_SIZE  8

typedef uint32_t EFU_CHIPID_TYPE;
#define EFU_CHIPID_SIZE   4

/*
 * There is both a permanent tag (RIP) and temporal tag.  By default, the temporal tag
 * takes precedence of the permanent tag.  Normally use EFU_DEFAULT_TAG, unless for
 * having an overview of which tags are stored
 */
#define EFU_DEFAULT_TAG                  0
#define EFU_PERMANENT_TAG                1
#define EFU_TEMPORAL_TAG                 2


/* 
 * This bitmask specifies which engineering features are supported in this release
 * It is not allowed to have gaps!!  So all 1s should be grouped together at the
 * least significant side of the variable
 */
#define EFU_SUPPORTEDFEATURES_BITMASK    0x000000000000000f

/*
 * This bitmask tells which of the supported features require a hash at the end of the tag
 */
#define EFU_REQUIREDHASHES_BITMASK       0x0000000000000000

/*
 * This is a list of defines to indicate which bit belongs to which engineering feature
 */
#define EFU_SKIPSIGNATURECHECK_BIT       0x0000000000000001
#define EFU_SKIPVARIANTIDCHECK_BIT       0x0000000000000002
#define EFU_DOWNGRADERESTRICTION_BIT     0x0000000000000004
#define EFU_ROOTSHELL_BIT                0x0000000000000008

#define EFU_SKIPSIGNATURECHECK_NAME      "skip_signature_check"
#define EFU_SKIPVARIANTIDCHECK_NAME      "skip_variantid_check"
#define EFU_DOWNGRADERESTRICTION_NAME    "skip_downgrade_restriction"
#define EFU_ROOTSHELL_NAME               "allow_root_shell_access"

/* Based on a valid tag stored in RIP */
int             EFU_isEngineeringFeatureUnlocked(EFU_BITMASK_TYPE efu_feature_bit);

#endif /* __EFU_COMMON_H */

