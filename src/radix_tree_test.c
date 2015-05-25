/*
 * Copyright 2014 Eric Mueller
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
 * \file radix_tree_test.c
 *
 * \author Eric Mueller
 *
 * \brief Test suite for functions defined in radix_tree.h
 */

#include "radix_tree.h"
#include "test.h"
#include "util.h"
#include <stdlib.h>

#define N 10000

struct test_struct {
	unsigned long key;
	uint64_t ts_val;
};

struct test_struct *get_test_struct(unsigned long key)
{
	struct test_struct *t = malloc(sizeof *t);
	if (!t) {
		ASSERT_TRUE(false, "malloc barfed in get_test_struct\n");
		exit(1);
	}
	t->ts_val = pcg64_random();
	t->key = key;
	return t;
}

void test_struct_dtor(void *victim, void *unused)
{
	(void)unused;
	free(victim);
}

int test_struct_cmp(const void *lhs, const void *rhs)
{
	const struct test_struct *ts_lhs = (const struct test_struct *)lhs;
	const struct test_struct *ts_rhs = (const struct test_struct *)rhs;

	return ts_lhs->key < ts_rhs->key ? -1
		: ts_lhs->key > ts_rhs->key ? 1
		: 0;
		
}

void init_test_tree_array(struct radix_head *head, unsigned long n,
			  bool contig, struct test_struct ***array)
{
	*array = malloc(sizeof(*array) * n);
	if (!*array) {
		ASSERT_TRUE(false, "malloc barfed in init_test_tree_array\n");
		exit(1);
	}
	
	for (unsigned long i = 0; i < n; i++) {
		unsigned long key = contig ? i : pcg64_random();
		struct test_struct *t = get_test_struct(key);
		(*array)[i] = t;
		t->ts_val = pcg64_random();

		ASSERT_TRUE(radix_insert(head, t->key, t),
			    "insert failed in init_test_tree_array\n");
	}

	/* sort the elements by key order */
	qsort(*array, n, sizeof (*array)[0], test_struct_cmp);
}

void init_test_tree(struct radix_head *head, unsigned long n, bool contig)
{
	for (unsigned long i = 0; i < n; i++) {
		struct test_struct *t = get_test_struct(i);
		t->ts_val = pcg64_random();
		if (contig)
			t->key = i;
		else
			t->key = pcg64_random();
		ASSERT_TRUE(radix_insert(head, t->key, t),
			    "insert failed in init_test_tree\n");
	}
}

void assert_tree_empty(struct radix_head *tree, const char *msg)
{
	ASSERT_TRUE(tree->root == NULL
		    && tree->nnodes == 0
		    && tree->nentries == 0,
		    msg);
}


/*** insert/delete/lookup tests ***/

/* insert/destroy */
void test_insert_one()
{
	RADIX_HEAD(test);	
	assert_tree_empty(&test, "tree not empty after declaration macro\n");
	
	init_test_tree(&test, 1, true);
	ASSERT_TRUE(test.root != NULL
		    && test.nnodes == 1
		    && test.nentries == 1,
		    "tree head was wrong after one insert\n");

	radix_destroy(&test, test_struct_dtor, NULL);
	assert_tree_empty(&test, "tree not empty after destruction\n");
}

void test_insert_many()
{
	RADIX_HEAD(test);

	/* true --> contiguous keys */
	init_test_tree(&test, N, true);
	ASSERT_TRUE(test.nentries == N,	/* all we can really test reliably */
		    "entries was wrong after contiguous insert\n");
	radix_destroy(&test, test_struct_dtor, NULL);
	assert_tree_empty(&test, "contig. tree not empty after destruction\n");

	/* false --> random keys */
	init_test_tree(&test, N, false);
	ASSERT_TRUE(test.nentries == N,
		    "entries was wrong after contiguous insert\n");
	radix_destroy(&test, test_struct_dtor, NULL);
	assert_tree_empty(&test, "random tree not empty after destruction\n");
}

/* insert/delete */
void test_delete_one()
{
	RADIX_HEAD(test);

	struct test_struct *t = get_test_struct(0);
	radix_insert(&test, t->key, t);

	const void *res;
	radix_delete(&test, t->key, &res);
	ASSERT_TRUE(t == (struct test_struct *)res,
		    "deleted value is incorrect\n");
	assert_tree_empty(&test, "tree not empty after delete\n");
	free(t);
}

void test_delete_many()
{
 	RADIX_HEAD(test);

	struct test_struct **array;
	bool tf[] = {true, false};

	for (unsigned int i = 0; i < sizeof(tf)/sizeof(tf[0]); i++) {
		/* contiguous keys */
		init_test_tree_array(&test, N, tf[i], &array);
		ASSERT_TRUE(test.nentries == N,
			    "entries was wrong after insert\n");
		
		for (unsigned long i = 0; i < N; i++) {
			struct test_struct *t = array[i];
			const void *res;
			radix_delete(&test, t->key, &res);
			ASSERT_TRUE(t == (struct test_struct *)res,
                                    "deleted value is incorrect\n");
			ASSERT_TRUE(test.nentries == N - (i + 1),
				    "entries was wrong\n");

			/* false positives */
			res = NULL;
			radix_delete(&test, t->key, &res);
			ASSERT_TRUE(res == NULL,
				    "delete removed something that wasn't there\n");
			ASSERT_TRUE(test.nentries == N - (i + 1),
				    "entries was decremented when nothing was removed\n");
			free(t);
		}
		assert_tree_empty(&test, "tree not empty after destruction\n");
		free(array);
	}
}

