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
/**
 * \file htable.h
 *
 * \author Eric Mueller
 *
 * \brief Header file for a hash table.
 */

#ifndef STRUCT_HTABLE_H
#define STRUCT_HTABLE_H 1

#include "libstruct.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * \brief Declare a hash table.
 *
 * \param name  (token) The name of the hash table to declare.
 */
#define HASH_TABLE(name) \
	htable_t name = {0, 0, 0, 0, NULL, NULL, NULL}

/**
 * \brief Initialize a hash table of a given size.
 *
 * \param ht        Pointer to the hash table to initialize.
 * \param nbuckets  Initial number of buckets to allocate, where a bucket
 *                  constitutes space for a single insertion. (i.e. has nothing
 *                  to do with the actual underlying bucket size)
 * \return true on success or false if table allocation failed.
 */
bool htable_init(htable_t *ht, size_t nbuckets);

/**
 * \brief Deallocate any memory that was allocated by the hash table.
 *
 * \param ht  Pointer to the hash table to deallocate.
 */
void htable_destroy(htable_t *ht);

/**
 * \brief Insert an element into a table.
 *
 * \param ht    Pointer to the hash table to remove from.
 * \param key   Key to insert.
 * \param value Value corresponding to the key.
 * \return true if the insertion suceeded, false if the table is full.
 */
bool htable_insert(htable_t *ht, uint64_t key,
		   void const *value);

/**
 * \brief Query the existance of an element in a table.
 *
 * \param ht   Pointer to the hash table to remove from.
 * \param key  Key to look up.
 * \return true if the object exists, false if not.
 */
bool htable_exists(htable_t const *ht, uint64_t key);

/**
 * \brief Remove an element from the table by its key.
 * 
 * \param ht   Pointer to hash table to remove from.
 * \param key  Key to remove.
 */
void htable_remove(htable_t *ht, uint64_t key);

#endif /* STRUCT_HTABLE_H */
