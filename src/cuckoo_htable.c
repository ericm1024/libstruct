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
 * \file cuckoo_htable.c
 *
 * \author Eric Mueller
 * 
 * \brief Implementation of a hash table using cuckoo hashing "with a stash"
 * as described here.
 * 
 *     http://research.microsoft.com/pubs/73856/stash-full.9-30.pdf
 *
 * \detail I've also made a few general performance optimizations based on this
 * paper.
 * 
 *     http://domino.research.ibm.com/library/cyberdig.nsf/papers/DF54E3545C82E8A585257222006FD9A2/$File/rc24100.pdf
 *     
 * In partiular, the size of each bucket in the hash table is chosen so that a
 * whole bucket fits snugly within a cache line. In general, hash tables
 * exibit poor performance as soon as they spill out of L3 cache, as each
 * 'probe' results in main memory access. Typically each access is a read or
 * write for a one machine-word key sized or pointer.
 *
 * In all schemes but linear and to a lesser extent quadratic probing, the rest
 * of the cacheline will never be used. We can make use of it by having larger
 * buckets. Fewer, larger buckets mean that we should have to traverse fewer of
 * them durring a probe, in turn meaning less main memory accesses per probe.
 *
 * The ibm paper above suggests using SIMD instructions to further speed up
 * operations with large buckets, and I'd like to implement that in a later
 * version.
 */

#include "cuckoo_htable.h"
#include "util.h"
#include "fasthash.h"
#ifdef _POSIX_C_SOURCE
  #undef _POSIX_C_SOURCE
#endif
#define _POSIX_C_SOURCE 200112L
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <math.h>

#define CACHELINE (64)
#define BUCKET_SIZE (CACHELINE/(sizeof(uint64_t)+sizeof(void*)))

/* hash function wrapper */
static inline uint64_t cuckoo_hash(uint64_t key, uint64_t seed)
{
	return fasthash64(&key, sizeof key, seed);
}

/*
 * \brief itterate through all the "nests" (buckets) in which a given key
 * could live.
 * \param table        Pointer to a struct cuckoo_table
 * \param bucket_name  (token) name of the bucket loop variable to declare --
 *                     this is the identifier to use within the loop
 * \param key          The key whose nests we're iterating over.
 *
 * \note This macro is a double loop, you won't be able to get out of it
 * with a break, you'll need a goto (which I don't think is ever needed
 * in this code).
 *
 * \detail In essence, this is the core of the cuckoo table-walking
 * algorithm. For the ith array in the abstract "table", we get the bucket
 * indexed by the hash seeded with the ith seed.
 *
 * implementation: The outer loop does the actual work (i.e. looping over
 * the tables), and the inner loop just exists so that we have a space to
 * assign the bucket. The '__j' variable exists to make the inner loop
 * run just once. I know this macro is disgusting, but it makes the later
 * code so much prettier later because this loop is so so common.
 */ 
#define for_each_nest(__table, bucket_name, __key)			\
	for (unsigned long __i = 0, __j = 0;				\
	     __i < CUCKOO_HTABLE_NTABLES;				\
	     __i++, __j = 0)						\
		for (struct cuckoo_bucket *bucket_name =		\
			     &(__table)->tables[__i][cuckoo_hash(__key,	\
					     (__table)->seeds[__i])	\
					   % (__table)->capacity];	\
		     __j < 1; __j++)

#define for_each_bucket(__table, bucket_name)				\
	for (unsigned long __i = 0;					\
	     __i < CUCKOO_HTABLE_NTABLES;				\
	     __i++)							\
		for (unsigned long __j = 0, __k = 0;			\
		     __j < (__table)->capacity;				\
		     __j++, __k = 0)					\
			for (struct cuckoo_bucket *bucket_name =	\
				    &(__table)->tables[__i][__j];       \
			     __k < 1;					\
			     __k++)


/* ======= bucket struct and methods ======= */

#define TAG_OCCUPIED ((uintptr_t)0x1)
#define TAG_INVALID ((uintptr_t)0x2)
#define TAG_WIDTH (TAG_OCCUPIED | TAG_INVALID)

struct cuckoo_bucket {
	uint64_t keys[BUCKET_SIZE];
	
