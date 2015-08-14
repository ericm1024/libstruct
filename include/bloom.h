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
 * \file bloom.h 
 *
 * \author Eric Mueller
 * 
 * \brief Header file for a bloom filter
 *
 * \detail A bloom filter is a set-like data structure similar to a hash table
 * with a rather unique set of pros and cons.
 *
 * pros:
 *   - size: bloom filters are small, ususally requiring on the order of 8-12
 *     bits per element.
 *   - speed: all operations (insert and query) take worst case constant time,
 *     and in practice are very fast.
 *
 * cons:
 *   - false positives: bloom filters will ocasionally return true when
 *     querying for an element that was not inserted. The probability of
 *     false positives can be selected when constructing the filter. Lower
 *     probabilities require more space.
 *   - resizing: bloom filters can not be resized, so you must know (roughly)
 *     the size of your data set at construction time. If you underestimate and
 *     end up inserting more elements that you originally indented, the false
 *     positive probability will rise.
 *   - deletion: elements can not be deleted from a bloom filter.
 *
 * Despite these drawbacks, bloom filters can be extremely useful. For example,
 * many databases use them to prevent unnecessary disk lookups for non-existent
 * rows. (i.e., if a row doesn't exist, the bloom filter will tell you so with
 * high probability, but if it does the db would have to go to disk anyway.
 * their small memory footprint makes them an even better canidate for this
 * task.)
 *
 * To use this bloom filter, first declare one using the BLOOM_FILTER macro, ex:
 *
 *     BLOOM_FILTER(my_filter, 1 << 20, 0.005)
 *
 * Then call bloom_init to initialize the filter (this allocates memory). At
 * this point use any combination of bloom_insert and bloom_query. When you are
 * done with the filter, call bloom_destroy to free all memory associated with
 * it.
 *
 * Synchronization is left to the caller.
 */

#ifndef STRUCT_BLOOM_H
#define STRUCT_BLOOM_H 1

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/** bloom filter */
struct bloom {
        /** bits array for filter */
	unsigned long *bits;

        /** seeds for hash functions */
	uint64_t *seeds;

        /** target number of elements. This is used to size the bitmap */
	unsigned long n;

        /** number of longs in the bits array */
	unsigned long bsize;

        /** number of hash functions, i.e. number of entries in seeds */
	unsigned long nhash;

        /** target false probability */
	double p;

        /** number of bits we actually use in the bits array */
	unsigned long nbits;
};

/*! lower bound on allowable false positive probability parameter */
#define BLOOM_P_MIN (1e-5)
/*! upper bound on allowable false positive probability parameter */
#define BLOOM_P_MAX (5e-2)
/*! convenience macro for a reasonable default false positive probability */
#define BLOOM_P_DEFAULT (5e-3)

/**
 * \brief Declare a bloom filter.
 * \param name  (token) name of the filter to declare
 * \param n  Expected number of keys to be inserted into the filter.
 * \param p  Target false probability. Must be between BLOOM_P_MIN and
 * BLOOM_P_MAX. BLOOM_P_DEFAULT is a good choice if you aren't sure.
 * \detail This does not initialize the structure. That is done by
 * bloom_init.
 */
#define BLOOM_FILTER(name, nkeys, prob)			   \
	struct bloom name = {				   \
		.bits = NULL,				   \
		.seeds = NULL,				   \
		.n = (nkeys),				   \
		.bsize = 0,				   \
		.nhash = 0,				   \
		.p = (prob),				   \
		.nbits = 0};

/**
 * \brief Initialize a bloom filter.
 * \param bf  The filter to initialize.
 * \return true on sucess, false on allocation failure.
 */
extern bool bloom_init(struct bloom *bf);

/**
 * \brief Destroy a bloom filter.
 * \param bf  The bloom filter to initialize.
 * \detail Frees all memory associated with @bf
 */
extern void bloom_destroy(struct bloom *bf);

/**
 * \brief Insert a key into the filter.
 * \param bf  The bloom filter to insert into.
 * \param key  The key to insert.
 */
extern void bloom_insert(struct bloom *bf, uint64_t key);

/**
 * \brief Query a bloom filter for the existence of a key.
 * \param bf  The bloom filter to query.
 * \param key  The key to query for.
 * \return true if the key probably exists, false if it definitely does not. 
 */
extern bool bloom_query(const struct bloom *bf, uint64_t key);

#endif /* STRUCT_BLOOM_H */
