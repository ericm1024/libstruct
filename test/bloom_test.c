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
 * \file bloom_test.c
 *
 * \author Eric Mueller
 *
 * \brief Tests for the bloom filter defined in bloom.h
 */

#include "test.h"
#include "bloom.h"
#include "pcg_variants.h"
#include <stdlib.h>
#include <time.h>
#include <math.h>

/*
 * what needs to be tested:
 *    1. init and destroy cause no memory leaks.
 *    2. init creates a table with reasonable parameters.
 *    3. we can insert elements and query will return true.
 *    4. Given a moderately full table, query returns false with high
 *       probability for elements that do not exist in the table.
 */

#define TEST_FILTER_SIZE (1 << 20)
#define FALSEP_SLACK 1.1

static void init_filter(struct bloom *filter, uint64_t **_keys,
			unsigned long size, const struct bloom *other)
{
	unsigned long i;
	uint64_t *keys;

	if (!other)
		ASSERT_TRUE(bloom_init(filter), "init_filter: bloom_init\n");
	else
		ASSERT_TRUE(bloom_init_from(filter, other),
			    "init_filter: bloom_inot_from\n");

	keys = malloc(sizeof *keys * size);
	ASSERT_TRUE(keys, "init_filter: keys\n");

	for (i = 0; i < size; i++) {
		keys[i] = pcg64_random();
		bloom_insert(filter, keys[i]);
	}

	*_keys = keys;
}

static double compute_falsep(const struct bloom *bf, unsigned long _n)
{
	double k = bf->nhash;
	double n = _n;
	double m = bf->nbits;

	/* https://en.wikipedia.org/wiki/Bloom_filter#Probability_of_false_positives */
	return pow(1.0 - pow(M_E, -(k*n)/m), k);
}

void test_init_destroy()
{
	BLOOM_FILTER(b, TEST_FILTER_SIZE, BLOOM_P_DEFAULT);
	ASSERT_TRUE(b.p == BLOOM_P_DEFAULT,
		    "BLOOM_FITLER macro did not initialize probability.\n");
	ASSERT_TRUE(b.n == TEST_FILTER_SIZE,
		    "BLOOM_FITLER macro did not initialize n.\n");
	ASSERT_TRUE(bloom_init(&b),
		    "bloom init returned failure return value.\n");
	ASSERT_TRUE(b.bits,
		    "bloom init did not allocate bits array.\n");
	ASSERT_TRUE(b.seeds,
		    "bloom init did not allocate seeds array.\n");
	bloom_destroy(&b);
	/* valgrind will catch memory leaks */
}

void test_insert()
{
	BLOOM_FILTER(b, TEST_FILTER_SIZE, BLOOM_P_DEFAULT);
	uint64_t *test_data;
	init_filter(&b, &test_data, TEST_FILTER_SIZE, NULL);

	for (size_t i = 0; i < TEST_FILTER_SIZE; i++) {
		test_data[i] = pcg64_random();
		bloom_insert(&b, test_data[i]);
	}
	
	for (size_t i = 0; i < TEST_FILTER_SIZE; i++)
		ASSERT_TRUE(bloom_query(&b,test_data[i]),
			    "query returned false for inserted element.\n");

	bloom_destroy(&b);
	free(test_data);
}

void test_false_positive()
{
	BLOOM_FILTER(b, TEST_FILTER_SIZE, BLOOM_P_DEFAULT);
	bloom_init(&b);
	for (size_t i = 0; i < TEST_FILTER_SIZE; i++)
		bloom_insert(&b, pcg64_random());

	size_t false_pos = 0;
	for (size_t i= 0; i < TEST_FILTER_SIZE; i++)
		if (bloom_query(&b, pcg64_random()))
			false_pos++;
	
        double falsep = ((double)false_pos)/((double)TEST_FILTER_SIZE);
	ASSERT_TRUE(falsep < BLOOM_P_DEFAULT*FALSEP_SLACK,
		    "got too many false positives\n");
	bloom_destroy(&b);
}

void test_empty_query()
{
	BLOOM_FILTER(b, TEST_FILTER_SIZE, BLOOM_P_DEFAULT);
	bloom_init(&b);
	for (size_t i = 0; i < TEST_FILTER_SIZE; i++)
		ASSERT_FALSE(bloom_query(&b, pcg64_random()),
			    "query returned true for empty filter\n");
	bloom_destroy(&b);
}

/**
 * \brief Helper function for the union/intersection tests. Ensure that
 * bloom querry returns @res for all of the elements in set1 and set2
 */
static unsigned long querry_count(struct bloom *b, uint64_t *set0,
				  uint64_t *set1, unsigned long size)
{
	unsigned long i = 0;
	unsigned long count = 0;

	for (i = 0; i < size; i++) {
		if (bloom_query(b, set0[i]))
			count++;
		if (bloom_query(b, set1[i]))
			count++;
	}

	return count;
}

