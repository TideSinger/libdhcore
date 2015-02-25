/***********************************************************************************
 * Copyright (c) 2012, Sepehr Taghdisian
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 ***********************************************************************************/

#include "dhcore/hash.h"

#define HSEED 98424

#define HASH_M 0x5bd1e995
#define HASH_R 24
#define MMIX(h, k) { k *= HASH_M; k ^= k >> HASH_R; k *= HASH_M; h *= HASH_M; h ^= k; }

#undef GET16BITS
#if defined(_GNUC_) || defined(_WATCOMC_) || defined(_MSVC_)
#define GET16BITS(d)    (*((const uint16 *) (d)))
#endif

#if !defined(GET16BITS)
#define GET16BITS(d) ((((uint)(((const uint8 *)(d))[1])) << 8)\
        +(uint)(((const uint8 *)(d))[0]))
#endif

#if defined(_MSVC_)
#include <stdlib.h>
#define ROTL32(x,y)     _rotl(x,y)
#define ROTL64(x,y)     _rotl64(x,y)
#define BIG_CONSTANT(x) (x)
#else
FORCE_INLINE uint rotl32(uint x, int8 r)
{
    return (x << r) | (x >> (32 - r));
}

FORCE_INLINE uint64 rotl64(uint64 x, int8 r)
{
    return (x << r) | (x >> (64 - r));
}
#define ROTL32(x,y)     rotl32(x,y)
#define ROTL64(x,y)     rotl64(x,y)
#define BIG_CONSTANT(x) (x##LLU)
#endif

FORCE_INLINE uint getblock32(const uint* p, int i)
{
        return p[i];
}

FORCE_INLINE uint64 getblock64(const uint64* p, int i)
{
        return p[i];
}

INLINE uint fmix32(uint h)
{
	  h ^= h >> 16;
	  h *= 0x85ebca6b;
	  h ^= h >> 13;
	  h *= 0xc2b2ae35;
	  h ^= h >> 16;

	  return h;
}

INLINE uint64 fmix64(uint64 k)
{
	  k ^= k >> 33;
	  k *= BIG_CONSTANT(0xff51afd7ed558ccd);
	  k ^= k >> 33;
	  k *= BIG_CONSTANT(0xc4ceb9fe1a85ec53);
	  k ^= k >> 33;

	  return k;
}

/* fwd */
static void hash_mixtail(struct hash_incr* h, const uint8** pdata, size_t* psize);

/* Hash functions */
uint hash_str(const char* str)
{
    return hash_murmur32(str, strlen(str), HSEED);
}

uint hash_murmur32(const void* key, size_t size_bytes, uint seed)
{
    const uint8 * data = (const uint8*)key;
    const int nblocks = (int)(size_bytes / 4);

    uint h1 = seed;
    uint c1 = 0xcc9e2d51;
    uint c2 = 0x1b873593;

    const uint * blocks = (const uint *)(data + nblocks*4);
    for (int i = -nblocks; i; i++)    {
            uint k1 = getblock32(blocks,i);

            k1 *= c1;
            k1 = ROTL32(k1,15);
            k1 *= c2;

            h1 ^= k1;
            h1 = ROTL32(h1,13);
            h1 = h1*5+0xe6546b64;
    }

    const uint8 * tail = (const uint8*)(data + nblocks*4);
    uint k1 = 0;
    switch (size_bytes & 3)    {
    case 3: k1 ^= tail[2] << 16;
    case 2: k1 ^= tail[1] << 8;
    case 1: k1 ^= tail[0];
            k1 *= c1; k1 = ROTL32(k1,15); k1 *= c2; h1 ^= k1;
    };

    /* finalization */
    h1 ^= size_bytes;
    h1 = fmix32(h1);
    return h1;
}

