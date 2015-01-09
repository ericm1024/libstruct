/* Copyright 2014 Eric Mueller
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

/* 
 * Eric Mueller
 * November 2014
 * htable.c
 *
 * Contains two implementations of a hash table.
 */

#include <assert.h>
#include <climits.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "fast-hash/fasthash.h"
#include "htable.h"
#include "sllist.h"

const size_t INIT_TABLE_SIZE = 7; /* small, arbitrary prime */

/*
 * implementation specific forward declarations
 */
void* init_buckets(size_t num_buckets);
void free_buckets(htable_t* ht);
bool maybe_resize(htable_t* ht);
bool insert_at(htable_t* ht, void* elem, size_t where);
bool exists(htable_t* ht, size_t hash);

/*************************** generic functions ****************************/

size_t hash(const void* buf, size_t len, size_t seed)
{
    assert(sizeof(size_t) == 4 || sizeof(size_t) == 8);
    
    if (sizeof(size_t) == 4) {
	return fasthash32(buf, len, seed);
    } else {
	return fasthash64(buf, len, seed);
    }
}

htable_t* htable_init(htable_t* ht)
{
    /* initialize the table */
    ht->size = INIT_TABLE_SIZE;
    ht->entries = 0;
    ht->buckets = init_buckets(ht->size);

    /* if init_buckets failed, free ht, then, bail */
    if (!ht->buckets) {
	free(ht);
	return NULL;
    }

    /* initialize the rng, seed the hash function */
    srand(time(NULL));
    ht->hash = rand()
    
    return ht;
}

void htable_destroy(htable_t* ht)
{
    free_buckets(ht);
}

bool htable_insert(htable_t* ht, const void* elem, size_t size)
{
    if (!maybe_resize(ht)) {
	return false;
    }
    
    size_t index = ht->hash(elem) % ht->size;
    return insert_at(ht,elem,index);
}

bool htable_exists(const htable_t* ht, const void* elem, size_t size)
{
    size_t hash = hash(elem, size);
    return exists(ht, hash);
}

float load_factor(const htable_t* ht)
{
    return ht->entries / (float)ht->size;
}

/*
 * Returns the next prime number above a given power of 2.
 * Code taken from HMC CS70 fall 2014 assignment 10.
 */ 
size_t prime_above_power(size_t power)
{
    // Table of differences between nearest prime and power of two. We
    // use unsigned char (i.e., an 8-bit unsigned int) to make the
    // table tiny.
    static unsigned char offset[] = {
	0, 0, 1, 3, 1, 5, 3, 3, 1, 9, 7, 5, 3, 17, 27, 3, 1, 29, 3,
	21, 7, 17, 15, 9, 43, 35, 15, 29, 3, 11, 3, 11
#if LONG_BIT == 32
	// No more entries needed
#elif LONG_BIT == 64
	, 15, 17, 25, 53, 31, 9, 7, 23, 15, 27, 15, 29, 7, 59, 15, 5,
	21, 69, 55, 21, 21, 5, 159, 3, 81, 9, 69, 131, 33, 15, 135, 29
#else
#error Expected long to be 32-bit or 64-bit.
#endif
    };

    assert (power > 0 && power < LONG_BIT);
    return (1UL << power) + offset[power];
}

/******************** hash table implementation ************************/

#if HTABLE_USE_SEP_CHAIN

/******************* separate chaining functions *********************/

void* init_buckets(size_t num_buckets)
{
    sllist_head_t* buckets = malloc(sizeof(sllist_head_t) * num_buckets);

    /* if the allocation failed, bail */
    if (!buckets) {
	return NULL;
    }

    /* initalize all of the linked lists */
    for (size_t i = 0; i < num_buckets; i++) {
	sllist_init(buckets[i]);
    }
    
    return (void*)buckets;
}

void free_buckets(htable_t* ht)
{
    sllist_head_t* buckets = (sllist_head_t*)ht->buckets;
    for (size_t i = 0; i < ht->size; i++) {
	sllist_destroy(buckets[i]);
    }
    free(buckets);
}

/*
 * Resizes the table if necessary. Returns false if the reallocation fails,
 * otherwise returns true (even if the table was not resized)
 */
bool maybe_resize(htable_t* ht)
{
    /* return if we don't need to resize */
    if (load_factor(ht) <= SCHAIN_LOAD_MAX) {
      return true;
    }
    
    size_t new_size = prime_above(log2(ht->size) + 1);
    sllist_head_t* new_buckets = malloc(new_size * sizeof(sllist_head_t));

    /* if the malloc failed, bail */
    if (!new_buckets) {
	return false;
    }
    
    for (size_t i = 0; i < ht->size; i++) {
	for (sllist_t* node = sllist_pop_front(buckets[i]); node;
	     node = sllist_pop_front(buckets[i])) {
	    size_t hash = hash(node->data, siseof(llist_hed), 
	}
    }
}

bool insert_at(void* buckets, void* elem, size_t where)
{
    llist_head_t* buckets = (llist_head_t*)buckets;
    llist_push_back(buckets[where],elem);
}

bool exists(htable_t* ht, void* elem)
{
    
}

#elif HTABLE_USE_QUAD_PROBE 

/******************* quadratic probing functions **********************/

void* init_buckets(size_t num_buckets)
{

}

void free_buckets(void* buckets)
{

}

bool maybe_resize(htable_t* ht)
{

}

bool insert_at(htable_t* ht, void* elem, size_t where)
{

}

bool exists(htable_t* ht, void* elem)
{

}

#else
#error No hash table implementation type specified. 
#endif
