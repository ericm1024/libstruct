/*
 * Copyright 2014 Eric Mueller
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/**
 * \author Eric Mueller
 * 
 * \file bloom.c
 *
 * \brief Implementation of a bloom filter.
 */

#include "bloom.h"
#include "mtwist-1.5/mtwist.h"
#include "fasthash.h"
#include <stdlib.h>
#include <limits.h> /* CHAR_BIT */
#include <math.h>
#include <assert.h>

#define BITS_PER_LONG (CHAR_BIT * sizeof(long))

/*
 * right shift a bit array index by this to get the index within the long
 * array
 */ 
#define BINDEX_SHIFT (BITS_PER_LONG == 64 ? 6 : 5)
/*
 * mask a bit array index with this to get the index of the bit within
 * a long
 */ 
#define BINDEX_MASK ((1 << BINDEX_SHIFT) - 1)
/*
 * convert a bit array index into a long array index
 */ 
#define BINDEX_TO_INDEX(bi) (bi >> BINDEX_SHIFT)
/*
 * The bitwise AND of this mask with the long containing the given bit
 * will flag the bit, the bitwise OR will set the bit.
 */ 
#define BINDEX_TO_BITMASK(bi) (1 << (bi & BINDEX_MASK))

static inline void set_bit(bloom_t *bf, size_t biti)
{
	size_t i = BINDEX_TO_INDEX(biti);
	int mask = BINDEX_TO_BITMASK(biti);
	bf->bits[i] |= mask;
}

static inline int get_bit(bloom_t *bf, size_t biti)
{
	size_t i = BINDEX_TO_INDEX(biti);
	int mask = BINDEX_TO_BITMASK(biti);
	return !!(bf->bits[i] & mask);
}

int bloom_init(bloom_t *bf)
{
	/*
	 * Here we need to pick good values for the size of the filter table
	 * and the number of hash functions. The Wikipedia article tells
	 * us that the optimial number of bits for a desired false probability
	 * p is
	 *         m = - (n* ln(p))/(ln(2)^2).
	 * It also tells us that the optimal number of hash functions k is
	 * given by
	 *        k = (m/n)ln(2).
	 * We'll do all of this computation with floats before we round off
	 * at the end.
	 *        
	 * (TODO find a more reliable source for these formulas)
	 * Source:
	 * http://en.wikipedia.org/wiki/Bloom_filter#Optimal_number_of_hash_functions
	 */
	
	double p = bf->p;
	if (p < BLOOM_P_MIN || p > BLOOM_P_MAX) {
		p = BLOOM_P_DEFAULT;
		bf->p = p;
	}
	double n = (double)bf->n;
	double m = -(n * log(p))/(M_LN2*M_LN2);
	double k = (m/n)*M_LN2;
	
	/*
	 * m is the number of bits we want, but since we're allocating
	 * in chunks of size long, we want bf->bsize to be the number
	 * of entries in the array, so we have to convert. We add 1
	 * because the divide will always round down.
	 */
	bf->bsize = (size_t)(lrint(m)/(BITS_PER_LONG) + 1);
	bf->nbits = bf->bsize * BITS_PER_LONG;
	bf->nhash = (size_t)lrint(k);

	/* try to alocate both arrays */
	bf->bits = (long*)malloc(sizeof(long)*bf->bsize);
	if (!bf->bits)
		return 1;
	
	bf->seeds = (uint64_t*)malloc(sizeof(uint64_t)*bf->nhash);
	if (!bf->seeds) {
		free(bf->bits);
		return 1;
	}

	/* initialize the Mersenne-Twise PRNG if necessary */
	mt_state *s = mt_getstate();
	if (!s->initialized)
		mt_goodseed();

	/* generate seeds for the hash functions */
	for (size_t i = 0; i < bf->nhash; i++)
		bf->seeds[i] = mt_llrand();
	return 0;
}

void bloom_destroy(bloom_t *bf)
{
	free(bf->bits);
	free(bf->seeds);
	bf->bits = NULL;
	bf->seeds = NULL;
}

void bloom_insert(bloom_t *bf, uint64_t key)
{
	for (size_t i = 0; i < bf->nhash; i++) {
		uint64_t hash = fasthash64(&key, sizeof(key), bf->seeds[i]);
		set_bit(bf, hash % bf->nbits);
	}
}

int bloom_query(bloom_t *bf, uint64_t key)
{
	for (size_t i = 0; i < bf->nhash; i++) {
		uint64_t hash = fasthash64(&key, sizeof(key), bf->seeds[i]);
		if (!get_bit(bf, hash % bf->nbits))
			return 1;
	}
	return 0;
}
