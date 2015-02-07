/**
 * \file htable_test.c
 *
 * \author Eric Mueller
 *
 * \brief Tests for the hash table defined in htable.h
 */

#include "test.h"
#include "htable.h"
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

static const size_t n = 100;
struct value { int x; int y; };	

/* 
 * 1. init & destroy:
 *     - no memory leaks (valgrind will catch this)
 *     - table and stash members are non-null after initialization.
 *     - table should be nulled out after operation.
 */
void test_init_destroy()
{
	HASH_TABLE(t);

	/* call init */
	htable_init(&t, 16);

	ASSERT_TRUE(t.table1 != NULL && t.table2 != NULL && t.stash != NULL,
		    "test_init_destroy: null member after initialisation.\n");
	ASSERT_TRUE(t.size != 0,
		    "test_init_destroy: size was zero after initialisation.\n");
	ASSERT_TRUE(t.entries == 0,
		    "test_init_destroy: entries was not zero despite no "
		    "insertions.\n");

	/* call destroy */
	htable_destroy(&t);

	ASSERT_TRUE(t.size == 0 && t.entries == 0 && t.seed1 == 0
		    && t.seed2 == 0 && t.table1 == NULL && t.table2 == NULL
		    && t.stash == NULL, "test_init_destroy: table was not "
		    "zeroed after calling destroy.\n");

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
	HASH_TABLE(t);
	htable_init(&t, 16);

	int x = rand();
	int y = x;
	
	ASSERT_TRUE(htable_insert(&t, x, NULL), "test_insert_same: insert "
		    "failed.\n");

	ASSERT_TRUE(t.entries == 1, "test_insert_same: entries was not "
		    "incrememnted.\n");
	ASSERT_TRUE(htable_exists(&t, x), "test_insert_same: exists does not "
		    "return true for inserted element.\n");

	ASSERT_TRUE(htable_insert(&t, y, NULL), "test_insert_same: insert "
		    "failed for duplicate key.\n");

	ASSERT_TRUE(t.entries == 1, "test_insert_same: entries was incrimented "
		    "after inserting same element.\n");
	
	htable_destroy(&t);
}

void test_insert_many()
{
	HASH_TABLE(t);
	htable_init(&t, 16);
	size_t initial_size = t.size;
	
	for (size_t i = 0; i < n; i++) {
		ASSERT_TRUE(htable_insert(&t, i, NULL), "test_insert_many: "
			    "insert failed.\n");
                ASSERT_TRUE(htable_exists(&t, i), "test_insert_many: exists "
                            "returns false imediately after inserting");
        }
        
	ASSERT_TRUE(t.entries == n, "test_insert_many: entries was not "
		"incremented properly.\n");
        printf("entries is %zu \n", t.entries);

	for (size_t i = 0; i < n; i++)
		ASSERT_TRUE(htable_exists(&t, i), "test_insert_many: elements "
			    "were clobered on insertion.\n");
	
	size_t new_size = t.size;

	ASSERT_TRUE(new_size > initial_size, "test_insert_many: table did not "
		    "resize propperly.\n");

	htable_destroy(&t);
}

/*
 * 3. exists:
 *     - After inserting stuff, exists should return true for that key.
 *     - Exists should return false for things we haven't inserted.
 */
void test_exists()
{
	HASH_TABLE(t);
	htable_init(&t, 512);

	uint64_t keys[2*n];
	struct value data[n];

	for (size_t i = 0; i < n; i++) {
		keys[i] = i;
		data[i].x = rand();
		data[i].y = rand();
		ASSERT_TRUE(htable_insert(&t, keys[i], &(data[i])),
			    "test_exists: insert failed.\n");
	}

	/* initialize the rest of the keys (for negative asserts) */
	for (size_t i = n; i < 2*n; i++)
		keys[i] = rand();

	for (size_t i = 0; i < n; i++) {
		ASSERT_TRUE(htable_exists(&t, keys[i]), "test_exists: exists "
			    "returned false for inserted key.\n");
		ASSERT_FALSE(htable_exists(&t, keys[i+n]), "test_exists: exists "
			     "returned true for key that was not inserted.\n");
	}

	htable_destroy(&t);
}

/*
 * 4. remove:
 *     - After inserting an element, we should be able to remove it.
 *     - Removing an elememt should decrease the element count of the table.
 *     - Removing an element that does not exist should not decrease the element
 *       count of the tabe.
 *     - The table should shrink itself after many removes.
 */
void test_remove()
{
	HASH_TABLE(t);
	htable_init(&t, 16);

	uint64_t keys[n];
	struct value data[n];

	for (size_t i = 0; i < n; i++) {
		keys[i] = i;
		data[i].x = rand();
		data[i].y = rand();
		ASSERT_TRUE(htable_insert(&t, keys[i], &(data[i])),
			    "test_remove: insert failed.\n");
	}
	ASSERT_TRUE(t.entries == n, "test_remove: entries was wrong after "
		    "inserting.\n");
	
	size_t size_after_inserts = t.size;
	
	/* initialize the rest of the keys (for negative asserts) */
	for (size_t i = n; i < 2*n; i++) {
		htable_remove(&t, i);
		ASSERT_TRUE(t.entries == n, "test_remove: entries was "
			    "decremented after removing key not in table.\n");
	}

	for (size_t i = 0; i < n; i++) {
		htable_remove(&t, i);
		ASSERT_TRUE(t.entries == n - (i+1), "test_remove: entries was "
			    "not decremented after removal.\n");
		ASSERT_FALSE(htable_exists(&t, i), "test_remove: exists "
			     "returns true for removed element.\n");
	}

	ASSERT_TRUE(t.size < size_after_inserts, "test_remove: table did not "
		    "shrink after many removes.\n");

	htable_destroy(&t);
}

/*
 * 5. get:
 *     - After inserting an element, we should be able to get it out.
 *     - calling get on a key that we have not inserted should not modify
 *       the 'out' paramater.
 */
void test_get()
{
	HASH_TABLE(t);
	htable_init(&t, 512);

	uint64_t keys[2*n];
	struct value data[n];

	for (size_t i = 0; i < n; i++) {
		keys[i] = i;
		data[i].x = rand();
		data[i].y = rand();
		ASSERT_TRUE(htable_insert(&t, keys[i], &(data[i])),
			    "test_get: insert failed.\n");
	}

	/* initialize the rest of the keys (for negative asserts) */
	for (size_t i = n; i < 2*n; i++)
		keys[i] = rand();

	for (size_t i = 0; i < n; i++) {
		void const *val;
		ASSERT_TRUE(htable_get(&t, keys[i], &val), "test_get: get "
			    "returned false for inserted key.\n");
		struct value *cast_value = (struct value *)val;
		ASSERT_TRUE(cast_value->x == data[i].x
			    && cast_value->y == data[i].y, "test_get: "
			    "value returned by get does not match inserted "
			    "value.\n");
		val = NULL;
		ASSERT_FALSE(htable_get(&t, keys[i+n], &val), "test_get: get "
			     "returned true for key that was not inserted.\n");
		ASSERT_TRUE(val == NULL, "test_get: get modifies out paramater "
			    "even though no value was found.\n");
	}

	htable_destroy(&t);
}


int main(void) 
{
	srand(time(NULL));
	REGISTER_TEST(test_init_destroy);
	REGISTER_TEST(test_insert_same);
	REGISTER_TEST(test_insert_many);
	REGISTER_TEST(test_exists);
	REGISTER_TEST(test_remove);
	REGISTER_TEST(test_get);
	return run_all_tests();
}


