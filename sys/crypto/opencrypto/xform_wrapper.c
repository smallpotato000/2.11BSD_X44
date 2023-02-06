/*	$NetBSD: xform.c,v 1.13 2003/11/18 23:01:39 jonathan Exp $ */
/*	$FreeBSD: src/sys/opencrypto/xform.c,v 1.1.2.1 2002/11/21 23:34:23 sam Exp $	*/
/*	$OpenBSD: xform.c,v 1.19 2002/08/16 22:47:25 dhartmei Exp $	*/

/*
 * The authors of this code are John Ioannidis (ji@tla.org),
 * Angelos D. Keromytis (kermit@csd.uch.gr) and
 * Niels Provos (provos@physnet.uni-hamburg.de).
 *
 * This code was written by John Ioannidis for BSD/OS in Athens, Greece,
 * in November 1995.
 *
 * Ported to OpenBSD and NetBSD, with additional transforms, in December 1996,
 * by Angelos D. Keromytis.
 *
 * Additional transforms and features in 1997 and 1998 by Angelos D. Keromytis
 * and Niels Provos.
 *
 * Additional features in 1999 by Angelos D. Keromytis.
 *
 * Copyright (C) 1995, 1996, 1997, 1998, 1999 by John Ioannidis,
 * Angelos D. Keromytis and Niels Provos.
 *
 * Copyright (C) 2001, Angelos D. Keromytis.
 *
 * Permission to use, copy, and modify this software with or without fee
 * is hereby granted, provided that this entire notice is included in
 * all copies of any software which is or includes a copy or
 * modification of this software.
 * You may use this code under the GNU public license if you so wish. Please
 * contribute changes back to the authors under this freer than GPL license
 * so that we may further the use of strong encryption without limitations to
 * all.
 *
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTY. IN PARTICULAR, NONE OF THE AUTHORS MAKES ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE
 * MERCHANTABILITY OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR
 * PURPOSE.
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/sysctl.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/kernel.h>

#include <crypto/md5/md5.h>
#include <crypto/sha1/sha1.h>
#include <crypto/sha2/sha2.h>
#include <crypto/blowfish/blowfish.h>
#include <crypto/cast128/cast128.h>
#include <crypto/des/des.h>
#include <crypto/rijndael/rijndael.h>
#include <crypto/ripemd160/rmd160.h>
#include <crypto/skipjack/skipjack.h>

#include <crypto/opencrypto/deflate.h>
#include <crypto/opencrypto/cryptodev.h>
#include <crypto/opencrypto/xform.h>
#include <crypto/opencrypto/xform_wrapper.h>

/*
 * Encryption wrapper routines.
 */
void
null_encrypt(caddr_t key, u_int8_t *blk)
{

}

void
null_decrypt(caddr_t key, u_int8_t *blk)
{

}

int
null_setkey(u_int8_t **sched, const u_int8_t *key, int len)
{
	*sched = NULL;
	return 0;
}

void
null_zerokey(u_int8_t **sched)
{
	*sched = NULL;
}

void
des1_encrypt(caddr_t key, u_int8_t *blk)
{
	des_cblock *cb = (des_cblock *) blk;
	des_key_schedule *p = (des_key_schedule *) key;

	des_ecb_encrypt(cb, cb, p[0], DES_ENCRYPT);
}

void
des1_decrypt(caddr_t key, u_int8_t *blk)
{
	des_cblock *cb = (des_cblock *) blk;
	des_key_schedule *p = (des_key_schedule *) key;

	des_ecb_encrypt(cb, cb, p[0], DES_DECRYPT);
}

int
des1_setkey(u_int8_t **sched, const u_int8_t *key, int len)
{
	des_key_schedule *p;
	int err;

	MALLOC(p, des_key_schedule *, sizeof (des_key_schedule), M_CRYPTO_DATA, M_NOWAIT);
	if (p != NULL) {
		bzero(p, sizeof(des_key_schedule));
		des_set_key((des_cblock *) key, p[0]);
		err = 0;
	} else
		err = ENOMEM;
	*sched = (u_int8_t *) p;
	return err;
}

