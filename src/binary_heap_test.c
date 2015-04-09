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
 * \file binary_heap_test.c
 *
 * \author Eric Mueller
 *
 * \brief Tests for a binary heap.
 */

#include "binary_heap.h"
#include "test.h"
#include <stddef.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

/*
 * what needs to be tested:
 *     1. the BINARY_HEAP macro declares a default heap with sane values
 *     2. binary_heap_init allocates space for the requested number of
 *        elements. We can insert that many elements without the heap resizing
 *     3. binary_heap_destroy destroys a heap and frees all memory associated
 *        with it.
 *     4. binary_heap_grow grows a heap to the specified size. It should
 *        modify the capacity field and should copy over all values from
 *        the old heap.
 *     5. binary_heap_shrink will resize the heap to the new size
 *        that we give.
 *     6. binary_heap_pop removes the kv pair with minimum key, moves a new
 *        minimum key to the root of the heap, and decreases the end field
 *        of the heap structure. pop should shrink the heap if there is room
 *        to do so.
 *     7. binary_heap_insert puts a new kv pair into the heap and maintains
 *        the heap property. Resizes the heap if necessary, and always
 *        incements the number of elements in the heap.
 *     8. binary_heap_merge merges two heaps, maintaining the heap property.
 *        The new heap contains all the elements in the two other heaps.
 */

/**** global tesing types, variables, and macros ****/

/* size of test lists */
#define TEST_N 100000

struct test_kv {
	unsigned long key;
	char value[24];
} test_data[TEST_N], test_data_ordered[TEST_N];

bool is_valid_heap_node(struct binary_heap *heap, unsigned long index)
{
	if (index >= heap->end)
		return true;
	unsigned long rchild_index = index*2 + 2;
	unsigned long lchild_index = index*2 + 1;
	bool okay = true;
	
	/* must be less than rchild */
	if (rchild_index < heap->end &&
	    heap->heap[index].key >= heap->heap[rchild_index].key)
		okay = false;

	/* must be less than lchild */
	if (lchild_index < heap->end &&
	    heap->heap[index].key >= heap->heap[lchild_index].key)
		okay = false;

	return okay && is_valid_heap_node(heap, rchild_index)
		&& is_valid_heap_node(heap, lchild_index);
}

/* true if heap is a valid min heap */
bool is_valid_heap(struct binary_heap *heap)
{
	return heap->end <= heap->capacity && is_valid_heap_node(heap, 0);
}

void init_test_data()
{
	for (unsigned long i = 0; i < TEST_N; i++) {
		test_data[i].key = i;
		for (unsigned long j = 0; j < sizeof test_data[i].value; j++)
			test_data[i].value[j] = rand();
		test_data_ordered[i] = test_data[i];
	}
	/* permute the array */
	for (unsigned long i = TEST_N - 1; i > 0; i--) {
		unsigned long j = rand() % i;
		struct test_kv tmp = test_data[i];
		test_data[i] = test_data[j];
		test_data[j] = tmp;
	}
}


/* decl macro, init, destroy */
void test_init_destroy()
{
	BINARY_HEAP(test);
	ASSERT_TRUE(test.capacity == 0,"capacity was not initialized to 0\n");
	ASSERT_TRUE(test.end == 0, "end was not initialized to 0\n");
	ASSERT_TRUE(test.heap == NULL, "heap was not initialized to null.\n");
	
	ASSERT_TRUE(binary_heap_init(&test, TEST_N), "malloc barfed\n");
	
	ASSERT_TRUE(test.capacity == TEST_N, "capacity was wrong\n");
	ASSERT_TRUE(test.end == 0, "end was not 0 after init.\n");
	ASSERT_FALSE(test.heap == NULL,"heap was null after init returned\n");

	binary_heap_destroy(&test);

	ASSERT_TRUE(test.capacity == 0, "capacity was nz after destroy\n");
	ASSERT_TRUE(test.end == 0, "end was nz after destroy\n");
	ASSERT_TRUE(test.heap == NULL, "heap was non-null after destroy\n");
	/* valgrind will catch leaks */
}

