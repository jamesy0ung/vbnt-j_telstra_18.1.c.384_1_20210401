/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
**                                                                      **
** Copyright (c)  2017 - Technicolor Delivery Technologies, SAS         **
** - All Rights Reserved                                                **
**
** This program is free software; you can redistribute it and/or modify **
** it under the terms of the GNU General Public License version 2 as    **
** published by the Free Software Foundation.                           **                                                       **
*************************************************************************/

#include "crypto_api.h"
#include "rip2.h"
#include "ripdrv.h"
#include "rip2_common.h"
#include "rip_ids.h"
#include "efu_privileged.h"

#include "rip2_crypto.h"

typedef enum{
    RIP2_REQ_REQUIRED,
    RIP2_REQ_OPTIONAL,
    RIP2_REQ_REQUIRED_UNLESS_EFU,

} T_RIP2_REQ;

static int rip2_crypto_check_item(T_RIP2_ID id,
                                  uint32_t  crypt_attr,
                                  T_RIP2_REQ requirements)
{
    T_RIP2_ITEM item;

    if (rip2_get_idx(id, &item) != RIP2_SUCCESS) {
        switch (requirements) {
        case RIP2_REQ_REQUIRED_UNLESS_EFU:
            if (EFU_verifyStoredTag() == EFU_RET_SUCCESS) {
                return RIP2_SUCCESS;
            }
            else {
                return RIP2_ERR_NOELEM;
            }
        case RIP2_REQ_OPTIONAL:
            return RIP2_SUCCESS;
        default:
            return RIP2_ERR_NOELEM;
        }
    }
    if ((~BETOH32(item.attr[ATTR_HI]) & RIP2_ATTR_CRYPTO) != crypt_attr) {
        return RIP2_ERR_BADCRYPTO;
    }

    return RIP2_SUCCESS;
}