/* insert/lookup */
void test_lookup_one()
{
	RADIX_HEAD(test);

	struct test_struct *t = get_test_struct(0);
	radix_insert(&test, t->key, t);

	const void *res;
	ASSERT_TRUE(radix_lookup(&test, t->key, &res),
		    "lookup failed for inserted element\n");
	ASSERT_TRUE((struct test_struct *)res == t,
		    "lookup returned wrong element\n");

	ASSERT_TRUE(radix_lookup(&test, t->key, NULL),
		    "lookup failed with NULL result ptr\n");
	radix_destroy(&test, test_struct_dtor, NULL);
}

void test_lookup_many()
{
 	RADIX_HEAD(test);

	struct test_struct **array;
	bool tf[] = {true, false};

	for (unsigned int i = 0; i < sizeof(tf)/sizeof(tf[0]); i++) {
		init_test_tree_array(&test, N, tf[i], &array);
		ASSERT_TRUE(test.nentries == N,
			    "entries was wrong after insert\n");
		
		for (unsigned long i = 0; i < N; i++) {
			struct test_struct *t = array[i];
			const void *res;
			ASSERT_TRUE(radix_lookup(&test, t->key, &res),
				    "lookup failed for inserted element\n");
			ASSERT_TRUE(t == (struct test_struct *)res,
				    "lookup returned wrong value\n");

			radix_delete(&test, t->key, NULL);
			ASSERT_FALSE(radix_lookup(&test, t->key, NULL), 
				     "lookup returned true for deleted element\n");
			ASSERT_TRUE(test.nentries == N - (i + 1),
				    "entries was wrong\n");
			free(t);
		}
		assert_tree_empty(&test, "tree not empty after destruction\n");
		free(array);
	}
}


/*** cursor tests ***/

/* begin/end */
#define SMALL_TEST_SIZE (100)
#define NR_SMALL_RUNS (1000)
void test_cursor_begin_end()
{
 	RADIX_HEAD(test);

	struct test_struct **array;

	for (unsigned long nr_runs = 0; nr_runs < NR_SMALL_RUNS; nr_runs++) {
		/* false == test trees use random keys */
		init_test_tree_array(&test, SMALL_TEST_SIZE, false, &array);
		
		unsigned long min = array[0]->key;
		unsigned long max = array[0]->key;
		
		for (unsigned long j = 0; j < SMALL_TEST_SIZE; j++) {
			unsigned long key = array[j]->key;
			if (key < min)
				min = key;
			if (key > max)
				max = key;
		}

		radix_cursor_t cursor;
		radix_cursor_begin(&test, &cursor);
		ASSERT_TRUE(radix_cursor_key(&cursor) == min,
			    "radix_cursor_begin did not initialize "
			    "cursor to first key in tree\n");
		radix_cursor_end(&test, &cursor);
		ASSERT_TRUE(radix_cursor_key(&cursor) == max,
			    "radix_cursor_end did not initialize "
			    "cursor to last key in tree\n");

		radix_destroy(&test, test_struct_dtor, NULL);
		assert_tree_empty(&test, "tree not empty after destruction\n");
		free(array);
	}
}

/* next/prev */
void test_cursor_next_prev()
{

}

/* next/prev valid */
void test_cursor_next_prev_valid()
{

}

/* next/prev alloc */
void test_cursor_next_prev_alloc()
{

}

/* seek */
void test_cursor_seek()
{

}

/* cusor has entry */
void test_cursor_has_entry()
{

}

/* read/write */
void test_cursor_read_write()
{

}

/* read/write block */
void test_cursor_read_write_block()
{

}


int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	seed_rng();
	REGISTER_TEST(test_insert_one);
	REGISTER_TEST(test_insert_many);
	REGISTER_TEST(test_delete_one);
	REGISTER_TEST(test_delete_many);
	REGISTER_TEST(test_lookup_one);
	REGISTER_TEST(test_lookup_many);
	REGISTER_TEST(test_cursor_begin_end);
	REGISTER_TEST(test_cursor_next_prev);
	REGISTER_TEST(test_cursor_next_prev_valid);
	REGISTER_TEST(test_cursor_next_prev_alloc);
	REGISTER_TEST(test_cursor_seek);
	REGISTER_TEST(test_cursor_has_entry);
	REGISTER_TEST(test_cursor_read_write);
	REGISTER_TEST(test_cursor_read_write_block);
	return run_all_tests();
}
