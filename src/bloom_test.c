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


/*
 * what needs to be tested:
 *    1. init and destroy cause no memory leaks.
 *    2. init creates a table with reasonable parameters.
 *    3. we can insert elements and query will return true.
 *    4. Given a moderately full table, query returns false with high
 *       probability for elements that do not exist in the table.
 */

#define TEST_FILTER_SIZE (1 << 18)

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
	bloom_init(&b);
	uint64_t test_data[TEST_FILTER_SIZE];
	for (size_t i = 0; i < TEST_FILTER_SIZE; i++) {
		test_data[i] = pcg64_random();
		bloom_insert(&b, test_data[i]);
	}
	
	for (size_t i = 0; i < TEST_FILTER_SIZE; i++)
		ASSERT_TRUE(bloom_query(&b,test_data[i]),
			    "query returned false for inserted element.\n");
	bloom_destroy(&b);
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
	ASSERT_TRUE(falsep < BLOOM_P_DEFAULT*1.1,
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


int main(void) 
{
	srand(time(NULL));
	REGISTER_TEST(test_init_destroy);
	REGISTER_TEST(test_insert);
	REGISTER_TEST(test_false_positive);
	REGISTER_TEST(test_empty_query);
	return run_all_tests();
}