int rip2_crypto_check(uint8_t *ripStart)
{
    T_RIP2_ITEM item, *it = NULL;

    /* Check whether signature of all EIK signed items are OK */
    while(rip2_get_next(&it, RIP2_ATTR_ANY, &item) == RIP2_SUCCESS) {
        if (BETOH32(item.attr[ATTR_HI]) & RIP2_ATTR_N_EIK_SIGN) {
            continue;
        }
        if (rip2_get_data(ripStart, BETOH16(item.ID), NULL) != RIP2_SUCCESS) {
            return RIP2_ERR_BADCRYPTO;
        }
    }

    if (rip2_crypto_check_item(RIP_ID_EIK, RIP2_ATTR_N_MCV_SIGN, RIP2_REQ_REQUIRED) != RIP2_SUCCESS) {
        return RIP2_ERR_BADCRYPTO;
    }

    if (rip2_crypto_check_item(RIP_ID_OSCK, RIP2_ATTR_N_EIK_SIGN , RIP2_REQ_REQUIRED) != RIP2_SUCCESS) {
        return RIP2_ERR_BADCRYPTO;
    }

    if (rip2_crypto_check_item(RIP_ID_RANDOM_KEY_A, RIP2_ATTR_N_EIK_SIGN , RIP2_REQ_REQUIRED) != RIP2_SUCCESS) {
        return RIP2_ERR_BADCRYPTO;
    }

    if (rip2_crypto_check_item(RIP_ID_RANDOM_KEY_B, RIP2_ATTR_N_EIK_SIGN , RIP2_REQ_REQUIRED) != RIP2_SUCCESS) {
        return RIP2_ERR_BADCRYPTO;
    }

    if (rip2_crypto_check_item(RIP_ID_OSIK, RIP2_ATTR_N_EIK_SIGN, RIP2_REQ_REQUIRED) != RIP2_SUCCESS) {
        return RIP2_ERR_BADCRYPTO;
    }

    if (rip2_crypto_check_item(RIP_ID_CLIENT_CERTIFICATE, RIP2_ATTR_N_EIK_SIGN , RIP2_REQ_REQUIRED_UNLESS_EFU) != RIP2_SUCCESS) {
        return RIP2_ERR_BADCRYPTO;
    }

    if (rip2_crypto_check_item(RIP_ID_OLYMPUS_IK, RIP2_ATTR_N_EIK_SIGN, RIP2_REQ_REQUIRED) != RIP2_SUCCESS) {
        return RIP2_ERR_BADCRYPTO;
    }

    if (rip2_crypto_check_item(RIP_ID_OLYMPUS_CK, RIP2_ATTR_N_EIK_SIGN , RIP2_REQ_REQUIRED) != RIP2_SUCCESS) {
        return RIP2_ERR_BADCRYPTO;
    }

    if (rip2_crypto_check_item(RIP_ID_GENERIC_ACCESS_KEY_LIST, RIP2_ATTR_N_EIK_SIGN , RIP2_REQ_REQUIRED) != RIP2_SUCCESS) {
        return RIP2_ERR_BADCRYPTO;
    }

    if (rip2_crypto_check_item(RIP_ID_RANDOM_KEY_C, RIP2_ATTR_N_EIK_SIGN , RIP2_REQ_REQUIRED) != RIP2_SUCCESS) {
        return RIP2_ERR_BADCRYPTO;
    }

    if (rip2_crypto_check_item(RIP_ID_RANDOM_KEY_D, RIP2_ATTR_N_EIK_SIGN , RIP2_REQ_REQUIRED) != RIP2_SUCCESS) {
        return RIP2_ERR_BADCRYPTO;
    }

    if (rip2_crypto_check_item(RIP_ID_CWPSL, RIP2_ATTR_N_EIK_SIGN , RIP2_REQ_REQUIRED) != RIP2_SUCCESS) {
        return RIP2_ERR_BADCRYPTO;
    }

    if (rip2_crypto_check_item(RIP_ID_CWEPL, RIP2_ATTR_N_EIK_SIGN , RIP2_REQ_REQUIRED) != RIP2_SUCCESS) {
        return RIP2_ERR_BADCRYPTO;
    }

    if (rip2_crypto_check_item(RIP_ID_CWPAL, RIP2_ATTR_N_EIK_SIGN , RIP2_REQ_REQUIRED) != RIP2_SUCCESS) {
        return RIP2_ERR_BADCRYPTO;
    }

    if (rip2_crypto_check_item(RIP_ID_CGAKL, RIP2_ATTR_N_EIK_SIGN , RIP2_REQ_REQUIRED) != RIP2_SUCCESS) {
        return RIP2_ERR_BADCRYPTO;
    }


    return RIP2_SUCCESS;
}

