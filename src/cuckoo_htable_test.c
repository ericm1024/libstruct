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
 * \file cuckoo_htable_test.c
 *
 * \author Eric Mueller
 *
 * \brief Tests for the hash table defined in cuckoo_htable.h
 */

#include "test.h"
#include "cuckoo_htable.h"
#include <stdlib.h>
#include <time.h>

/*
 * what needs to be tested:
 *
 * 6. performance / algorithmic correctness:
 *     - inserting the entire 32 bit unsigned int namespace should suceed with
 *       reasonable load. (note -- this requires a metric shitload of memory)
 *       Each 64 byte bucket holds 4 entries. We'd need 2^32 entries so 2^30
 *       buckets, meaning 2^36 bytes of memory, i.e. 64 GiB. I don't know
 *       about you, but I don't have that much memory.
 */

#define n 100
struct value { int x; int y; };	

/* 
 * 1. init & destroy:
 *     - no memory leaks (valgrind will catch this)
 *     - table and stash members are non-null after initialization.
 *     - table should be nulled out after operation.
 */
void test_init_destroy()
{
	CUCKOO_HASH_TABLE(t);

	/* call init */
	cuckoo_htable_init(&t, 16);

	ASSERT_TRUE(t.capacity != 0, "size was zero after initialisation.\n");
	ASSERT_TRUE(t.nentries == 0,
		    "entries was not zero despite no insertions.\n");

	/* call destroy */
	cuckoo_htable_destroy(&t);

	ASSERT_TRUE(t.capacity == 0 && t.nentries == 0,
		    "table was not zeroed after calling destroy.\n");

	/* and valgrind will check for leaks */
}

/*
 * 2. insert:
 *     - If we insert the same key twice, the table should not change
 *       after the second insertion. (i.e. no double insertion)
 *     - We should be able to insert NULL values.
 *     - The table should resize to accomidate many insertions.
 *     - inserting an element should increase the element counter in the table
 */
void test_insert_same()
{
	CUCKOO_HASH_TABLE(t);
	cuckoo_htable_init(&t, 16);

	int x = rand();
	
	ASSERT_TRUE(cuckoo_htable_insert(&t, x, NULL), "insert failed.\n");
	ASSERT_TRUE(t.nentries == 1, "entries was not incrememnted.\n");
	
	ASSERT_TRUE(cuckoo_htable_exists(&t, x), "exists does not "
		    "return true for inserted element.\n");

	ASSERT_TRUE(cuckoo_htable_insert(&t, x, NULL), "insert failed.\n");
	ASSERT_TRUE(t.nentries == 1, "entries was incrimented "
		    "after inserting same element.\n");
	
	cuckoo_htable_destroy(&t);
}

void test_insert_few()
{
	CUCKOO_HASH_TABLE(t);
	cuckoo_htable_init(&t, 2*n);
	
	for (size_t i = 0; i < n; i++) {
		ASSERT_TRUE(cuckoo_htable_insert(&t, i, NULL),
			    "insert failed.\n");
                ASSERT_TRUE(cuckoo_htable_exists(&t, i), "exists returns "
			    "false imediately after inserting\n");
		ASSERT_TRUE(t.nentries == i + 1,
			    "entries was not incremented properly.\n");
        }

	for (size_t i = 0; i < n; i++)
		ASSERT_TRUE(cuckoo_htable_exists(&t, i),
			    "elements were clobered on insertion.\n");

	cuckoo_htable_destroy(&t);
}

void test_insert_many()
{
	CUCKOO_HASH_TABLE(t);
	cuckoo_htable_init(&t, 1);
	size_t initial_size = t.capacity;
	
	for (size_t i = 0; i < n; i++) {
		ASSERT_TRUE(cuckoo_htable_insert(&t, i, NULL),
			    "insert failed.\n");
                ASSERT_TRUE(cuckoo_htable_exists(&t, i), "exists returns "
			    "false imediately after inserting\n");
		ASSERT_TRUE(t.nentries == i + 1,
			    "entries was not incremented properly.\n");
        }

	for (size_t i = 0; i < n; i++)
		ASSERT_TRUE(cuckoo_htable_exists(&t, i),
			    "elements were clobered on insertion.\n");
	
	size_t new_size = t.capacity;

	ASSERT_TRUE(new_size > initial_size,
		    "table did not resize propperly.\n");

	cuckoo_htable_destroy(&t);
}

