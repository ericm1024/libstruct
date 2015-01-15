/**
 * \file htable.c
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

#include "htable.h"
#include "murmurhash.h"
#include "fasthash.h"
#include <assert.h> 
#include <stdint.h>
#include <stdlib.h> /* posix_memalign and rand */ 
#include <math.h> /* log */
#include <string.h> /* memset */

/*
 * TODO: handle this better. We can hack memory allignment ourselves, I'm just
 * being lazy for the first pass.
 */
#if _POSIX_C_SOURCE < 200112L && _XOPEN_SOURCE < 600
#error "Error. Don't have posix_memalign"
#endif

/*
 * Cachelines are 64 bytes. Yes this isn't portable but I can't find a macro for
 * it.
 */
#define CACHELINE ((unsigned)64)

/*
 * On 64 bit systems this works out to 4, but to 32 bit systems it works out
 * to 5 with 4 bytes wasted.
 */ 
#define BUCKET_SIZE (CACHELINE / (sizeof (uint64_t) + sizeof (void*)))

/*
 * We need some way of identifying which buckets are used currently. We could
 * do this by keeping a counter of how many buckets are in use, but that
 * takes a big percentage of the space in each bucket, and forces us to keep
 * one less key / value in each bucket, and ultimately wastes a lot of space
 * (remember we're going to have an array of possibly millions of bucket_t's).
 *
 * We could treat NULL values as being empty, but that would prevent NULL
 * values from being inserted, which is an arbitrary limitation that we want
 * to avoid. We comprimize by using the least signifficant bit. 0 signifies
 * that the bucket is not in use, 1 signifies that it is. Hence why uintptr_t
 * is typedef'd to value_t.
 */

typedef uintptr_t value_t;

/*
 * TODO: Does this need an __attrubute__((aligned(CACHELINE))) ? If so, where
 * in the declaration should I put it
 */
struct bucket {
	uint64_t keys[BUCKET_SIZE];
	value_t values[BUCKET_SIZE];
};
typedef struct bucket bucket_t;

static inline bool is_empty(value_t val)
{
	return val & 1;
}

static inline void * get_value(value_t val)
{
	return (void *)(val & ~1);
}

static inline void set_value(value_t *target, void const *val)
{
	assert(((uintptr_t)val & 1) == 0);
	*target = (uintptr_t)val;
	*target |= 1;
}

static inline void make_empty(value_t *target)
{
	*target = 0;
}

/*
 * Max number of times to kick out an element and reinsert the evicted element
 * (i.e. max number of times to itterate) during insertion.
 * 
 * The literature says that, if you've reinserted more than \alpha log (n)
 * times, then your insertion has hit a cycle and the table needs to be
 * rehashed. However, no one seems to specify any good guesses for \alpha.
 * So we're just gonna set it at 16 and cross our fingers real tight.
 */ 
static inline size_t max_tries(size_t tsize)
{
	size_t ntries = log((tsize * BUCKET_SIZE))/log(2.0);
	ntries = ntries > 0 ? ntries : 1;
	return ntries * 16;
}

/*
 * returns true if the insertion sucedded without having to kick anything out,
 * otherwise false.
 *
 * TODO: we may want to compare the key against existing keys to make sure
 * the same value was not inserted, because that will fuck us over real bad.
 */
static inline bool insert_into_bucket(bucket_t *bucket,
				      uint64_t *key,   /* in & out*/
				      void const **value)  /* in & out */
{
	/* look for an empty slot in the bucket an insert into it */
	size_t i = 0;
	for ( ; i < BUCKET_SIZE; i++)
		if (is_empty(bucket->values[i]))
			break;

	/* grab whatever was at index i previously, then insert */
	uint64_t tmp_key = bucket->values[i];
	void *tmp_val = get_value(bucket->values[i]);
	bucket->keys[i] = *key;
 	set_value(&(bucket->values[i]), *value);

	/* overwrite the in/out variables */
	*key = tmp_key;
	*value = tmp_val;
	return (tmp_val == NULL);
}

static inline bool insert_into_stash(bucket_t *stash,
				     uint64_t key,
				     void const *value)
{
	/* look for an empty slot in the bucket an insert into it */
	size_t i = 0;
	for ( ; i < BUCKET_SIZE; i++)
		if (is_empty(stash->values[i]))
			break;

	/* if we got to the end of the stash, it's full */
	if (i == BUCKET_SIZE)
		return false;

	/* else insert sucessfully */
	stash->keys[i] = key;
	set_value(&(stash->values[i]), value);
	return true;
}

static inline bool bucket_contains(bucket_t *b, uint64_t key)
{
	for (size_t i = 0; i < BUCKET_SIZE; i++) {
		if (is_empty(b->values[i]))
			return false;
		else if (b->keys[i] == key)
			return true;
	}
	return false;
}

