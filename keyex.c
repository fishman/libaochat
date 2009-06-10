/*
 * $Id: keyex.c,v 1.2 2003/06/17 13:29:23 pickett Exp $
 *
 * AOChat -- library for talking with the Anarchy Online chat servers
 * Copyright (c) 2002, 2003 Oskari Saarenmaa <pickett@sumu.org>.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 *
 *
 */

/*
 * based on the PHP class I wrote some time ago.. this C-port is
 * quite unfinished, the only thing it can do at the moment is generate
 * the required login keys (onesided Diffie-Hellman key exchange with
 * TEA encrypted message body)
 *
 * DH:  http://www.rsasecurity.com/rsalabs/faq/3-6-1.html
 * TEA: http://www.ftp.cl.cam.ac.uk/ftp/papers/djw-rmn/djw-rmn-tea.html
 *
 */

/*
	This file has been heavily modified :P
	* Removed assert()'s
	* Merged with "misc.c" and removed unused functions (from Oskari Saarenmaa)
	* Merged with "keyex.h" (from Oskari Saarenmaa)
	* Renamed functions, changed coding style to match libaochat code
	* Added OpenSSL BIGNUM code
	* Greatly improved the random number generator...
	  The original random function could only generate 1000000 unique random
	  sequences, making brute force attacks possible.
	* Win32 compatibility changes

	Get the original source code at: http://auno.org/ao/chat/
*/

#include "aochat.h"
#ifdef HAVE_LIBCRYPTO
	#include <openssl/bn.h>
	#include <openssl/crypto.h>
#else
	#include <gmp.h>
#endif

#ifdef __BIG_ENDIAN__
#define SwapIfBe32(a) ((uint32_t)((a&0xFF000000)>>24)| ((a&0x00FF0000)>>8) | ((a&0x0000FF00)<<8) | ((a&0xFF)<<24))
#else
#define SwapIfBe32(a) (a)
#endif

static int aocHtoi(int c)
{
	if(c >= '0' && c <= '9') return c - 48;
	if(c >= 'a' && c <= 'f') return c - 87;
	if(c >= 'A' && c <= 'F') return c - 55;

	return 0;
}


static unsigned char *aocRandomHex32()
{
	static char buf[33];
	uint32_t rnd[4];

	aocRandom((unsigned char *)rnd, 16);
	sprintf(buf, "%08x%08x%08x%08x", (int)rnd[0], (int)rnd[1], (int)rnd[2], (int)rnd[3]);

	return buf;
}


#ifdef HAVE_LIBCRYPTO

unsigned char *aocTrimLeadingZero(unsigned char *ptr)
{
	unsigned char *tmp;

	if(*ptr == '0')
	{
		tmp = ptr;

		ptr = OPENSSL_malloc(strlen(ptr));
		strcpy(ptr, tmp + 1);

		OPENSSL_free(tmp);
	}

	return ptr;
}