/* grow */
void test_grow()
{
	BINARY_HEAP(test);
	init_test_data();
	ASSERT_TRUE(binary_heap_init(&test, TEST_N), "malloc barfed\n");
	for (unsigned long i = 0; i < TEST_N; i++)
		binary_heap_insert(&test, test_data[i].key,
				   &test_data[i].value);
	ASSERT_TRUE(test.end == TEST_N, "end was wrong after inserts\n");
	ASSERT_TRUE(is_valid_heap(&test), "heap was not valid after inserts\n");
	
	ASSERT_TRUE(binary_heap_grow(&test, TEST_N*2), "grow returned false\n");
	ASSERT_TRUE(is_valid_heap(&test), "heap was not valid after grow\n");
	
	ASSERT_TRUE(test.capacity == 2*TEST_N, "grow did not change capacity.\n");
	ASSERT_TRUE(test.end == TEST_N, "grow changed end\n");
	for (unsigned long i = 0; i < TEST_N; i++) {
		unsigned long key;
		const void *value;
		binary_heap_pop(&test, &key, &value);
		ASSERT_TRUE(key == i, "popped wrong key after grow\n");
	}

	ASSERT_TRUE(is_valid_heap(&test), "invalid heap after removing everything\n");
	ASSERT_TRUE(test.end == 0, "end was not zero after removing everything\n");
	binary_heap_destroy(&test);
}

/* shrink/clear */
void test_shrink_clear()
{
	BINARY_HEAP(test);
	init_test_data();
	ASSERT_TRUE(binary_heap_init(&test, TEST_N), "malloc barfed\n");
	for (unsigned long i = 0; i < TEST_N; i++)
		binary_heap_insert(&test, test_data[i].key,
				   &test_data[i].value);
	ASSERT_TRUE(test.end == TEST_N, "end was wrong after inserts\n");
	ASSERT_TRUE(is_valid_heap(&test), "heap was not valid after inserts\n");

	binary_heap_clear(&test);
	ASSERT_TRUE(test.end == 0, "clear did not clear\n");

	for (unsigned long i = 0; i < TEST_N/2; i++)
		binary_heap_insert(&test, test_data_ordered[i].key,
				   &test_data_ordered[i].value);
	
	binary_heap_shrink(&test, TEST_N/2);
	ASSERT_TRUE(test.capacity == TEST_N/2,
		    "shrink did not change capacity\n");
	ASSERT_TRUE(is_valid_heap(&test), "heap was not valid after shrink\n");
	
	for (unsigned long i = 0; i < TEST_N/2; i++) {
		unsigned long key;
		const void *value;
		binary_heap_pop(&test, &key, &value);
		ASSERT_TRUE(key == i, "popped value was wrong\n");
	}
	binary_heap_destroy(&test);
}

/* pop */
void test_pop()
{
	BINARY_HEAP(test);
	init_test_data();
	ASSERT_TRUE(binary_heap_init(&test, TEST_N), "malloc barfed\n");
	for (unsigned long i = 0; i < TEST_N; i++)
		binary_heap_insert(&test, test_data[i].key,
				   &test_data[i].value);
	ASSERT_TRUE(test.end == TEST_N, "end was wrong after inserts\n");
	ASSERT_TRUE(is_valid_heap(&test), "heap was not valid after inserts\n");

	for (unsigned long i = 0; i < TEST_N; i++) {
		unsigned long key;
		const void *value;
		binary_heap_pop(&test, &key, &value);
		ASSERT_TRUE((test.end + 1) * 2 > test.capacity,
			    "did not resize\n");
		ASSERT_TRUE(key == test_data_ordered[i].key,
			    "popped wrong key\n");
		for (unsigned long j = 0; j < sizeof test_data_ordered[i].value;
		     j++)
			ASSERT_TRUE(((char*)value)[j] == test_data_ordered[i]
				    .value[j], "pop returned wrong value\n");
		ASSERT_TRUE(is_valid_heap(&test),
			    "heap was invalid after pop\n");
		ASSERT_TRUE(test.end == (TEST_N - (i + 1)),
			    "end was wrong after pop\n");
	}
	binary_heap_destroy(&test);
}