char mcv_prod[256] = {
  0xc1, 0xb7, 0x04, 0xf3, 0x98, 0xc8, 0x71, 0x47, 0x73, 0xce, 0x9d, 0x9c,
  0x7f, 0xfc, 0x84, 0x7e, 0x73, 0x04, 0x15, 0x74, 0xf1, 0x63, 0x88, 0x07,
  0x3a, 0x2a, 0x81, 0xfd, 0x63, 0xc5, 0x84, 0xcf, 0xb6, 0x31, 0xf9, 0x09,
  0xb2, 0xab, 0x74, 0x89, 0xca, 0xa5, 0x84, 0x90, 0x85, 0x82, 0xd0, 0xdc,
  0x99, 0x93, 0x4e, 0x64, 0x09, 0x0d, 0xd2, 0xdc, 0x8b, 0xde, 0xe0, 0xf2,
  0xd2, 0xe7, 0x1d, 0xc3, 0x89, 0x1d, 0xf7, 0x0e, 0xa1, 0xac, 0xed, 0xe9,
  0xab, 0xb9, 0x5f, 0x21, 0xd6, 0xcc, 0x4d, 0x59, 0x30, 0x1c, 0xaa, 0xa3,
  0x6e, 0xb1, 0xea, 0x1d, 0xd9, 0x74, 0x83, 0x03, 0x4e, 0xd3, 0x8f, 0xf5,
  0x25, 0x66, 0x73, 0xe8, 0xb2, 0xc2, 0xa3, 0x71, 0xd5, 0x45, 0x92, 0x5c,
  0xe9, 0x44, 0xbc, 0x61, 0x12, 0x8d, 0xa4, 0xd6, 0xa5, 0x57, 0xf2, 0x62,
  0x53, 0x9f, 0x22, 0x24, 0xca, 0x99, 0x23, 0xb5, 0x01, 0x05, 0x5c, 0x1d,
  0x3b, 0x7b, 0xd1, 0x8c, 0xb0, 0xd1, 0xaa, 0x57, 0xc8, 0x9d, 0x87, 0xf9,
  0x20, 0x31, 0xde, 0x9f, 0x70, 0xd0, 0xae, 0xc5, 0x5b, 0xc4, 0x20, 0x0a,
  0xa3, 0x41, 0x15, 0x7a, 0xb9, 0x7d, 0x6d, 0xf6, 0xc0, 0x79, 0x7a, 0xa8,
  0xb9, 0xee, 0xfb, 0xd9, 0xc1, 0x75, 0xcb, 0x95, 0xf1, 0x0f, 0x6c, 0xaa,
  0xe4, 0x44, 0x85, 0xa5, 0x60, 0xb8, 0xb5, 0x48, 0xf2, 0x3b, 0x3a, 0x3f,
  0x3d, 0x11, 0x30, 0xd1, 0xf3, 0x5f, 0x27, 0x81, 0x4f, 0x3a, 0x0a, 0x50,
  0x8a, 0xba, 0x60, 0x64, 0x6d, 0x64, 0xc2, 0xdb, 0xe3, 0xb9, 0xe3, 0xa8,
  0x99, 0x17, 0xc5, 0x01, 0x9c, 0x68, 0xd5, 0x66, 0x84, 0x28, 0x59, 0x02,
  0x07, 0xde, 0xa0, 0x6b, 0xc5, 0x65, 0x5b, 0x4c, 0x98, 0x55, 0x93, 0x40,
  0xce, 0x42, 0xfa, 0x5b, 0xc9, 0x5c, 0xc9, 0xa1, 0x84, 0xda, 0x16, 0x64,
  0xd8, 0x55, 0x8a, 0xb5
};


int rip2_crypto_init(uint8_t *ripStart)
{
    static int init_done = 0;

    if (init_done) {
        return RIP2_SUCCESS;
    }
    DBG("\r\neRIPv2 crypto module init\r\n");

#ifndef CONFIG_RIPDRV_INTEGRITY_ONLY
    bek.key  = 0;
    eck.key  = 0;
#endif
    mcv.key  = 0;
    eik.key  = 0;


#ifndef CONFIG_RIPDRV_INTEGRITY_ONLY
    bek.length = 16;
    bek.key    = bootArgs->encrArgs.bek;  //TODO BEK Source?
    bek.iv     = 0;
#endif

    mcv.length = 256;

    mcv.key    = mcv_prod;

    unsigned char tmp[256 + 256 + 16]; /* Allocate for data + cryptopadding + signature */
    uint32_t length = 0;
    int allok = 1;

    /* Retrieve EIK, which should be a 2048 bit RSA public key modulus */
    if (rip2_get_data_ex(ripStart, RIP_ID_EIK, tmp, &length, 0) == RIP2_SUCCESS) {
        rip2_crypto_import_eik(tmp, length);
    }
    else {
        allok = 0;
    }

#ifndef CONFIG_RIPDRV_INTEGRITY_ONLY
    /* Retrieve ECK, which should be a 128 bit AES key */
    if (rip2_get_data_ex(ripStart, RIP_ID_ECK, tmp, &length, 0) == RIP2_SUCCESS) {
        rip2_crypto_import_eck(tmp, length);
    }
    else {
        allok = 0;
    }
#endif

    /* All secure boot secrets are no longer needed.  In case of the production scenario, we need the keys */
#if 0
    if (allok || !check_MBH_Enabled()) {
        secureboot_gopublic(0);
    }
    else {
        init_done = 0;
    }
#endif
    init_done = allok;

    return RIP2_SUCCESS;
}