/* Diffie-Hellman using OpenSSL's BIGNUM functions */
static int aocDoDiffieHellman(char **r_key_a, char **r_key_b)
{
	BIGNUM *pkey1 = NULL, *pkey2 = NULL;
	BIGNUM *prime = NULL, *secret = NULL, *res = NULL;
	BN_CTX *ctx = NULL;
	char *key_a = NULL, *key_b = NULL;
	int retval = 0;


	/* set public keys */
	pkey1 = BN_new();
	if(BN_hex2bn(&pkey1, _aoc_pref.pkey1) == 0)
		goto dh_error;

	pkey2 = BN_new();
	if(BN_hex2bn(&pkey2, _aoc_pref.pkey2) == 0)
		goto dh_error;

	/* prime number */
	prime = BN_new();
	if(BN_dec2bn(&prime, "5") == 0)
		goto dh_error;

	/* generate a 128bit random number (this is our secret key) */
	secret = BN_new();
	if(BN_hex2bn(&secret, aocRandomHex32()) == 0)
		goto dh_error;


	/* init result & ctx */
	res = BN_new();
	ctx = BN_CTX_new();


	/* calculate: res = pow(prime, secret) % pkey1
	   store it in key_a, and remove any leading zeroes */
	if(BN_mod_exp(res, prime, secret, pkey1, ctx) == 0)
		goto dh_error;

	/* save result in key_a */
	key_a = BN_bn2hex(res);
	key_a = aocTrimLeadingZero(key_a);
	aocLowerCase(key_a);


	/* calculate: res = pow(pkey2, secret) % pkey1
	   store up to 32 chars in key_b */
	if(BN_mod_exp(res, pkey2, secret, pkey1, ctx) == 0)
		goto dh_error;

	key_b = BN_bn2hex(res);
	key_b = aocTrimLeadingZero(key_b);
	if(strlen(key_b) > 32) key_b[32] = 0;
	aocLowerCase(key_b);	

	/* save result pointers */
	*r_key_a = key_a;
	*r_key_b = key_b;

	retval = 1;

dh_error:;
	if(pkey1  != NULL) BN_free(pkey1);
	if(pkey2  != NULL) BN_free(pkey2);
	if(prime  != NULL) BN_free(prime);
	if(secret != NULL) BN_free(secret);
	if(res    != NULL) BN_free(res);
	if(ctx    != NULL) BN_CTX_free(ctx);

	if(retval == 0)
	{
		if(key_a  != NULL) OPENSSL_free(key_a);
		if(key_b  != NULL) OPENSSL_free(key_b);
	}

	return retval;
}

#else

/* Diffie-Hellman using GNU MP (Multiple Precision Arithmetic Library) */
static int aocDoDiffieHellman(char **r_key_a, char **r_key_b)
{
	mpz_t pkey1 = {{0}}, pkey2 = {{0}};
	mpz_t prime ={{0}}, secret = {{0}}, res = {{0}};
	char *key_a = NULL, *key_b = NULL;
	int retval = 0;


	/* set public keys */
	if(mpz_init_set_str(pkey1, _aoc_pref.pkey1, 16) != 0)
		goto dh_error;

	if(mpz_init_set_str(pkey2, _aoc_pref.pkey2, 16) != 0)
		goto dh_error;

	/* any prime number */
	mpz_init_set_ui(prime, 5);

	/* generate a 128bit random number (this is our secret key) */
	if(mpz_init_set_str(secret, aocRandomHex32(), 16) != 0)
		goto dh_error;


	/* init result */
	mpz_init(res);

	/*	calculate: res = pow(prime, secret) % pkey1
		store it in key_a, and remove any leading zeroes */
	mpz_powm(res, prime, secret, pkey1);
	gmp_asprintf(&key_a, "%Zx", res);

	/*	calculate: res = pow(pkey2, secret) % pkey1
		store up to 32 chars in key_b */
	mpz_powm(res, pkey2, secret, pkey1);
	key_b = malloc(33);
	gmp_snprintf(key_b, 33, "%Zx", res);


	/* save result pointers */
	*r_key_a = key_a;
	*r_key_b = key_b;

	retval = 1;

dh_error:;
	if(pkey1  != NULL) mpz_clear(pkey1);
	if(pkey2  != NULL) mpz_clear(pkey2);
	if(prime  != NULL) mpz_clear(prime);
	if(secret != NULL) mpz_clear(secret);
	if(res    != NULL) mpz_clear(res);

	if(retval == 0)
	{
		if(key_a  != NULL) free(key_a);
		if(key_b  != NULL) free(key_b);
	}

	return retval;
}

#endif


static void aocKeyexTeaEncrypt(uint32_t cycle[4], uint32_t key[4])
{ 
	uint32_t a = cycle[0];
	uint32_t b = cycle[1];
	uint32_t sum = 0;
	uint32_t delta = 0x9e3779b9;
	uint32_t i = 32;

	while(i--)
	{
		sum += delta;
		a += ((b << 4 & 0xfffffff0) + key[0]) ^ (b + sum) ^ ((b >> 5 & 0x7ffffff) + key[1]);
		b += ((a << 4 & 0xfffffff0) + key[2]) ^ (a + sum) ^ ((a >> 5 & 0x7ffffff) + key[3]);
	}

	cycle[2] = a;
	cycle[3] = b;

	return;
}