static inline bool bucket_remove(bucket_t *b, uint64_t victim)
{
	size_t i = 0;
	
	/* look for the victim */
	for ( ; i < BUCKET_SIZE; i++) {
		if (is_empty(b->values[i]))
			return false;
		else if (b->keys[i] == victim)
			break;
	}
	
	/* bail if we didn't find it */
	if (i == BUCKET_SIZE)
		return false;
	
	/* shift everything else back one */
	for ( ; i < BUCKET_SIZE - 1; i++) {
		b->keys[i] = b->keys[i+1];
		b->values[i] = b->values[i+1];
	}
	
	/* empty the end bucket */
	make_empty(&(b->values[i]));
	return true;
}	

/* wrapper functions for our two hashes */

static inline uint32_t hash1(htable_t const *ht, uint64_t key)
{
	return fasthash32((char const *)(&key), sizeof(uint64_t), ht->seed1);
}

static inline uint32_t hash2(htable_t const *ht, uint64_t key)
{
	return murmurhash((char const *)(&key), sizeof(uint64_t), ht->seed2);
}

/**
 * \brief Internal resource allocator function for the hash table.
 * 
 * \detail Attempts to allocate two arrays of size tsize * sizeof(bucket_t)
 * and one array of size ssize * sizeof(bucket_t). The two tsize'd arrays are
 * put into *table1 and *table2. The ssize'd array is put into *stash. The stash
 * argument may be null as there are cases where we may want to only allocate
 * the tables (i.e. in resize).
 * 
 * \param table1  Pointer to where table1 should be put. *table1 is overwritten
 * \param table2  Pointer to where table2 should be put. *table2 is overwritten
 * \param tsize   The size of table to allocate.
 * \param stash   Pointer to where the stash should be put. *stash is
 *                overwritten. Can be null, in which case no stash is allocated.
 * \param ssize   The size of stash to allocate.
 * \return true if all allocations succeed. false if one or more failed.
 */ 
static bool do_allocs(bucket_t **table1,
		      bucket_t **table2,
		      size_t tsize,
		      bucket_t **stash,
		      size_t  ssize)
{
	/*
	 * gotta assert this somewhere, if nothing else to ensure that I can do
	 * math, but more importantly to ensure that no weird padding happened.
	 * Particularly on systems that don't have 64 byte pointers, or with
	 * some god forsaken stdint implementation that uses some crazy size of
	 * uintptr_t.
	 */
	assert(sizeof(bucket_t) == CACHELINE);
	
	/* allocate the first table */
	int err = posix_memalign((void**)table1, CACHELINE,
				 tsize * sizeof(bucket_t));
	if (err)
		goto table1_err;
	
	/* allocate the second table */
	err = posix_memalign((void**)table2, CACHELINE,
				 tsize * sizeof(bucket_t));
	if (err)
		goto table2_err;
	
	/* allocate a stash */
	if (stash) {	
		err = posix_memalign((void**)stash, CACHELINE,
				     ssize * sizeof(bucket_t));
		if (err)
			goto stash_err;
	}		

	/* zero out everything */
	memset(*table1, 0, tsize * sizeof(bucket_t));
	memset(*table2, 0, tsize * sizeof(bucket_t));
	if (stash)
		memset(*stash, 0, ssize * sizeof(bucket_t));

	return true;

/* unwind the errors */
stash_err:
	free(*table2);
	*table2 = NULL;
table2_err:
	free(*table1);
	*table1 = NULL;
table1_err:
	return false;	
}

/**
 * \detail The leg work of insertion is done here. This function is the
 * workhorse of insert and resize. In essence, the algorithm is as follows:
 * 
 *     hash an element with h1.
 *         if it's bucket is full, evict someone from the bucket,
 *         hash them with h2, and try to insert them into the other
 *         table.
 *     We keep trying this until we've tried the number of times returned by
 *     max_tries, at which point it's highly probable that we've hit a cycle.
 *         If we hit max tries, we then try to insert into the stash. And we
 *         hope to fuck that never fails.
 *
 * \param ht     Pointer to the hash table to insert into.
 * \param key    Key to insert.
 * \param value  Value to insert.
 * \return true if the insertion suceeded, false otherwise
 */
static bool insert_no_resize(htable_t *ht,
			     uint64_t key,
			     void const *value)
{
	void const *local_value = value;
	uint64_t local_key = key;
	size_t max = max_tries(ht->size);
	
	for (size_t tries = 0; tries < max; tries++) {
		size_t hash = hash1(ht, local_key) % ht->size;
		/* try inserting into the first table */
		if (insert_into_bucket(
			    &(ht->table1[hash]), &local_key, &local_value) )
			return true;

		hash = hash2(ht, local_key) % ht->size;
		/* try inserting into the second table */
		if (insert_into_bucket(
			    &(ht->table2[hash]), &local_key, &local_value) )
			return true;
	}

	/* last ditch -- if we got this far, try inserting into the stash */
	return insert_into_stash(ht->stash, local_key, local_value);
}

