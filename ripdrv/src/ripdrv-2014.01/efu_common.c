/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
**                                                                      **
** Copyright (c) 2014-2017  -  Technicolor Delivery Technologies, SAS   **
** - All Rights Reserved                                                **
**
** This program is free software; you can redistribute it and/or modify **
** it under the terms of the GNU General Public License version 2 as    **
** published by the Free Software Foundation.                           **                                                       **
*************************************************************************/

#include "efu_common.h"
#include "efu_privileged.h"

int EFU_isEngineeringFeatureUnlocked(EFU_BITMASK_TYPE efu_feature_bit) {
    EFU_BITMASK_TYPE unlockedfeatures_bitmask;

    if (!(efu_feature_bit & EFU_SUPPORTEDFEATURES_BITMASK)) {
        /* This feature is not supported in this release */
        return false;
    }

    if (EFU_getBitmask(&unlockedfeatures_bitmask, EFU_DEFAULT_TAG) == EFU_RET_SUCCESS) {
        // Bitmask found and valid, now check field
        if (unlockedfeatures_bitmask & efu_feature_bit) {
            return true;
        }
        else {
            // bitmask valid, but bit not set
            return false;
        }
    }
    else {
        // Bitmask not found or not valid
        return false;
    }
}

