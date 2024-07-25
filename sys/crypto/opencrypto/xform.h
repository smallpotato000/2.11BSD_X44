/*	$NetBSD: xform.h,v 1.21 2020/06/30 04:14:56 riastradh Exp $ */
/*	$FreeBSD: src/sys/opencrypto/xform.h,v 1.1.2.1 2002/11/21 23:34:23 sam Exp $	*/
/*	$OpenBSD: xform.h,v 1.10 2002/04/22 23:10:09 deraadt Exp $	*/

/*
 * The author of this code is Angelos D. Keromytis (angelos@cis.upenn.edu)
 *
 * This code was written by Angelos D. Keromytis in Athens, Greece, in
 * February 2000. Network Security Technologies Inc. (NSTI) kindly
 * supported the development of this code.
 *
 * Copyright (c) 2000 Angelos D. Keromytis
 *
 * Permission to use, copy, and modify this software with or without fee
 * is hereby granted, provided that this entire notice is included in
 * all source code copies of any software which is or includes a copy or
 * modification of this software.
 *
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTY. IN PARTICULAR, NONE OF THE AUTHORS MAKES ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE
 * MERCHANTABILITY OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR
 * PURPOSE.
 */

#ifndef _CRYPTO_XFORM_H_
#define _CRYPTO_XFORM_H_

/* Declarations */
struct auth_hash {
	int 		type;
	const char 	*name;
	u_int16_t 	keysize;
	u_int16_t 	hashsize;
	u_int16_t 	authsize;
	u_int16_t 	blocksize;
	u_int16_t 	ctxsize;
	void 		(*Init)(void *);
	void 		(*Setkey)(void *, const uint8_t *, uint16_t);
	void 		(*Reinit)(void *, const uint8_t *, uint16_t);
	int  		(*Update)(void *, const u_int8_t *, u_int16_t);
	void 		(*Final)(u_int8_t *, void *);
};

/* Provide array-limit for clients (e.g., netipsec) */
#define	AH_ALEN_MAX	32	/* max authenticator hash length */

struct enc_xform {
	int 		type;
	const char 	*name;
	u_int16_t 	blocksize;
	u_int16_t 	ivsize;
	u_int16_t 	minkey;
	u_int16_t 	maxkey;
	void 		(*encrypt)(caddr_t, u_int8_t *);
	void 		(*decrypt)(caddr_t, u_int8_t *);
	int 		(*setkey)(u_int8_t **, const u_int8_t *, int len);
	void 		(*zerokey)(u_int8_t **);
	void 		(*reinit)(void *, const uint8_t *, uint8_t *);
};

struct comp_algo {
	int 		type;
	const char 	*name;
	size_t 		minlen;
	u_int32_t 	(*compress)(u_int8_t *, u_int32_t, u_int8_t **);
	u_int32_t 	(*decompress)(u_int8_t *, u_int32_t, u_int8_t **);
};

extern const u_int8_t hmac_ipad_buffer[128];
extern const u_int8_t hmac_opad_buffer[128];

extern const struct enc_xform enc_xform_null;
extern const struct enc_xform enc_xform_des;
extern const struct enc_xform enc_xform_3des;
extern const struct enc_xform enc_xform_blf;
extern const struct enc_xform enc_xform_cast5;
extern const struct enc_xform enc_xform_skipjack;
extern const struct enc_xform enc_xform_rijndael128;
extern const struct enc_xform enc_xform_aes;
extern const struct enc_xform enc_xform_arc4;
extern const struct enc_xform enc_xform_camellia;
extern const struct enc_xform enc_xform_twofish;
extern const struct enc_xform enc_xform_twofish_xts;
extern const struct enc_xform enc_xform_serpent;
extern const struct enc_xform enc_xform_serpent_xts;
extern const struct enc_xform enc_xform_mars;
extern const struct enc_xform enc_xform_aes_ctr;
extern const struct enc_xform enc_xform_aes_xts;
extern const struct enc_xform enc_xform_aes_gcm;
extern const struct enc_xform enc_xform_aes_gmac;

extern const struct auth_hash auth_hash_null;
extern const struct auth_hash auth_hash_md5;
extern const struct auth_hash auth_hash_sha1;
extern const struct auth_hash auth_hash_key_md5;
extern const struct auth_hash auth_hash_key_sha1;
extern const struct auth_hash auth_hash_hmac_md5;
extern const struct auth_hash auth_hash_hmac_sha1;
extern const struct auth_hash auth_hash_hmac_ripemd_160;
extern const struct auth_hash auth_hash_hmac_md5_96;
extern const struct auth_hash auth_hash_hmac_sha1_96;
extern const struct auth_hash auth_hash_hmac_ripemd_160_96;
extern const struct auth_hash auth_hash_hmac_sha2_256;
extern const struct auth_hash auth_hash_hmac_sha2_384;
extern const struct auth_hash auth_hash_hmac_sha2_512;
extern const struct auth_hash auth_hash_aes_xcbc_mac_96;
extern const struct auth_hash auth_hash_gmac_aes_128;
extern const struct auth_hash auth_hash_gmac_aes_192;
extern const struct auth_hash auth_hash_gmac_aes_256;

extern const struct comp_algo comp_algo_deflate;

#ifdef _KERNEL
#include <sys/malloc.h>
#endif

#endif /* _CRYPTO_XFORM_H_  */