/*
 * 3. exists:
 *     - After inserting stuff, exists should return true for that key.
 *     - Exists should return false for things we haven't inserted.
 */
void test_exists()
{
	CUCKOO_HASH_TABLE(t);
	cuckoo_htable_init(&t, 16);

	uint64_t keys[2*n];
	struct value data[n];

	for (size_t i = 0; i < n; i++) {
		keys[i] = i;
		data[i].x = rand();
		data[i].y = rand();
		ASSERT_TRUE(cuckoo_htable_insert(&t, keys[i], &(data[i])),
			    "test_exists: insert failed.\n");
	}

	/* initialize the rest of the keys (for negative asserts) */
	for (size_t i = n; i < 2*n; i++)
		keys[i] = rand();

	for (size_t i = 0; i < n; i++) {
		ASSERT_TRUE(cuckoo_htable_exists(&t, keys[i]), "test_exists: exists "
			    "returned false for inserted key.\n");
		ASSERT_FALSE(cuckoo_htable_exists(&t, keys[i+n]), "test_exists: exists "
			     "returned true for key that was not inserted.\n");
	}

	cuckoo_htable_destroy(&t);
}

/*
 * 4. remove:
 *     - After inserting an element, we should be able to remove it.
 *     - Removing an elememt should decrease the element count of the table.
 *     - Removing an element that does not exist should not decrease the
 *       element count of the tabe.
 */
void test_remove()
{
	CUCKOO_HASH_TABLE(t);
	cuckoo_htable_init(&t, 16);

	uint64_t keys[n];
	struct value data[n];

	for (size_t i = 0; i < n; i++) {
		keys[i] = i;
		data[i].x = rand();
		data[i].y = rand();
		ASSERT_TRUE(cuckoo_htable_insert(&t, keys[i], &(data[i])),
			    "insert failed.\n");
	}
	ASSERT_TRUE(t.nentries == n, "nentries was wrong after "
		    "inserting.\n");
	
	/* initialize the rest of the keys (for negative asserts) */
	for (size_t i = n; i < 2*n; i++) {
		cuckoo_htable_remove(&t, i);
		ASSERT_TRUE(t.nentries == n, "nentries was decremented "
			    "after removing key not in table.\n");
	}

	for (size_t i = 0; i < n; i++) {
		cuckoo_htable_remove(&t, i);
		ASSERT_TRUE(t.nentries == n - (i+1), "nentries was "
			    "not decremented after removal.\n");
		ASSERT_FALSE(cuckoo_htable_exists(&t, i), "exists "
			     "returns true for removed element.\n");
	}
	
	cuckoo_htable_destroy(&t);
}

/*
 * 5. get:
 *     - After inserting an element, we should be able to get it out.
 *     - calling get on a key that we have not inserted should not modify
 *       the 'out' paramater.
 */
void test_get()
{
	CUCKOO_HASH_TABLE(t);
	cuckoo_htable_init(&t, 512);

	uint64_t keys[2*n];
	struct value data[n];

	for (size_t i = 0; i < n; i++) {
		keys[i] = i;
		data[i].x = rand();
		data[i].y = rand();
		ASSERT_TRUE(cuckoo_htable_insert(&t, keys[i], &(data[i])),
			    "insert failed.\n");
	}

	/* initialize the rest of the keys (for negative asserts) */
	for (size_t i = n; i < 2*n; i++)
		keys[i] = rand();

	for (size_t i = 0; i < n; i++) {
		void const *val;
		ASSERT_TRUE(cuckoo_htable_get(&t, keys[i], &val),
			    "get returned false for inserted key.\n");
		struct value *cast_value = (struct value *)val;
		ASSERT_TRUE(cast_value->x == data[i].x
			    && cast_value->y == data[i].y, "value returned "
			    "by get does not match inserted value.\n");
		val = NULL;
		ASSERT_FALSE(cuckoo_htable_get(&t, keys[i+n], &val), "get "
			     "returned true for key not inserted.\n");
		ASSERT_TRUE(val == NULL, "get modifies out paramater "
			    "even though no value was found.\n");
	}

	cuckoo_htable_destroy(&t);
}


int main(void) 
{
	srand(time(NULL));
	REGISTER_TEST(test_init_destroy);
	REGISTER_TEST(test_insert_same);
	REGISTER_TEST(test_insert_few);
	REGISTER_TEST(test_insert_many);
	REGISTER_TEST(test_exists);
	//REGISTER_TEST(test_remove);
	REGISTER_TEST(test_get);
	return run_all_tests();
}