void test_union()
{
	uint64_t *bf0_keys, *bf1_keys;
	unsigned long count;
	BLOOM_FILTER(into, TEST_FILTER_SIZE, BLOOM_P_DEFAULT);
	BLOOM_FILTER(bf0, TEST_FILTER_SIZE, BLOOM_P_DEFAULT);
	BLOOM_FILTER(bf1, TEST_FILTER_SIZE, BLOOM_P_DEFAULT);

	init_filter(&bf0, &bf0_keys, TEST_FILTER_SIZE, NULL);
	init_filter(&bf1, &bf1_keys, TEST_FILTER_SIZE, &bf0);
	ASSERT_TRUE(bloom_init_from(&into, &bf0), "init into\n");

	/* join the two filters, make sure union semantics are obeyed */
	ASSERT_TRUE(bloom_union(&into, &bf0, &bf1), "union\n");
	count = querry_count(&into, bf0_keys, bf1_keys, TEST_FILTER_SIZE);
	ASSERT_TRUE(count == TEST_FILTER_SIZE*2,
		    "union did not have all emenets\n");

	/* same should hold true if the filter is not initialized */
	bloom_destroy(&into);
	ASSERT_TRUE(bloom_union(&into, &bf0, &bf1), "union 2\n");
	count = querry_count(&into, bf0_keys, bf1_keys, TEST_FILTER_SIZE);
	ASSERT_TRUE(count == TEST_FILTER_SIZE*2,
		    "union did not have all emenets\n");

	/* join and clobber */
	ASSERT_TRUE(bloom_union(&bf0, &bf0, &bf1), "union 3\n");
	count = querry_count(&bf0, bf0_keys, bf1_keys, TEST_FILTER_SIZE);
	ASSERT_TRUE(count == TEST_FILTER_SIZE*2,
		    "union did not have all emenets\n");

	bloom_destroy(&into);
	bloom_destroy(&bf0);
	bloom_destroy(&bf1);
	free(bf0_keys);
	free(bf1_keys);
}

void test_intersection()
{
	uint64_t *bf0_keys, *bf1_keys;
	unsigned long i, count, same_count, false_count;
	BLOOM_FILTER(into, TEST_FILTER_SIZE, BLOOM_P_DEFAULT);
	BLOOM_FILTER(bf0, TEST_FILTER_SIZE, BLOOM_P_DEFAULT);
	BLOOM_FILTER(bf1, TEST_FILTER_SIZE, BLOOM_P_DEFAULT);

	ASSERT_TRUE(bloom_init(&bf0), "init bf0\n");
	ASSERT_TRUE(bloom_init_from(&bf1, &bf0), "init bf1\n");
	ASSERT_TRUE(bloom_init_from(&into, &bf0), "init into\n");

	bf0_keys = malloc(sizeof *bf0_keys * TEST_FILTER_SIZE);
	bf1_keys = malloc(sizeof *bf1_keys * TEST_FILTER_SIZE);
	ASSERT_TRUE(bf0_keys, "malloc bf0_keys\n");
	ASSERT_TRUE(bf1_keys, "malloc bf1_keys\n");

	/*
	 * make sure the two filters get at least a few equal keys, as otherwise
	 * the intersection would be empty.
	 */
	same_count = TEST_FILTER_SIZE/5;
	for (i = 0; i < same_count; i++) {
		uint64_t key = pcg64_random();
		bf0_keys[i] = key;
		bf1_keys[i] = key;
	}
	for (i = same_count; i < TEST_FILTER_SIZE; i++) {
		bf0_keys[i] = pcg64_random();
		bf1_keys[i] = pcg64_random();
	}
	for (i = 0; i < TEST_FILTER_SIZE; i++) {
		bloom_insert(&bf0, bf0_keys[i]);
		bloom_insert(&bf1, bf1_keys[i]);
	}

	/* join the two filters, make sure intersection semantics are obeyed */
	ASSERT_TRUE(bloom_intersection(&into, &bf0, &bf1), "intersection\n");
	count = querry_count(&into, bf0_keys, bf1_keys, TEST_FILTER_SIZE/5);

	/* x2 because double coutning */
	false_count = 2 * (TEST_FILTER_SIZE - same_count)
			* compute_falsep(&into, TEST_FILTER_SIZE)
			* FALSEP_SLACK;
	ASSERT_TRUE(count == 2*same_count,
		    "intersection did not have all elements\n");
	count = querry_count(&into, bf0_keys + same_count, bf1_keys + same_count,
			     TEST_FILTER_SIZE - same_count);
	ASSERT_TRUE(count < false_count,
		    "intersection had too many false positives\n");

	/* same should hold true if the filter is not initialized */
	bloom_destroy(&into);
	ASSERT_TRUE(bloom_intersection(&into, &bf0, &bf1), "intersection 2\n");
	count = querry_count(&into, bf0_keys, bf1_keys, TEST_FILTER_SIZE/5);
	ASSERT_TRUE(count == 2*(TEST_FILTER_SIZE/5),
		    "intersection did not have all elements\n");
	count = querry_count(&into, bf0_keys + TEST_FILTER_SIZE/5,
			     bf1_keys + TEST_FILTER_SIZE/5,
			     TEST_FILTER_SIZE - TEST_FILTER_SIZE/5);
	ASSERT_TRUE(count < false_count,
		    "intersection had too many false positives\n");

	/* join and clobber */
	ASSERT_TRUE(bloom_intersection(&bf0, &bf0, &bf1), "intersection 3\n");
	count = querry_count(&bf0, bf0_keys, bf1_keys, TEST_FILTER_SIZE/5);
	ASSERT_TRUE(count == 2*(TEST_FILTER_SIZE/5),
		    "intersection did not have all elements\n");
	count = querry_count(&bf0, bf0_keys + TEST_FILTER_SIZE/5,
			     bf1_keys + TEST_FILTER_SIZE/5,
			     TEST_FILTER_SIZE - TEST_FILTER_SIZE/5);
	ASSERT_TRUE(count < false_count,
		    "intersection had too many false positives\n");

	bloom_destroy(&into);
	bloom_destroy(&bf0);
	bloom_destroy(&bf1);
	free(bf0_keys);
	free(bf1_keys);
}

int main(void) 
{
	srand(time(NULL));
	REGISTER_TEST(test_init_destroy);
	REGISTER_TEST(test_insert);
	REGISTER_TEST(test_false_positive);
	REGISTER_TEST(test_empty_query);
	REGISTER_TEST(test_union);
	REGISTER_TEST(test_intersection);
	return run_all_tests();
}