static char *aocKeyexCrypt(char *key, char *str, int len)
{ 
	uint32_t i, cycle[4], akey[4];
	char *crypted, *p;

	/* Our return buffer, and pointer to it */
	p = crypted = malloc(len*2 + 1);

	/* Zero the cycle */
	memset(cycle, 0, sizeof(cycle));

	/* Convert the hexadecimal key to binary */
	for(i=0; i<16; i++)
		*((char*)&akey+i) = aocHtoi(key[2*i])*16 + aocHtoi(key[2*i+1]);

#ifdef __BIG_ENDIAN__
  for(i = 0; i<4 ; i++)
  {
    akey[i] = SwapIfBe32(akey[i]);
  }
#endif

		for(i=0; i<len/4;)
		{
#ifdef __BIG_ENDIAN__
	    cycle[0] = (*(uint32_t*)(str+4*i++)) ^ SwapIfBe32(cycle[2]);
	    cycle[1] = (*(uint32_t*)(str+4*i++)) ^ SwapIfBe32(cycle[3]);
			cycle[0] = SwapIfBe32(cycle[0]);
			cycle[1] = SwapIfBe32(cycle[1]);
#else
	    cycle[0] = (*(uint32_t*)(str+4*i++)) ^ (cycle[2]);
	    cycle[1] = (*(uint32_t*)(str+4*i++)) ^ (cycle[3]);
	#endif

			aocKeyexTeaEncrypt(cycle, akey);
	#ifdef __BIG_ENDIAN__
	    sprintf(p, "%08x%08x", SwapIfBe32(cycle[2]), SwapIfBe32(cycle[3]));
	#else
	    sprintf(p, "%08x%08x", htonl(cycle[2]), htonl(cycle[3]));
	#endif
			p += 16;
		}

	return crypted;
}


char *aocKeyexGenerateKey(const char *servkey, const char *username, const char *password)
{
	char *key_a, *key_b, *plain, *crypted;
	int slen, tlen;
	char *retkey;


	if(servkey == NULL) return NULL;
	if(username == NULL) return NULL;
	if(password == NULL) return NULL;


	if(!aocDoDiffieHellman(&key_a, &key_b))
		return NULL;


	if(strlen(key_b) < 32)
	{
		char *dest = key_b + 32 - strlen(key_b);

		/* left-pad with zeroes */
		memmove(dest, key_b, strlen(key_b));
		memset(key_b, '0', dest - key_b);
	}

	/* the string's length 'slen', and the total block length 'tlen' */
	slen = strlen(username) + strlen(servkey) + strlen(password) + 2;
	tlen = (slen + 19) / 8 * 8; 	/* rounded up to next multiple of eight */

	plain = malloc(tlen+1);
	/* generate a random prefix to avoid plain-text attacks */
	aocRandom(plain, 8);

	/* pack the content-length in network byte order */
	plain[ 8] = (slen>>24) & 0xff;
	plain[ 9] = (slen>>16) & 0xff;
	plain[10] = (slen>> 8) & 0xff;
	plain[11] = (slen>> 0) & 0xff;

	/* the actual content of the message */
	sprintf(plain+12, "%s|%s|%s", username, servkey, password);

	/* pad with spaces as needed */
	memset(plain+12+slen, ' ', tlen-slen-12);

	/* 'crypted' is a malloc()ed string from aocKeyexCrypt() */
	crypted = aocKeyexCrypt(key_b, plain, tlen);

	/* zero memory */
	memset(plain, 0, tlen);

	/* store the data in the pointer provided to us */
	retkey = malloc(strlen(key_a) + tlen*2 + 2);
	sprintf(retkey, "%s-%s", key_a, crypted);

	/* clean up */
	free(plain);
	free(crypted);

#ifdef HAVE_LIBCRYPTO
	OPENSSL_free(key_b);
	OPENSSL_free(key_a);
#else
	free(key_b);
	free(key_a);
#endif

	return retkey;
}