/* insert */
void test_insert()
{
	BINARY_HEAP(test);
	init_test_data();
	ASSERT_TRUE(binary_heap_init(&test, TEST_N), "malloc barfed\n");
	for (unsigned long i = 0; i < TEST_N; i++) {
		bool should_resize = false;
		unsigned long old_cap = test.capacity;
		ASSERT_TRUE(test.end == i, "end was wrong\n");
		if (test.end == old_cap)
			should_resize = true;
		ASSERT_TRUE(binary_heap_insert(&test, test_data[i].key,
					       &test_data[i].value),
			    "insert returned false\n");
		if (should_resize)
			ASSERT_TRUE(test.capacity > old_cap,
				    "heap did not resize when full\n");
		ASSERT_TRUE(is_valid_heap(&test),
			    "heap was not valid after insert\n");
	}

	for (unsigned long i = 0; i < TEST_N; i++) {
		unsigned long key;
		const void *value;
		binary_heap_pop(&test, &key, &value);

		ASSERT_TRUE(key == test_data_ordered[i].key,
			    "popped wrong key\n");
		for (unsigned long j = 0; j < sizeof test_data_ordered[i].value;
		     j++)
			ASSERT_TRUE(((char*)value)[j] == test_data_ordered[i]
				    .value[j], "pop returned wrong value\n");
	}
	binary_heap_destroy(&test);
}


/* merge */
void test_merge()
{
	BINARY_HEAP(test_a);
	BINARY_HEAP(test_b);
	init_test_data();
	ASSERT_TRUE(binary_heap_init(&test_a, TEST_N/3), "malloc barfed\n");
	ASSERT_TRUE(binary_heap_init(&test_b, TEST_N - TEST_N/3),
		    "malloc barfed\n");
	
	for (unsigned long i = 0; i < TEST_N/3; i++)
		ASSERT_TRUE(binary_heap_insert(&test_a, test_data[i].key,
					       &test_data[i].value),
			    "insert failed\n");
	for (unsigned long i = TEST_N/3; i < TEST_N; i++)
		ASSERT_TRUE(binary_heap_insert(&test_b, test_data[i].key,
					       &test_data[i].value),
			    "insert failed\n");
	
	ASSERT_TRUE(binary_heap_merge(&test_a, &test_b),
		    "merge returned false\n");
	ASSERT_TRUE(test_b.end == 0 && test_b.capacity == 0
		    && test_b.heap == NULL, "victim heap was not emptied\n");
	ASSERT_TRUE(test_a.end == TEST_N,
		    "new heap does not have all elems (wrong end)\n");
	ASSERT_TRUE(is_valid_heap(&test_a), "new heap was not valid\n");
	
	for (unsigned long i = 0; i < TEST_N; i++) {
		unsigned long key;
		const void *value;
		binary_heap_pop(&test_a, &key, &value);

		ASSERT_TRUE(key == test_data_ordered[i].key,
			    "popped wrong key\n");
		for (unsigned long j = 0; j < sizeof test_data_ordered[i].value;
		     j++)
			ASSERT_TRUE(((char*)value)[j] == test_data_ordered[i]
				    .value[j], "pop returned wrong value\n");
	}
	binary_heap_destroy(&test_b);
}

/* main */
int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	srand(time(NULL));
	
	REGISTER_TEST(test_init_destroy);
	REGISTER_TEST(test_grow);
	REGISTER_TEST(test_shrink_clear);
	REGISTER_TEST(test_pop);
	REGISTER_TEST(test_insert);
	REGISTER_TEST(test_merge);
	
	return run_all_tests();
}
