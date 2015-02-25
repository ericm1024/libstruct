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
 */

#ifndef STRUCT_BLOOM_H
#define STRUCT_BLOOM_H 1

#include "libstruct.h"

/*
 * max and min allowable target false positive rates. Admitedly somewhat
 * arbitrary, but sue me.
 */
#define BLOOM_P_MIN (1e-5)
#define BLOOM_P_MAX (5e-2)
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
#define BLOOM_FILTER(name, n, p)                           \
	bloom_t name = (bloom_t){NULL, NULL, n, 0, 0, p, 0};

/**
 * \brief Initialize a bloom filter.
 * \param bf  The filter to initialize.
 * \return 0 on sucess, 1 if memory allocation failed.
 */
extern int bloom_init(bloom_t *bf);

/**
 * \brief Destroy a bloom filter.
 * \param bf  The bloom filter to initialize.
 * \detail Frees all memory associated with @bf
 */
extern void bloom_destroy(bloom_t *bf);

/**
 * \brief Insert a key into the filter.
 * \param bf  The bloom filter to insert into.
 * \param key  The key to insert.
 */
extern void bloom_insert(bloom_t *bf, uint64_t key);

/**
 * \brief Query a bloom filter for the existence of a key.
 * \param bf  The bloom filter to query.
 * \param key  The key to query for.
 * \return 0 if the key probably exists, 1 if it definitely does not. 
 */
extern int bloom_query(bloom_t *bf, uint64_t key);

#endif /* STRUCT_BLOOM_H */