/* 128bits murmur hash is different in 64/32bit platforms */
#ifdef _ARCH64_
hash_t hash_murmur128(const void* key, size_t size_bytes, uint seed)
{
    const uint8* data = (const uint8*)key;
    const int nblocks = (int)(size_bytes / 16);

    hash_t h;
    uint64 h1 = seed;
    uint64 h2 = h1;

    uint64 c1 = BIG_CONSTANT(0x87c37b91114253d5);
    uint64 c2 = BIG_CONSTANT(0x4cf5ad432745937f);

    /* body */
    const uint64* blocks = (const uint64 *)(data);

    for (int i = 0; i < nblocks; i++)    {
            uint64 k1 = getblock64(blocks,i*2+0);
            uint64 k2 = getblock64(blocks,i*2+1);

            k1 *= c1; k1 = ROTL64(k1,31); k1 *= c2; h1 ^= k1;
            h1 = ROTL64(h1,27); h1 += h2; h1 = h1*5+0x52dce729;
            k2 *= c2; k2 = ROTL64(k2,33); k2 *= c1; h2 ^= k2;
            h2 = ROTL64(h2,31); h2 += h1; h2 = h2*5+0x38495ab5;
    }

    /* tail */
    const uint8* tail = (const uint8*)(data + nblocks*16);

    uint64 k1 = 0;
    uint64 k2 = 0;

    switch (size_bytes & 15)    {
    case 15: k2 ^= (uint64)tail[14] << 48;
    case 14: k2 ^= (uint64)tail[13] << 40;
    case 13: k2 ^= (uint64)tail[12] << 32;
    case 12: k2 ^= (uint64)tail[11] << 24;
    case 11: k2 ^= (uint64)tail[10] << 16;
    case 10: k2 ^= (uint64)tail[ 9] << 8;
    case  9: k2 ^= (uint64)tail[ 8] << 0;
            k2 *= c2; k2 = ROTL64(k2,33); k2 *= c1; h2 ^= k2;

    case  8: k1 ^= (uint64)tail[ 7] << 56;
    case  7: k1 ^= (uint64)tail[ 6] << 48;
    case  6: k1 ^= (uint64)tail[ 5] << 40;
    case  5: k1 ^= (uint64)tail[ 4] << 32;
    case  4: k1 ^= (uint64)tail[ 3] << 24;
    case  3: k1 ^= (uint64)tail[ 2] << 16;
    case  2: k1 ^= (uint64)tail[ 1] << 8;
    case  1: k1 ^= (uint64)tail[ 0] << 0;
            k1 *= c1; k1 = ROTL64(k1,31); k1 *= c2; h1 ^= k1;
    };

    /* finalization */
    h1 ^= size_bytes; h2 ^= size_bytes;
    h1 += h2;
    h2 += h1;
    h1 = fmix64(h1);
    h2 = fmix64(h2);
    h1 += h2;
    h2 += h1;
    h.h[0] = h1;
    h.h[1] = h2;
    return h;
}
#else
hash_t hash_murmur128(const void* key, size_t size_bytes, uint seed)
{
    const uint8 * data = (const uint8*)key;
    const int nblocks = (int)(size_bytes / 16);

    hash_t h;
    uint h1 = seed;
    uint h2 = h1;
    uint h3 = h1;
    uint h4 = h1;

    uint c1 = 0x239b961b;
    uint c2 = 0xab0e9789;
    uint c3 = 0x38b34ae5;
    uint c4 = 0xa1e38b93;

    /* body */
    const uint* blocks = (const uint*)(data + nblocks*16);

    for (int i = -nblocks; i; i++)     {
            uint k1 = getblock32(blocks, i*4+0);
            uint k2 = getblock32(blocks, i*4+1);
            uint k3 = getblock32(blocks, i*4+2);
            uint k4 = getblock32(blocks, i*4+3);

            k1 *= c1; k1 = ROTL32(k1,15); k1 *= c2; h1 ^= k1;
            h1 = ROTL32(h1,19); h1 += h2; h1 = h1*5+0x561ccd1b;
            k2 *= c2; k2 = ROTL32(k2,16); k2 *= c3; h2 ^= k2;
            h2 = ROTL32(h2,17); h2 += h3; h2 = h2*5+0x0bcaa747;
            k3 *= c3; k3 = ROTL32(k3,17); k3 *= c4; h3 ^= k3;
            h3 = ROTL32(h3,15); h3 += h4; h3 = h3*5+0x96cd1c35;
            k4 *= c4; k4 = ROTL32(k4,18); k4 *= c1; h4 ^= k4;
            h4 = ROTL32(h4,13); h4 += h1; h4 = h4*5+0x32ac3b17;
    }

    /* tail */
    const uint8 * tail = (const uint8*)(data + nblocks*16);

    uint k1 = 0;
    uint k2 = 0;
    uint k3 = 0;
    uint k4 = 0;

    switch (size_bytes & 15)    {
    case 15: k4 ^= tail[14] << 16;
    case 14: k4 ^= tail[13] << 8;
    case 13: k4 ^= tail[12] << 0;
            k4 *= c4; k4 = ROTL32(k4,18); k4 *= c1; h4 ^= k4;

    case 12: k3 ^= tail[11] << 24;
    case 11: k3 ^= tail[10] << 16;
    case 10: k3 ^= tail[ 9] << 8;
    case  9: k3 ^= tail[ 8] << 0;
            k3 *= c3; k3 = ROTL32(k3,17); k3 *= c4; h3 ^= k3;

    case  8: k2 ^= tail[ 7] << 24;
    case  7: k2 ^= tail[ 6] << 16;
    case  6: k2 ^= tail[ 5] << 8;
    case  5: k2 ^= tail[ 4] << 0;
            k2 *= c2; k2 = ROTL32(k2,16); k2 *= c3; h2 ^= k2;

    case  4: k1 ^= tail[ 3] << 24;
    case  3: k1 ^= tail[ 2] << 16;
    case  2: k1 ^= tail[ 1] << 8;
    case  1: k1 ^= tail[ 0] << 0;
            k1 *= c1; k1 = ROTL32(k1,15); k1 *= c2; h1 ^= k1;
    };

    /* finalization */
    h1 ^= size_bytes; h2 ^= size_bytes; h3 ^= size_bytes; h4 ^= size_bytes;
    h1 += h2; h1 += h3; h1 += h4;
    h2 += h1; h3 += h1; h4 += h1;
    h1 = fmix32(h1);
    h2 = fmix32(h2);
    h3 = fmix32(h3);
    h4 = fmix32(h4);

    h1 += h2; h1 += h3; h1 += h4;
    h2 += h1; h3 += h1; h4 += h1;

    h.h[0] = h1;
    h.h[1] = h2;
    h.h[2] = h3;
    h.h[3] = h4;
    return h;
}

