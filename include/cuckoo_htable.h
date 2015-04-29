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
 * \file cuckoo_htable.h
 *
 * \author Eric Mueller
 *
 * \brief Header file for a hash table.
 *
 * \detail This hash table has fairly standard behavior. To create one,
 * use the HASH_TABLE macro, ex:
 *
 *   CUCKOO_HASH_TABLE(my_table)
 *
 * then call cuckoo_htable_init to do the initial memory allocations. At that
 * point the full hash table api can be used. When finished with the table,
 * call cuckoo_htable_destroy to free all memory associated with the table.
 *
 * ** TODO DISCUSS API COMPLEXITY **
 *
 * Synchronization is left to the caller.
 *
 * IMPORTANT NOTE: Because of the colision resolution algorithm used by this
 * hash table, it can not handle multiple insertions of the same key. To be
 * clear, inserting multiple of the same key is undefined behavior. If you're
 * not sure if your key already exists in the table, ask with a call to
 * cuckoo_htable_exists
 */

#ifndef STRUCT_CUCKOO_HTABLE_H
#define STRUCT_CUCKOO_HTABLE_H 1

#include <stdbool.h>
#include <stdint.h>

/*
 * number of arrays and hash functions to use (this is a trait specific to
 * cuckoo hashing).
 */
#define CUCKOO_HTABLE_NTABLES (2U)

/* hash table types */
struct cuckoo_bucket;

/* note -- you should not declare one of these yourself */
struct cuckoo_table {
	/*
	 * nr of buckets in each table -- NOT the actual capacity of this
	 * entire struct or a single array. That is given by the capacity
	 * field of a struct cuckoo_head
	 */
	unsigned long capacity;
	
	/* buckets where key-value pairs are stored */ 
	struct cuckoo_bucket *tables[CUCKOO_HTABLE_NTABLES];
	
	/*
	 * seeds for the hash function. We need CUCKOO_HTABLE_NTABLES
	 * independent hash functions, but it is sufficient to use
	 * CUCKOO_HTABLE_NTABLES seeds for the same function, as long
	 * as we have good random seeds.
	 */ 
	uint64_t seeds[CUCKOO_HTABLE_NTABLES];
};

struct cuckoo_head {
	/* number of key-value pairs currently contained in the table */
	unsigned long nentries;

	/* maximum number of key-value pairs that we can store */
	unsigned long capacity;

	/* the actual table */
	struct cuckoo_table table;
};

/**
 * \brief Declare a hash table head.
 *
 * \param name  (token) The name of the hash table to declare.
 */
#define CUCKOO_HASH_TABLE(name)				\
	struct cuckoo_head name = {			\
		.nentries = 0,				\
		.capacity = 0,				\
                .table = {				\
		        .capacity = 0,			\
		        .tables = {0}}};

/**
 * \brief Initialize a hash table of a given size.
 *
 * \param ht        Pointer to the hash table to initialize.
 * \param capacity  How many insertions to allocate space for (upper bound).
 * \return true on success or false if table allocation failed.
 */
bool cuckoo_htable_init(struct cuckoo_head *head,
			unsigned long capacity);

/**
 * \brief Deallocate any memory that was allocated by the hash table.
 * \param ht  Pointer to the hash table to deallocate.
 */
void cuckoo_htable_destroy(struct cuckoo_head *head);

/**
 * \brief Insert an element into a table.
 *
 * \param ht    Pointer to the hash table to remove from.
 * \param key   Key to insert.
 * \param value Value to insert along with the key.
 * \return true if the insertion suceeded, false if the table is full. Note
 *         that if the inserted key already exists, insert will return true
 *         without modifying the table.
 */
bool cuckoo_htable_insert(struct cuckoo_head *head, uint64_t key,
			  void const *value);

/**
 * \brief Query the existance of an element in a table.
 *
 * \param ht   Pointer to the hash table to remove from.
 * \param key  Key to look up.
 * \return true if the object exists, false if not.
 */
bool cuckoo_htable_exists(struct cuckoo_head const *head,
			  uint64_t key);

/**
 * \brief Remove an element from the table.
 * 
 * \param ht   Pointer to hash table to remove from.
 * \param key  Key to remove.
 */
void cuckoo_htable_remove(struct cuckoo_head *head, uint64_t key);

/**
 * \brief Get the value corresponding to a key, if such a key exists.
 *
 * \param ht   Pointer to the hash table to search.
 * \param key  Key to searh for.
 * \param out  If a value is found, it is put here.
 * \return true if a value corresponding to the given key was found, false if
 *         it was not found.
 */
bool cuckoo_htable_get(struct cuckoo_head const *restrict head,
		       uint64_t key, void const **restrict out);

/**
 * \brief Begin the resizing process for a hash tabe.
 * \param ht        The hash table to resize.
 * \param grow      True if the table should grow, false to shrink.
 * \return true if the resize is sucesful, false if memory allocation
 * fails.
 */
bool cuckoo_htable_resize(struct cuckoo_head *head, bool grow);

#endif /* STRUCT_HTABLE_H */
