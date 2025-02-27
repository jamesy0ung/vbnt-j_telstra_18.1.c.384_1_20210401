#if 0
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/mm.h>
#include <linux/random.h>
#endif

#include <tomcrypt.h>
#include <tommath.h>

#include "crypto_api.h"

typedef struct keyInstanceV {
	symmetric_CBC cbc;
	int direction;
	int keysize;
} keyInstance;

typedef struct aes_crypto_ctxt {
  int direction;
  keyInstance key_inst[2];
} aes_crypto_ctxt_t; /* keyInstance for decrypt and/or encrypt  direction;
			Note that if unidirectional, then only first keyInstance is valid */

typedef struct rsa_crypto_ctxt {
    int public;
    mp_int  N, E;
    rsa_key key;
    int     bits;
} rsa_crypto_ctxt_t;

typedef struct {
    hash_state state;
    int hash_idx;
} SHA256_CTX;

extern int tch_register_cipher(const struct ltc_cipher_descriptor *cipher);
extern int tch_register_hash(const struct ltc_hash_descriptor *hash);
extern void tch_register_tommath(void);

#ifdef SHA256_HW
struct hw_sha256_ctx {
	uint32_t data_addr;
	uint32_t data_len;
	int data_left;
	uint32_t data_read_offset;
        uint32_t H[8];
};
extern int hw_sha256_init(struct hw_sha256_ctx *ctx);
extern int hw_sha256_update(void *ctxp,const void *buf, unsigned int size, int is_last);
extern int hw_sha256_final(void *ctxp,void *dest_buf, int size);
#endif

int aes_create_crypto(unsigned char *iv, unsigned char *key, int key_len_bits, void *crypto_ctx, int size_of_ctx)
{
  static int aes_tomcrypt_init_done = 0;
  keyInstance * kInst = ((aes_crypto_ctxt_t *)crypto_ctx)->key_inst;

  if (aes_tomcrypt_init_done == 0) {
    if (tch_register_cipher(&aes_desc) == -1) {
      return -1;
    }
    aes_tomcrypt_init_done = 1;
  }

  if (size_of_ctx < (sizeof(int) + sizeof(keyInstance)))
    return -1;

  kInst->keysize = key_len_bits / 8;
  return (cbc_start(find_cipher_id(aes_desc.ID), iv, key, kInst->keysize, 0, &(kInst->cbc)) == CRYPT_OK)? 0 : -1;
}

int aes_encrypt_blocks(void *crypto_ctx, unsigned char *idata, unsigned int numBytes, unsigned char *odata)
{
  keyInstance * kInst = ((aes_crypto_ctxt_t *)crypto_ctx)->key_inst;

  return (cbc_encrypt(idata, odata, numBytes, &kInst->cbc) == CRYPT_OK)? 0 : -1;
}

int aes_decrypt_blocks(void *crypto_ctx, unsigned char *idata, unsigned int numBytes, unsigned char *odata)
{
  keyInstance * kInst = ((aes_crypto_ctxt_t *)crypto_ctx)->key_inst;

  return (cbc_decrypt(idata, odata, numBytes, &kInst->cbc) == CRYPT_OK)? 0 : -1;
}

int aes_destroy_crypto(void *crypto_ctx, int size_of_ctx)
{
  memset(crypto_ctx, 0, size_of_ctx);
  return 0;
}

int rsa_create_crypto(unsigned char *key,
                      int           keyformat,
                      void          *crypto_ctx,
                      int           size_of_ctx)
{
	rsa_crypto_ctxt_t *ctx = (rsa_crypto_ctxt_t *)crypto_ctx;

    if (size_of_ctx < (sizeof(rsa_crypto_ctxt_t))) {
        return -1;
    }

    tch_register_tommath();

    switch (keyformat) {
        case RSA_KEYFORMAT_2048_PUBLICMODULUSONLY:
        case RSA_KEYFORMAT_1024_PUBLICMODULUSONLY:
            mp_init(&ctx->N);
            mp_init(&ctx->E);
            ctx->key.N = &ctx->N;
            ctx->key.e = &ctx->E;
            ctx->bits = (keyformat == RSA_KEYFORMAT_2048_PUBLICMODULUSONLY) ? 2048 : 1024;
            mp_read_unsigned_bin(&ctx->N, key, ctx->bits / 8);
            mp_set_int(&ctx->E, 0x10001);
            ctx->public = 1;
            break;

        default:
            return -1;

            break;
    }

    return 0;
}

