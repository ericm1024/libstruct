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
 * htable_schain.c
 *
 * This is an implementation of a hash table using separate chaining.
 */

#include <assert.h>
#include "htable.h"
#include "../c_linked_list/llist.h"

const size_t INIT_TABLE_SIZE = 7; /* small, arbitrary prime */

/************* implementation specific function declarations ***************/

void* init_buckets(size_t num_buckets);
void free_buckets(htable_t* ht);
bool maybe_resize(htable_t* ht);
bool insert_at(htable_t* ht, void* elem, size_t where);
bool exists(htable_t* ht, void* elem);

/*************************** generic functions ****************************/

htable_t* htable_init(hash_func_t hash)
{
    assert(hash);
    htable_t* ht = mallof(sizeof(htable_t));

    /* if the allocation failed, bail */
    if (!ht) {
	return NULL;
    }

    /* initialize the table */
    ht->size = INIT_TABLE_SIZE;
    ht->entries = 0;
    ht->hash = hash;
    ht->buckets = init_buckets(ht->size);

    /* if init_buckets failed, free ht, then, bail */
    if (!ht->buckets) {
	free(ht);
	return NULL;
    }
    
    return ht;
}

void htable_destroy(htable_t* ht)
{
    free_buckets(ht);
    free(ht);
}

bool htable_insert(htable_t* ht, void* elem)
{
    if (!maybe_resize(ht)) {
	return false;
    }
    
    size_t index = ht->hash(elem) % ht->size;
    return insert_at(ht,elem,index);
}

bool htable_exists(htable_t* ht, void* elem)
{
    return exists(ht, elem);
}

float load_factor(htable_t* ht)
{
    return ht->entries / (float)ht->size;
}

/******************** hash table implementation ************************/

#if HTABLE_USE_SEP_CHAIN

/******************* separate chaining functions *********************/

void* init_buckets(size_t num_buckets)
{
    llist_head_t* buckets = malloc(sizeof(llist_head_t) * num_buckets);

    /* if the allocation failed, bail */
    if (!buckets) {
	return NULL;
    }

    /* initalize all of the linked lists */
    for (size_t i = 0; i < num_buckets; i++) {
	llist_init(NULL, buckets[i]);
    }
    
    return (void*)buckets;
}

void free_buckets(htable_t* ht)
{
    llist_head_t* buckets = (llist_head_t*)ht->buckets;
    for (size_t i = 0; i < ht->size; i++) {
	
    }
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