	/*
	 * the lowest bit of each pointer in this array is set high to
	 * denote an occupied kv-pair, low to denote an unoccupied pair.
	 * The other option is to store NULL pointers to denote empty
	 * buckets, but that prevents us from actually storing NULL pointers,
	 * which is a somewhat aritrary limitation we want to avoid.
	 * We could also keep an integer counter variable, but that
	 * takes a lot of space over all buckets and screws over our memory
	 * allignment.
	 */
	union {
		const void *ptrs[BUCKET_SIZE];
		uintptr_t tags[BUCKET_SIZE];
	} vals;
};

static inline void set_val(struct cuckoo_bucket *restrict bkt,
			   const void *restrict val,
			   unsigned long i)
{
	bkt->vals.ptrs[i] = val;
	bkt->vals.tags[i] |= TAG_OCCUPIED;
}

/* remove a value from a bucket by clearing the tag bit */
static inline const void *remove_val(struct cuckoo_bucket *bkt,
				     unsigned long i)
{
	const void *val = bkt->vals.ptrs[i];
	bkt->vals.tags[i] = 0;
	return val;
}

/* retrieve a value (pointer) from the bucket */
static inline const void *get_val(const struct cuckoo_bucket *bkt,
				  unsigned long i)
{
	return (void*)(bkt->vals.tags[i] & ~TAG_WIDTH);
}

static inline uint64_t get_key(const struct cuckoo_bucket *bkt,
			       unsigned long i)
{
	return bkt->keys[i];
}

static inline void set_key(struct cuckoo_bucket *bkt, uint64_t key,
			   unsigned long i)
{
	bkt->keys[i] = key;
}

static inline bool slot_has_tag(const struct cuckoo_bucket *bkt,
				unsigned long i, uintptr_t tag)
{
	return bkt->vals.tags[i] & tag;
}

static inline void set_tag(struct cuckoo_bucket *bkt, unsigned long i,
			   uintptr_t tag)
{
	bkt->vals.tags[i] |= tag;
}

static inline void clear_tag(struct cuckoo_bucket *bkt, unsigned long i,
			     uintptr_t tag)
{
	bkt->vals.tags[i] &= ~tag;
}

static bool bucket_insert(struct cuckoo_bucket *restrict bkt,
			  uint64_t *restrict caller_key,
			  const void **restrict caller_val)
{
	unsigned long i;
	uint64_t key = *caller_key;
	const void *val = *caller_val;
	bool did_evict = false;

	/* find the first empty slot */
	for (i = 0; i < BUCKET_SIZE; i++)
		if (!slot_has_tag(bkt, i, TAG_OCCUPIED))
			break;

	/* bucket was full, kick out the last kv-pair */
	if (i == BUCKET_SIZE) {
		i--;
		*caller_key = get_key(bkt, i);
		*caller_val = get_val(bkt, i);
		did_evict = true;
	}

	/* insert new kv-pair */
	set_val(bkt, val, i);
	set_key(bkt, key, i);

	return !did_evict;
}

static bool bucket_insert_rehash(struct cuckoo_bucket *restrict bkt,
				 uint64_t *restrict caller_key,
				 const void **restrict caller_val)
{
	unsigned long i;
	uint64_t key = *caller_key;
	const void *val = *caller_val;
	bool did_evict = false;

	/* try to find an empty slot */
	for (i = 0; i < BUCKET_SIZE; i++)
		if (!slot_has_tag(bkt, i, TAG_OCCUPIED))
			break;

	/* if we couldn't find an empty slot, look for an invalid slot */
	if (i == BUCKET_SIZE)
		for (i = 0; i < BUCKET_SIZE; i++)
			if (slot_has_tag(bkt, i, TAG_INVALID))
				break;

	/* we got to the end */
	if (i == BUCKET_SIZE)
		i--;
	
	/* if the slot in question has something in it, kick it out */
	if (slot_has_tag(bkt, i, TAG_OCCUPIED)) {
		*caller_key = get_key(bkt, i);
		*caller_val = get_val(bkt, i);
		did_evict = true;
	}

	/* insert new kv-pair */
	set_val(bkt, val, i);
	set_key(bkt, key, i);

	return !did_evict;	
}