int rsa_delete_crypto(void *crypto_ctx)
{
    rsa_crypto_ctxt_t *ctx = (rsa_crypto_ctxt_t *)crypto_ctx;

    if (ctx->public) {
        mp_clear(&ctx->N);
        mp_clear(&ctx->E);
    }
    return 0;
}

int rsa_verifysig_pss(void          *crypto_ctx,
                      unsigned char *sig,
                      int           siglen,
                      unsigned char *hash,
                      int           hashlen)
{
    int hash_idx = 0;
    int res;
    rsa_crypto_ctxt_t *rsa_ctx = (rsa_crypto_ctxt_t *)crypto_ctx;

#ifdef SHA256_HW
    /* Only the SHA2 value is calculated using the hardware. In order to decode the OAEP signature, Alpine-db
     * uses the default tomcrypt implementation. Hence rgistering the sha2 as below */
    if ( (hash_idx = tch_register_hash(&sha256_desc)) == -1)
    {
      return -1;
    }
#else
    if ( (hash_idx = find_hash_id(sha256_desc.ID)) == -1 )
    {
	return -1;
    }
#endif

    /* We need a public key for verifying a signature */
    if (!rsa_ctx->public) {
        return -1;
    }

    rsa_verify_hash(sig, siglen, hash, hashlen, hash_idx, hashlen, &res, &rsa_ctx->key);
    return res;
}

int sha256_update_ctx(void *hash_ctx, unsigned char *data, unsigned int numBytes) {
#ifdef SHA256_HW
  hw_sha256_update(hash_ctx, data, numBytes, 1);
#else
  hash_descriptor[((SHA256_CTX *)hash_ctx)->hash_idx].process(& ((SHA256_CTX *)hash_ctx)->state, data, numBytes);
#endif
  return 0;
}

int sha256_final_gen_digest(void *hash_ctx, unsigned char *digest) {
#ifdef SHA256_HW
  hw_sha256_final(hash_ctx, digest, 32) ;
#else
  hash_descriptor[((SHA256_CTX *)hash_ctx)->hash_idx].done(& ((SHA256_CTX *)hash_ctx)->state, digest);
#endif
  return 0;
}

int sha256_duplicate_ctx(void *orig_hash_ctx, void *new_hash_ctx, int size_of_ctx)
{
  memcpy(new_hash_ctx, orig_hash_ctx, size_of_ctx);
  return 0;
}

int sha256_delete_ctx(void *hash_ctx)
{
  return 0;
}

int sha256_init_ctx(void *hash_ctx, int size_of_ctx)
{
#ifdef SHA256_HW
  hw_sha256_init((struct hw_sha256_ctx *)hash_ctx);
#else
  static int hash_idx = -1;
  if (size_of_ctx < sizeof(SHA256_CTX)) {
    return -1;
  }

  if (hash_idx == -1) {
    if (tch_register_hash(&sha256_desc) == -1) {
      return -1;
    }
    hash_idx = find_hash_id(sha256_desc.ID);
    if (hash_idx == -1) {
      return -1;
    }
  }

  ((SHA256_CTX *)hash_ctx)->hash_idx = hash_idx;
  hash_descriptor[((SHA256_CTX *)hash_ctx)->hash_idx].init(& ((SHA256_CTX *)hash_ctx)->state);
#endif
  return 0;
}


int rsa_verifysig_pss_sha256(void           *crypto_ctx,
                             unsigned char  *sig,
                             int            siglen,
                             unsigned char  *data,
                             int            datalen)
{
    unsigned char hash[32];
    int           hashlen = sizeof(hash);

    char          sha_ctx[CAPI_MAX_AUTH_CTX_SIZE];
    sha256_init_ctx((void *)sha_ctx, sizeof(sha_ctx));
    sha256_update_ctx(sha_ctx, data, (unsigned int)datalen);
    sha256_final_gen_digest(sha_ctx, hash);
    sha256_delete_ctx(sha_ctx);
    return rsa_verifysig_pss(crypto_ctx, sig, siglen, hash, hashlen);
}