/* resize the hash table to the new size */
static bool resize(htable_t *ht, size_t new_size)
{
	assert(new_size * 2 > ht->entries);
	htable_t new_ht;
	new_ht.size = new_size;
	new_ht.entries = ht->entries;
	new_ht.seed1 = ht->seed1;
	new_ht.seed2 = ht->seed2;
	
	if (!do_allocs(&new_ht.table1, &new_ht.table2, new_ht.size,
		       &new_ht.stash, 1))
		return false;
	
	/* purge and reinsert everything in the first table */
	for (size_t i = 0; i < ht->size; i++) {
		for (size_t j = 0; j < BUCKET_SIZE; j++) {
			if (is_empty(ht->table1[i].values[j]))
				break;
			/*
			 * I really hate these asserts, but for now they're here
			 * to ensure table consistency... if the assert fails,
			 * it means we hit the super rare situation where we
			 * need to resize again while we're already in the
			 * process of resizing. While not impossible, such a
			 * situation should in theory be rare to the point of
			 * being a non-issure, but if shit ever gets fucked
			 * this assert will let us know. We could solve this
			 * by reallocating again, but that could get us into
			 * an infinite loop, reallocating twice as much memory
			 * on each itteration... hardly a pretty alternative.
			 */
			assert(insert_no_resize(&new_ht,
						ht->table1[i].keys[j],
						get_value(
						ht->table1[i].values[j])));
		}
	}

	/* purge and reinset everything in the second table */
	for (size_t i = 0; i < ht->size; i++) {
		for (size_t j = 0; j < BUCKET_SIZE; j++) {
			if (is_empty(ht->table2[i].values[j]))
				break;
			/* see above wall of text */
			assert(insert_no_resize(&new_ht,
						ht->table2[i].keys[j],
						get_value(
						ht->table2[i].values[j])));
		}
	}

	/* (try to) reinsert everything from the stash */
	for (size_t i = 0; i < BUCKET_SIZE; i++) {
		if (is_empty(ht->stash->values[i]))
			break;
		insert_no_resize(&new_ht, ht->stash->keys[i],
				 get_value(ht->stash->values[i]));
	}

	/* update the head with the new tables  */
	free(ht->table1);
	free(ht->table2);
	free(ht->stash);
	*ht = new_ht;
	return true;
}

bool htable_init(htable_t *ht, size_t nbuckets)
{
	ht->size = 0;
	ht->entries = 0;
	ht->table1 = NULL;
	ht->stash = NULL;

	/*
	 * size corresponds to the actual number of bucket_t's that will be
	 * allocated in each of the two tables. We'll actually set ht->size to
	 * this after (if) do_allocs suceeds
	 */
	size_t tmp_size = (nbuckets/(BUCKET_SIZE * 2) + 1);
		
	/*
	 * because the hash functions we're using generate 32 bit hashes, we can't
	 * have a table with more than 2^32 buckets. That shouldn't be a huge
	 * limitation, and can be fixed by finding different hash functions.
	 * (currently this just entails replacing murmurr3 as fasthash has both
	 * 32 and 64 bit flavors).
	 * 
	 * TODO: lol does this macro actually exist?
	 */
	assert(tmp_size <= UINT32_MAX );
	
	if (!do_allocs(&(ht->table1), &(ht->table2), tmp_size, &(ht->stash), 1))
		return false;

	ht->size = tmp_size;
	/*
	 * TODO: use higher quality rands.
	 * rand() sucks and these are important
	 */
	ht->seed1 = rand();
	ht->seed2 = rand();
	
	return true;
}
	
void htable_destroy(htable_t *ht)
{
	ht->size = 0;
	ht->entries = 0;
	free(ht->table1);
	free(ht->table2);
	free(ht->stash);
}

bool htable_insert(htable_t *ht, uint64_t key,
		   void const *value)
{
	/*
	 * TODO: still gotta handle insetion of the same keys, as that's a
	 * no-no with cucoo hashing. Perhahs insert_no_resize should return
	 * a status flag instead of just a t/f ?
	 */
	ht->entries++;
	if (insert_no_resize(ht, key, value))
		return true;
	resize(ht, ht->size * 2);
	return insert_no_resize(ht, key, value);
}

bool htable_exists(htable_t const *ht, uint64_t key)
{
	size_t hash = hash1(ht, key) % ht->size;
	if (bucket_contains(&(ht->table1[hash]), key))
		return true;
	hash = hash2(ht, key) % ht->size;
	if (bucket_contains(&(ht->table2[hash]), key))
		return true;
	return bucket_contains(ht->stash, key);
}

void htable_remove(htable_t *ht, uint64_t key)
{
	/*
	 * TODO: should we try re-inesrting things from the stash after
	 * we evict any item from anywhere? 
         */
	
	/* look for the key in the first table */
	size_t hash = hash1(ht, key) % ht->size;
	if (bucket_remove(&(ht->table1[hash]), key)) {
		ht->entries--;
		return;
	}

	/* look for the key in the second table */
	hash = hash2(ht, key) % ht->size;
	if (bucket_remove(&(ht->table2[hash]), key)) {
		ht->entries--;
		return;
	}

	/* look for the key in the stash */
	if (bucket_remove(ht->stash, key)) {
		ht->entries--;
		return;
	}
}
