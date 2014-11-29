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

#define INIT_TABLE_SIZE 7 /* just an arbitrary small prime... */

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
    ht->buckets = init_buckets();

    /* if init_buckets() failed, free ht, then, bail */
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

void htable_insert(htable_t* ht, void* elem)
{
    size_t index = ht->hash(elem) % ht->size;
    insert_at(ht,elem,index);
}

bool htable_exists(htable_t* ht, void* elem)
{
    
}