static inline bool bucket_contains(const struct cuckoo_bucket *bkt,
				   uint64_t key)
{
	/* walk through the bucket and look for the key */
	for (unsigned long i = 0; i < BUCKET_SIZE; i++) {
		if (!slot_has_tag(bkt, i, TAG_OCCUPIED))
			break;
		else if (get_key(bkt, i) == key)
			return true;
	}
	return false;	
}

static bool try_bucket_remove(struct cuckoo_bucket *bkt, uint64_t key)
{
	unsigned long i;
	
	/*
	 * look for the key. If we get to an empty slot, we didn't find it,
	 * so bail.
	 */
	for (i = 0; i < BUCKET_SIZE; i++) {
		if (!slot_has_tag(bkt, i, TAG_OCCUPIED))
			return false;
		else if (get_key(bkt, i) == key) {
			remove_val(bkt, i);
			break;
		}
	}
	
	/* move all the other kv-pairs back one slot */
	for (; i < BUCKET_SIZE - 1; i++) {
		set_val(bkt, remove_val(bkt, i+1), i);
		set_key(bkt, get_key(bkt, i+1), i);
	}
	
	return true;
}

static bool try_bucket_get(const struct cuckoo_bucket *restrict bkt,
			   uint64_t key, const void **restrict val)
{
	for (unsigned long i = 0; i < BUCKET_SIZE; i++) {
		if (!slot_has_tag(bkt, i, TAG_OCCUPIED))
			break;
		else if (get_key(bkt, i) == key) {
			*val = get_val(bkt, i);
			return true;
		}
	}
	
	return false;
}



/* ======= initialization and destruction methods ======= */

/* alligned, zeroed allocation */
static inline void *alligned_zalloc(size_t allignment, size_t size)
{
	void *mem = NULL;
	posix_memalign(&mem, allignment, size);
	if (mem)
		memset(mem, 0, size);
	return mem;
}

/* allocate all arrays for a cuckoo hash table and initialize seeds */ 
static bool alloc_table(struct cuckoo_table *table, unsigned long entries)
{
	unsigned long i = 0;
	
	for (; i < CUCKOO_HTABLE_NTABLES; i++) {
		table->seeds[i] = pcg64_random();
		table->tables[i] = alligned_zalloc(CACHELINE,
				       entries*sizeof(struct cuckoo_bucket));
		if (!table->tables[i])
			goto failed_alloc;
	}
	table->capacity = entries;
	return true;

failed_alloc:
	while (i-- > 0)
		free(table->tables[i]);
	return false;
}

/* free all memory in a table */
static void free_table(struct cuckoo_table *table)
{
	for (unsigned long i = 0; i < CUCKOO_HTABLE_NTABLES; i++) {
		free(table->tables[i]);
		table->tables[i] = NULL;
	}
}

/* init rng, get rands, allocate memory, initialize members of head */ 
bool cuckoo_htable_init(struct cuckoo_head *head,
			unsigned long capacity)
{
	if (!seed_rng())
		return false;
	
	if (!alloc_table(&head->table, capacity/CUCKOO_HTABLE_NTABLES + 1))
		return false;

	head->capacity = capacity;		
	head->nentries = 0;
	return true;
}

/* free all memory, zero out all the members of head */ 
void cuckoo_htable_destroy(struct cuckoo_head *head)
{
	free_table(&head->table);
	head->nentries = 0;
	head->capacity = 0;
}



/* ======= insertion, deletion, and query methods ======= */

#define MAX_INSERT_TRYS_MULTIPLIER (4UL)

static inline unsigned long max_insert_trys(unsigned long entries)
{
	return MAX_INSERT_TRYS_MULTIPLIER*(log(entries) + 1);
}

static inline bool needs_resize(const struct cuckoo_head *head)
{
	/* this value imples half of the buckets are full */
	unsigned long threshold = CUCKOO_HTABLE_NTABLES
				* (BUCKET_SIZE - 1)
				* head->table.capacity;
	
	return head->nentries >= threshold;
}

/*
 * inline version with callback. We use this to make do_insert and
 * do_insert_rehash
 */ 