void
des1_zerokey(u_int8_t **sched)
{
	bzero(*sched, sizeof (des_key_schedule));
	FREE(*sched, M_CRYPTO_DATA);
	*sched = NULL;
}

void
des3_encrypt(caddr_t key, u_int8_t *blk)
{
	des_cblock *cb = (des_cblock *) blk;
	des_key_schedule *p = (des_key_schedule *) key;

	des_ecb3_encrypt(cb, cb, p[0], p[1], p[2], DES_ENCRYPT);
}

void
des3_decrypt(caddr_t key, u_int8_t *blk)
{
	des_cblock *cb = (des_cblock *) blk;
	des_key_schedule *p = (des_key_schedule *) key;

	des_ecb3_encrypt(cb, cb, p[0], p[1], p[2], DES_DECRYPT);
}

int
des3_setkey(u_int8_t **sched, const u_int8_t *key, int len)
{
	des_key_schedule *p;
	int err;

	MALLOC(p, des_key_schedule *, 3*sizeof (des_key_schedule), M_CRYPTO_DATA, M_NOWAIT);
	if (p != NULL) {
		bzero(p, 3*sizeof(des_key_schedule));
		des_set_key((des_cblock *)(key +  0), p[0]);
		des_set_key((des_cblock *)(key +  8), p[1]);
		des_set_key((des_cblock *)(key + 16), p[2]);
		err = 0;
	} else
		err = ENOMEM;
	*sched = (u_int8_t *) p;
	return err;
}

void
des3_zerokey(u_int8_t **sched)
{
	bzero(*sched, 3*sizeof (des_key_schedule));
	FREE(*sched, M_CRYPTO_DATA);
	*sched = NULL;
}

void
blf_encrypt(caddr_t key, u_int8_t *blk)
{
	BF_ecb_encrypt(blk, blk, (BF_KEY *)key, 1);
}

void
blf_decrypt(caddr_t key, u_int8_t *blk)
{

	BF_ecb_encrypt(blk, blk, (BF_KEY *)key, 0);
}

int
blf_setkey(u_int8_t **sched, const u_int8_t *key, int len)
{
	int err;

#define	BLF_SIZ	sizeof(BF_KEY)

	MALLOC(*sched, u_int8_t *, BLF_SIZ, M_CRYPTO_DATA, M_NOWAIT);
	if (*sched != NULL) {
		bzero(*sched, BLF_SIZ);
		BF_set_key((BF_KEY *) *sched, len, key);
		err = 0;
	} else
		err = ENOMEM;
	return err;
}

void
blf_zerokey(u_int8_t **sched)
{
	bzero(*sched, BLF_SIZ);
	FREE(*sched, M_CRYPTO_DATA);
	*sched = NULL;
}

void
cast5_encrypt(caddr_t key, u_int8_t *blk)
{
	cast128_encrypt((cast128_key *) key, blk, blk);
}

void
cast5_decrypt(caddr_t key, u_int8_t *blk)
{
	cast128_decrypt((cast128_key *) key, blk, blk);
}

int
cast5_setkey(u_int8_t **sched, const u_int8_t *key, int len)
{
	int err;

	MALLOC(*sched, u_int8_t *, sizeof(cast128_key), M_CRYPTO_DATA, M_NOWAIT);
	if (*sched != NULL) {
		bzero(*sched, sizeof(cast128_key));
		cast128_setkey((cast128_key *)*sched, key, len);
		err = 0;
	} else
		err = ENOMEM;
	return err;
}

void
cast5_zerokey(u_int8_t **sched)
{
	bzero(*sched, sizeof(cast128_key));
	FREE(*sched, M_CRYPTO_DATA);
	*sched = NULL;
}

void
skipjack_encrypt(caddr_t key, u_int8_t *blk)
{
	skipjack_forwards(blk, blk, (u_int8_t **) key);
}

void
skipjack_decrypt(caddr_t key, u_int8_t *blk)
{
	skipjack_backwards(blk, blk, (u_int8_t **) key);
}

