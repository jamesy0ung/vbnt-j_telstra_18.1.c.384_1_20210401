/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
** Copyright (c) 2017 - 2017  -  Technicolor Delivery Technologies, SAS **
** - All Rights Reserved                                                **
**
** This program is free software; you can redistribute it and/or modify **
** it under the terms of the GNU General Public License version 2 as    **
** published by the Free Software Foundation.                           **                                                       **
*************************************************************************/

/*
 * crypto_fixup.c
 *
 *  Created on: Jul 24, 2017
 *      Author: geertsn
 */


#include <tomcrypt.h>
#include <tommath.h>

DECLARE_GLOBAL_DATA_PTR;

//
// MACROS
//
#define RELOC(to, from, param) \
    do { if (from.param) \
        to.param = from.param + gd->reloc_off; \
    } \
    while (0)

#define RELOC_LTM(param) RELOC(ltc_mp, ltm_desc, param)

#define RELOC_PTR(to, from, param) \
    do { if (from->param) \
        to.param = from->param + gd->reloc_off; \
    } \
    while (0)

#define RELOC_CIPHER(param) RELOC_PTR(c, cipher, param)
#define RELOC_HASH(param)   RELOC_PTR(h, hash, param)
#define RELOC_PRNG(param)   RELOC_PTR(p, prng, param)

/**
 * This function will take in struct of type ltc_cipher_descriptor and fixup all pointers
 * before registering it to tomcrypt if needed
 */
int tch_register_cipher(const struct ltc_cipher_descriptor *cipher)
{
#if ((UBOOT_VERSION < 2010 || (UBOOT_VERSION == 2010 && UBOOT_PATCHLEVEL < 12)) && !defined(CONFIG_RELOC_FIXUP_WORKS)) \
    || ((UBOOT_VERSION > 2010) && defined(CONFIG_NEEDS_MANUAL_RELOC))
    struct ltc_cipher_descriptor c; // local var, the register_cipher function will copy it anyways

    RELOC_CIPHER(name);
    c.ID = cipher->ID;
    c.min_key_length = cipher->min_key_length;
    c.max_key_length = cipher->max_key_length;
    c.block_length   = cipher->block_length;
    c.default_rounds = cipher->default_rounds;

    RELOC_CIPHER(setup);
    RELOC_CIPHER(ecb_encrypt);
    RELOC_CIPHER(ecb_decrypt);
    RELOC_CIPHER(test);
    RELOC_CIPHER(done);
    RELOC_CIPHER(keysize);
    RELOC_CIPHER(accel_ecb_encrypt);
    RELOC_CIPHER(accel_ecb_decrypt);
    RELOC_CIPHER(accel_cbc_encrypt);
    RELOC_CIPHER(accel_cbc_decrypt);
    RELOC_CIPHER(accel_ctr_encrypt);
    RELOC_CIPHER(accel_lrw_encrypt);
    RELOC_CIPHER(accel_lrw_decrypt);
    RELOC_CIPHER(accel_ccm_memory);
    RELOC_CIPHER(accel_gcm_memory);
    RELOC_CIPHER(omac_memory);
    RELOC_CIPHER(xcbc_memory);
    RELOC_CIPHER(f9_memory);
    RELOC_CIPHER(accel_xts_encrypt);
    RELOC_CIPHER(accel_xts_decrypt);


    return register_cipher(&c);
#else
    return register_cipher(cipher);
#endif
}

/**
 * This function will take in struct of type ltc_hash_descriptor and fixup all pointers
 * before registering it to tomcrypt if needed
 */
int tch_register_hash(const struct ltc_hash_descriptor *hash)
{
#if ((UBOOT_VERSION < 2010 || (UBOOT_VERSION == 2010 && UBOOT_PATCHLEVEL < 12)) && !defined(CONFIG_RELOC_FIXUP_WORKS)) \
    || ((UBOOT_VERSION > 2010) && defined(CONFIG_NEEDS_MANUAL_RELOC))
    struct ltc_hash_descriptor h;

    RELOC_HASH(name);

    h.ID        = hash->ID;
    h.hashsize  = hash->hashsize;
    h.blocksize = hash->blocksize;
    memcpy(h.OID, hash->OID, sizeof(h.OID));
    h.OIDlen    = hash->OIDlen;

    RELOC_HASH(init);
    RELOC_HASH(process);
    RELOC_HASH(done);
    RELOC_HASH(test);
    RELOC_HASH(hmac_block);

    return register_hash(&h);
#else
    return register_hash(hash);
#endif
}

/**
 * This function will fixup all pointers of ltm_desc (libtommath descriptor)
 * before copying it to tomcrypt if needed
 */
void tch_register_tommath(void)
{
#if ((UBOOT_VERSION < 2010 || (UBOOT_VERSION == 2010 && UBOOT_PATCHLEVEL < 12)) && !defined(CONFIG_RELOC_FIXUP_WORKS)) \
    || ((UBOOT_VERSION > 2010) && defined(CONFIG_NEEDS_MANUAL_RELOC))

    RELOC_LTM(name);
    ltc_mp.bits_per_digit = ltm_desc.bits_per_digit;

    RELOC_LTM(init);
    RELOC_LTM(init_copy);
    RELOC_LTM(deinit);
    RELOC_LTM(neg);
    RELOC_LTM(copy);
    RELOC_LTM(set_int);
    RELOC_LTM(get_int);
    RELOC_LTM(get_digit);
    RELOC_LTM(get_digit_count);
    RELOC_LTM(compare);
    RELOC_LTM(compare_d);
    RELOC_LTM(count_bits);
    RELOC_LTM(count_lsb_bits);
    RELOC_LTM(twoexpt);
    RELOC_LTM(read_radix);
    RELOC_LTM(write_radix);
    RELOC_LTM(unsigned_size);
    RELOC_LTM(unsigned_write);
    RELOC_LTM(unsigned_read);
    RELOC_LTM(add);
    RELOC_LTM(addi);
    RELOC_LTM(sub);
    RELOC_LTM(subi);
    RELOC_LTM(mul);
    RELOC_LTM(muli);
    RELOC_LTM(sqr);
    RELOC_LTM(mpdiv);
    RELOC_LTM(div_2);
    RELOC_LTM(modi);
    RELOC_LTM(gcd);
    RELOC_LTM(lcm);
    RELOC_LTM(mulmod);
    RELOC_LTM(sqrmod);
    RELOC_LTM(invmod);
    RELOC_LTM(montgomery_setup);
    RELOC_LTM(montgomery_normalization);
    RELOC_LTM(montgomery_reduce);
    RELOC_LTM(montgomery_deinit);
    RELOC_LTM(exptmod);
    RELOC_LTM(isprime);
    RELOC_LTM(ecc_ptmul);
    RELOC_LTM(ecc_ptadd);
    RELOC_LTM(ecc_ptdbl);
    RELOC_LTM(ecc_map);
    RELOC_LTM(ecc_mul2add);
    RELOC_LTM(rsa_keygen);
    RELOC_LTM(rsa_me);
    RELOC_LTM(addmod);
    RELOC_LTM(submod);
    RELOC_LTM(rand);
#else
    ltc_mp = ltm_desc;
#endif
}