static inline bool __do_insert(struct cuckoo_table *restrict table,
			       uint64_t *restrict key,
			       void const **restrict val,
			       unsigned long max_trys,
			       bool (*bkt_insert)(
				       struct cuckoo_bucket *restrict,
				       uint64_t *restrict,
				       const void **restrict))
{
	/*
	 * idea: keep looping until bucket_insert succeedes. Each time its
	 * called, the key and value arguments get reassigned with a
	 * new key/value, which we then try to insert on the next itteration.
	 * 
	 * This is the "cuckoo" part of cuckoo hashing -- kicking out old
	 * values when we find fully occupied buckets.
	 */
	for (unsigned long i = 0, which_array = 0;
	     i < max_trys; i++, which_array++) {
		which_array %= CUCKOO_HTABLE_NTABLES;
		
		uint64_t hash = cuckoo_hash(*key, table->seeds[which_array]);
		hash %= table->capacity;
		
		struct cuckoo_bucket *b = &table->tables[which_array][hash];
		if (bkt_insert(b, key, val))
			return true;
	}
	return false;
}

static bool do_insert(struct cuckoo_table *restrict table,
		      uint64_t *restrict key, void const **restrict val,
		      unsigned long max_trys)
{
	return __do_insert(table, key, val, max_trys, bucket_insert);
}

static bool do_insert_rehash(struct cuckoo_table *restrict table,
			     uint64_t *restrict key,
			     void const **restrict val,
			     unsigned long max_trys)
{
	return __do_insert(table, key, val, max_trys, bucket_insert_rehash);
}

/*
 * \brief Resize a table.
 * \param head       The hash table to resize.
 * \param new_size   The number of buckets to allocate for each of the
 *                   CUCKOO_HTABLE_NTABLES arrays.
 * \return true on success, false otherwise.
 */ 
static bool do_resize(struct cuckoo_head *head, unsigned long new_size)
{
	printf("resizing\n");
	
	unsigned long trys = max_insert_trys(head->nentries);
	struct cuckoo_table new_table;

	/* allocate a new table */
	new_table.capacity = new_size;
	if (!alloc_table(&new_table, new_size))
		return false;

	/*
	 * for each array in the old table
	 *   for each bucket in array 
	 *     for each entry in the bucket 
	 *       insert it into the new table
	 */
	for_each_bucket(&head->table, b) {
		for (unsigned long i = 0; i < BUCKET_SIZE; i++) {
			if (!slot_has_tag(b, i, TAG_OCCUPIED))
				continue;
			uint64_t key = get_key(b, i);
			const void *val = get_val(b, i);
			if (!do_insert(&new_table, &key, &val, trys))
				goto failed_insert;
		}
	}

	/* free the old table and assign the new one */
	free_table(&head->table);
	head->table = new_table;
	head->capacity = new_size * CUCKOO_HTABLE_NTABLES * BUCKET_SIZE;
	return true;

failed_insert:
	free_table(&new_table);
	return false;
}

static unsigned long count_entries(struct cuckoo_table *table)
{
	unsigned long count = 0;
	for_each_bucket(table, b) {
		for (unsigned long i = 0; i < BUCKET_SIZE; i++)
			if (slot_has_tag(b, i, TAG_OCCUPIED))
				count++;
	}
	return count;
}

static void print_entries(struct cuckoo_table *table)
{
	for_each_bucket(table, b) {
		for (unsigned long i = 0; i < BUCKET_SIZE; i++)
			if (slot_has_tag(b, i, TAG_OCCUPIED))
				printf("%lu\n", get_key(b, i));
	}
	printf("\n");
}

/*
 * \brief reseed the keys of a table and rehash all the elements
 * 
 * \param table  The table to rehash
 * \param trys   The number of times to try each insertion
 *
 * \detail Rehashing is implemented in place. First every occupied slot in
 * the table is marked as invalid, meaning that the key is not in the slot
 * that corresponds to its hash. Then every key in an invalid slot is
 * evicted, rehashed, and reinserted.
 */ 