#endif

uint hash_u64(uint64 n)
{
    n = (~n) + (n << 18);
    n = n ^ (n >> 31);
    n = n * 21;
    n = n ^ (n >> 11);
    n = n + (n << 6);
    n = n ^ (n >> 22);
    return (uint)n;
}

void hash_murmurincr_begin(struct hash_incr* h, uint seed)
{
	h->hash = seed;
	h->size = 0;
	h->tail = 0;
	h->cnt = 0;
}

void hash_murmurincr_add(struct hash_incr* h, const void* data, size_t size)
{
	const uint8* key = (const uint8*)data;

	h->size += size;

	hash_mixtail(h, &key, &size);

	while (size >= 4)	{
		uint k = *((const uint*)key);

		MMIX(h->hash, k);

		key += 4;
		size -= 4;
	}

	hash_mixtail(h, &key, &size);
}

static void hash_mixtail(struct hash_incr* h, const uint8** pdata, size_t* psize)
{
	uint size = (uint)*psize;
	const uint8* data = *pdata;

	while (size && ((size<4) || h->cnt))	{
		h->tail |= (*data++) << (h->cnt * 8);

		h->cnt++;
		size--;

		if (h->cnt == 4)	{
			MMIX(h->hash, h->tail);
			h->tail = 0;
			h->cnt = 0;
		}
	}

	*pdata = data;
	*psize = size;
}

uint hash_murmurincr_end(struct hash_incr* h)
{
	MMIX(h->hash, h->tail);
	MMIX(h->hash, h->size);

	h->hash ^= h->hash >> 13;
	h->hash *= HASH_M;
	h->hash ^= h->hash >> 15;

	return h->hash;
}

