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
#include "fasthash.h"
#include "util.h"
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <string.h>

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
#define BINDEX_TO_BITMASK(bi) (1UL << (bi & BINDEX_MASK))

static inline void set_bit(struct bloom *bf, unsigned long biti)
{
	unsigned long i = BINDEX_TO_INDEX(biti);
	unsigned long mask = BINDEX_TO_BITMASK(biti);
	bf->bits[i] |= mask;
}

static inline bool get_bit(const struct bloom *bf, unsigned long biti)
{
	unsigned long i = BINDEX_TO_INDEX(biti);
	unsigned long mask = BINDEX_TO_BITMASK(biti);
	return !!(bf->bits[i] & mask);
}

bool bloom_same_class(const struct bloom *bf0, const struct bloom *bf1)
{
	unsigned i = 0;

	if (bf0->nbits != bf1->nbits || bf0->nhash != bf1->nhash)
		return false;

	for (i = 0; i < bf0->nhash; i++)
		if (bf0->seeds[i] != bf1->seeds[i])
			return false;

	return true;
}

static bool bloom_init_arrays(struct bloom *bf)
{
	/* try to alocate both arrays */
	bf->bits = malloc(sizeof *bf->bits * bf->bsize);
	if (!bf->bits)
		return false;

	bf->seeds = malloc(sizeof *bf->seeds * bf->nhash);
	if (!bf->seeds) {
		free(bf->bits);
		bf->bits = NULL;
		return false;
	}
	memset(bf->bits, 0, sizeof *bf->bits * bf->bsize);
	return true;
}

bool bloom_init(struct bloom *bf)
{
        double p, n, m, k;
        unsigned i;

	if (!seed_rng())
		return false;
	
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
	 *
	 * http://corte.si/%2Fposts/code/bloom-filter-rules-of-thumb/index.html
	 */
	
	p = bf->p;
	if (p < BLOOM_P_MIN) {
		p = BLOOM_P_MIN;
		bf->p = p;
	} else if (p > BLOOM_P_MAX) {
		p = BLOOM_P_MAX;
		bf->p = p;
	}
	
	n = bf->n;
	m = -(n * log(p))/(M_LN2*M_LN2);
	k = (m/n)*M_LN2;
	
	/*
	 * m is the number of bits we want, but since we're allocating
	 * in chunks of size long, we want bf->bsize to be the number
	 * of entries in the array, so we have to convert. We add 1
	 * because the divide will always round down.
	 */
	bf->bsize = lrint(m)/(BITS_PER_LONG) + 1;
	bf->nbits = bf->bsize * BITS_PER_LONG;
	bf->nhash = k;

	if (!bloom_init_arrays(bf))
		return false;
	
	/* generate seeds for the hash functions */
	for (i = 0; i < bf->nhash; i++)
		bf->seeds[i] = pcg64_random();

	return true;
}

bool bloom_init_from(struct bloom *restrict bf,
		     const struct bloom *restrict other)
{
	bf->n = other->n;
	bf->bsize = other->bsize;
	bf->nhash = other->nhash;
	bf->p = other->p;
	bf->nbits = other->nbits;

	if (!bloom_init_arrays(bf))
		return false;

	memcpy(bf->seeds, other->seeds, sizeof *other->seeds * other->nhash);
	return true;
}

void bloom_destroy(struct bloom *bf)
{
	free(bf->bits);
	free(bf->seeds);
	bf->bits = NULL;
	bf->seeds = NULL;
}

void bloom_insert(struct bloom *bf, uint64_t key)
{
        uint64_t hash;
        unsigned i;

	for (i = 0; i < bf->nhash; i++) {
		hash = fasthash64(&key, sizeof key, bf->seeds[i]);
		set_bit(bf, hash % bf->nbits);
	}
}

bool bloom_query(const struct bloom *bf, uint64_t key)
{
        uint64_t hash;
        unsigned i;

	for (i = 0; i < bf->nhash; i++) {
		hash = fasthash64(&key, sizeof key, bf->seeds[i]);
		if (!get_bit(bf, hash % bf->nbits))
			return false;
	}
	return true;
}

/**
 * \brief Helper for bloom_union and bloom_intersection.
 * \detail Check if into, bf0, and bf1 are all the same class, but allow
 * into to be uninitialized.
 *
 * \param into    The filter to merge into.
 * \param bf0     One filter to merge.
 * \param bf1     The other filter to merge.
 *
 * \return True if bf0 and bf1 can be merged into into.
 */
static bool can_merge(struct bloom *into, const struct bloom *bf0,
		      const struct bloom *restrict bf1)
{
	bool need_free = false;

	/* we allow into to be uninitialized, if it is unique */
	if (into != bf0 && !into->bits) {
		need_free = true;
		*into = BLOOM_FILTER_INITIALIZER(bf0->n, bf0->p);
		if (!bloom_init_from(into, bf0))
			return false;
	}

	if (!bloom_same_class(into, bf0) || !bloom_same_class(into, bf1)) {
		if (need_free)
			bloom_destroy(into);
		return false;
	}

	return true;
}

bool bloom_union(struct bloom *restrict into,
		 const struct bloom *restrict bf0,
		 const struct bloom *restrict bf1)
{
	unsigned long i;

	if (!can_merge(into, bf0, bf1))
		return false;

	for (i = 0; i < into->bsize; i++)
		into->bits[i] = bf0->bits[i] | bf1->bits[i];

	return true;
}

bool bloom_intersection(struct bloom *restrict into,
			const struct bloom *restrict bf0,
			const struct bloom *restrict bf1)
{
	unsigned long i;

	if (!can_merge(into, bf0, bf1))
		return false;

	for (i = 0; i < into->bsize; i++)
		into->bits[i] = bf0->bits[i] & bf1->bits[i];

	return true;
}