static void do_rehash(struct cuckoo_table *table, unsigned long trys)
{
	printf("rehashing\n");
	const void *val;
	uint64_t key;
	bool has_orphan = false;

again:
	printf("entries is %lu\n", count_entries(table));
	//print_entries(table);
	
	/* reseed the hash functions */
	for (unsigned long i = 0; i < CUCKOO_HTABLE_NTABLES; i++)
		table->seeds[i] = pcg64_random();

	/*
	 * mark all entries as invalid, i.e. denote that the keys are now
	 * not in the right buckets as the hash functions changed.
	 */
	for_each_bucket(table, b) {
		for (unsigned long i = 0; i < BUCKET_SIZE; i++)
			if (slot_has_tag(b, i, TAG_OCCUPIED))
				set_tag(b, i, TAG_INVALID);
	}

	/* reinsert an outstanding k-v pair if we have one */
	if (has_orphan) {
		/* this should never fail */
		assert(do_insert_rehash(table, &key, &val, trys));
		has_orphan = false;
	}
		
	for_each_bucket(table, b) {
		for (unsigned long i = 0; i < BUCKET_SIZE; i++) {
			if (!slot_has_tag(b, i, TAG_INVALID)
			    || !slot_has_tag(b, i, TAG_OCCUPIED))
				continue;
			
			key = get_key(b, i);
			val = remove_val(b, i);
			
			if (!do_insert_rehash(table, &key,
					      &val, trys)) {
				has_orphan = true;
				printf("again\n");
				goto again;
			}
		}
	}

	for_each_bucket(table, b) {
		for (unsigned long i = 0; i < BUCKET_SIZE; i++)
			clear_tag(b, i, TAG_INVALID);
	}
}

/*
 * inset a key-value pair into the table. Because of the evicting nature
 * of the cuckoo insertion algorithm, our insertion helpers take
 * pointers to keys/values so they can return key/values that they evict.
 * they need something to point to, so we keep the "anchor" key and value
 * in this function's stack frame.
 */ 
bool cuckoo_htable_insert(struct cuckoo_head *head, uint64_t key,
			  void const *val)
{
	unsigned long count = count_entries(&head->table);
	if (count != head->nentries)
		printf("expected nentreies == %lu, got %lu\n",
		       head->nentries, count);
	
	uint64_t key_anchor = key;
	const void *val_anchor = val;
	bool ret = true;
	unsigned long trys = max_insert_trys(head->nentries);

	/* if it exists, yay */
	if (cuckoo_htable_exists(head, key))
		return ret;

	/* try inserting */
	head->nentries++;
	if (do_insert(&head->table, &key_anchor, &val_anchor, trys))
		return ret;

	/* if we need to resize, do that */
	if (needs_resize(head)) {
		if (!do_resize(head, head->table.capacity*2)) {
			/*
			 * this is nasty - if we failed resizing, we need
			 * to evict the key we originally inserted and
			 * reinsert the orphan key so that the table
			 * remains in a consistent state
			 */
			ret = false;
			head->nentries--;

			/*
			 * extremely unlikely case where the last evicted key
			 * was the insertee, in which case we're done
			 */
			if (key == key_anchor)
				return ret;

			cuckoo_htable_remove(head, key);
		}
		
		if (do_insert(&head->table, &key_anchor, &val_anchor, trys))
			return ret;
	}
	
	/* otherwise we need to rehash */
	do {
		do_rehash(&head->table, trys);
	} while (!do_insert(&head->table, &key_anchor, &val_anchor, trys));
	
	return ret;
}
			
bool cuckoo_htable_exists(struct cuckoo_head const *head, uint64_t key)
{
	for_each_nest(&head->table, b, key) {
		if (bucket_contains(b, key))
			return true;
	}
	return false;
}

void cuckoo_htable_remove(struct cuckoo_head *head, uint64_t key)
{
	for_each_nest(&head->table, b, key) {
		if (try_bucket_remove(b, key)) {
			head->nentries--;
			return;
		}
	}
}

bool cuckoo_htable_get(struct cuckoo_head const *restrict head,
		       uint64_t key, void const **restrict out)
{
	for_each_nest(&head->table, b, key) {
		if (try_bucket_get(b, key, out))
			return true;
	}	
	return false;
}

bool cuckoo_htable_resize(struct cuckoo_head *head, bool grow)
{
	if (head->nentries <= head->capacity/4 && !grow)
		return do_resize(head, head->table.capacity/2);
	else if (grow)
		return do_resize(head, head->table.capacity*2);
	else
		return false;
}