int
skipjack_setkey(u_int8_t **sched, const u_int8_t *key, int len)
{
	int err;

	/* NB: allocate all the memory that's needed at once */
	/* XXX assumes bytes are aligned on sizeof(u_char) == 1 boundaries.
	 * Will this break a pdp-10, Cray-1, or GE-645 port?
	 */
	MALLOC(*sched, u_int8_t *, 10 * (sizeof(u_int8_t *) + 0x100), M_CRYPTO_DATA, M_NOWAIT);

	if (*sched != NULL) {

		u_int8_t** key_tables = (u_int8_t**) *sched;
		u_int8_t* table = (u_int8_t*) &key_tables[10];
		int k;

		bzero(*sched, 10 * sizeof(u_int8_t *)+0x100);

		for (k = 0; k < 10; k++) {
			key_tables[k] = table;
			table += 0x100;
		}
		subkey_table_gen(key, (u_int8_t **) *sched);
		err = 0;
	} else
		err = ENOMEM;
	return err;
}

void
skipjack_zerokey(u_int8_t **sched)
{
	bzero(*sched, 10 * (sizeof(u_int8_t *) + 0x100));
	FREE(*sched, M_CRYPTO_DATA);
	*sched = NULL;
}

void
rijndael128_encrypt(caddr_t key, u_int8_t *blk)
{
	rijndael_encrypt((rijndael_ctx *) key, (u_char *) blk, (u_char *) blk);
}

void
rijndael128_decrypt(caddr_t key, u_int8_t *blk)
{
	rijndael_decrypt((rijndael_ctx *) key, (u_char *) blk,
	    (u_char *) blk);
}

int
rijndael128_setkey(u_int8_t **sched, const u_int8_t *key, int len)
{
	int err;

	MALLOC(*sched, u_int8_t *, sizeof(rijndael_ctx), M_CRYPTO_DATA, M_WAITOK);
	if (*sched != NULL) {
		bzero(*sched, sizeof(rijndael_ctx));
		rijndael_set_key((rijndael_ctx *) *sched, key, len * 8);
		err = 0;
	} else
		err = ENOMEM;
	return err;
}

void
rijndael128_zerokey(u_int8_t **sched)
{
	bzero(*sched, sizeof(rijndael_ctx));
	FREE(*sched, M_CRYPTO_DATA);
	*sched = NULL;
}

/*
 * And now for auth.
 */

void
null_init(void *ctx)
{

}

int
null_update(void *ctx, const u_int8_t *buf, u_int16_t len)
{
	return 0;
}

void
null_final(u_int8_t *buf, void *ctx)
{
	if (buf != (u_int8_t *) 0) {
		bzero(buf, 12);
	}
}

int
RMD160Update_int(void *ctx, const u_int8_t *buf, u_int16_t len)
{
	RMD160Update(ctx, buf, len);
	return 0;
}

int
MD5Update_int(void *ctx, const u_int8_t *buf, u_int16_t len)
{
	MD5Update(ctx, buf, len);
	return 0;
}

void
SHA1Init_int(void *ctx)
{
	SHA1Init(ctx);
}

int
SHA1Update_int(void *ctx, const u_int8_t *buf, u_int16_t len)
{
	SHA1Update(ctx, buf, len);
	return 0;
}

void
SHA1Final_int(u_int8_t *blk, void *ctx)
{
	SHA1Final(blk, ctx);
}

int
SHA256Update_int(void *ctx, const u_int8_t *buf, u_int16_t len)
{
	SHA256_Update(ctx, buf, len);
	return 0;
}

int
SHA384Update_int(void *ctx, const u_int8_t *buf, u_int16_t len)
{
	SHA384_Update(ctx, buf, len);
	return 0;
}

int
SHA512Update_int(void *ctx, const u_int8_t *buf, u_int16_t len)
{
	SHA512_Update(ctx, buf, len);
	return 0;
}

/*
 * And compression
 */

u_int32_t
deflate_compress(data, size, out)
	u_int8_t *data;
	u_int32_t size;
	u_int8_t **out;
{
	return deflate_global(data, size, 0, out);
}

u_int32_t
deflate_decompress(data, size, out)
	u_int8_t *data;
	u_int32_t size;
	u_int8_t **out;
{
	return deflate_global(data, size, 1, out);
}
