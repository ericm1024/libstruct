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
 * htable.h
 *
 * This is a header file for a hash table.
 */

#ifndef HTABLE_H
#define HTABLE_H 1

#include <stdbool.h>

/* typedefs */
typedef size_t(*)(void*) hash_func_t;
typedef struct {
    size_t size;         /* number of buckets */
    size_t entries;      /* number of items inserted into the table */
    hash_func_t hash;
    void* buckets;
} htable_t;

/* c'tor & d'tor */
htable_t* htable_init(hash_func_t hash);
void htable_destroy(htable_t* ht);

/* insert */
bool htable_insert(htable_t* ht, void* elem);

/* exists */
bool htable_exists(htable_t* ht, void* elem);

/* bookeeping function */
float load_factor(htable_t* ht);

#endif /* HTABLE_H */
