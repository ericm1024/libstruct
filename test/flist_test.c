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
/* Eric Mueller
 * flist_test.c
 * December 2014
 *
 * Tests for flist.
 */

#include "flist.h"
#include "test.h"
#include <stddef.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

/* size of test lists */
#define data_length 10000

struct point_t {
	int x;
	int y;
	int z;
	struct flist l;
};

static bool point_equal(struct point_t *t, struct point_t *c)
{
	return t->x == c->x 
		&& t->y == c->y
		&& t->z == c->z;
}

/* generate a random point */
static void rand_point(struct point_t *p)
{
	*p = (struct point_t) {rand(), rand(), rand(), (struct flist){NULL}};
}

/* copy the provided point into a new point (allocates memory) */
static struct point_t *copy_point(struct point_t *src)
{
	struct point_t *copy = (struct point_t *)malloc(sizeof(struct point_t));
	if (!copy) {
		fprintf(stderr, "copy_point: failed to allocate new point\n");
		return NULL;
	}
	*copy = *src;
	return copy;
}

static void mutate_point(struct point_t *p)
{
	p->x /= 2;
	p->y /= 3;
	p->z /= 4;
}


/* initialize an array of struct point's pointed to by control */
static void gen_test_data(struct point_t *control, size_t size)
{
	for (size_t i = 0; i < size; i++)
		rand_point(&control[i]);
}

/* assert equality of an array and test list */
static void assert_equal(struct point_t *control, struct flist_head *hd,
			 size_t size, const char *msg)
{
	/* test for size */
	ASSERT_TRUE(hd->length == size, msg);
	
	/* test for correct data and ordering */
	struct point_t *l = flist_first(hd);
	for (size_t i = 0; i < size; i++, l = flist_next(hd, l)) {
		ASSERT_TRUE(point_equal(l, &control[i]), msg);
	}
	
	/* test for null termination */
	ASSERT_TRUE(l == NULL, msg);
}

/**** more generic testing macros ****/

/* VERY UNSAFE MACRO. DO NOT USE IN CONDITIONAL CODE */
#define INIT_TEST_DATA(controll_name, test_name, size)			\
	struct point_t controll_name[(size)];				\
	gen_test_data(controll_name, (size));				\
        FLIST_HEAD(test_name, struct point_t, l);

/***************************************
 *               TESTS                 *
 ***************************************/

/* insert after tests */
/* 1: construct a list in order using flist_insert_after */
void test_flist_insert_after_1()
{
	INIT_TEST_DATA(controll, test_list, data_length);
	
	void *prev = flist_first(&test_list);

	for (size_t i = 0; i < data_length; i++) {
		struct point_t *next = copy_point(&(controll[i]));
		flist_insert_after(&test_list, prev, next);
		prev = next;
	}

	assert_equal(controll, &test_list, data_length,
		    "test_flist_insert_after_1: got invalid list.\n");
	 
	flist_for_each(&test_list, &free);
}

/* 2: construct a list in reverse order using flist_insert_after */
void test_flist_insert_after_many()
{
	INIT_TEST_DATA(controll, test_list, data_length);	

         /* construct the flist */	 
	for (size_t i = 0; i < data_length; i++ ) {
		struct point_t *next =
		      copy_point(&controll[data_length - (i+1)]);
		flist_insert_after(&test_list, NULL, next);
	}

	/* test for equality */
	assert_equal(controll, &test_list, data_length,
		      "test_flist_insert_after_2: got invalid list.\n");
	/* cleanup */
	flist_for_each(&test_list, &free);
}

/* push front tests */
/* 1: push front 1 element */
void test_flist_push_front_1()
{
 	INIT_TEST_DATA(control, test_list, 1);

	/* push front 1 element */
	struct point_t *t = copy_point(control);
	flist_push_front(&test_list, t);

	/* test for equality */
	assert_equal(control, &test_list, 1,
		     "test_flist_push_front_1: got invalid list.\n");
	/* cleanup */
	flist_for_each(&test_list, &free);
}

/* 2: push front lots of elements */
void test_flist_push_front_many()
{
	INIT_TEST_DATA(control, test_list, data_length);

	for (size_t i = 0; i < data_length; i++) {
		struct point_t *copy = copy_point(&control[data_length - (i+1)]);
		flist_push_front(&test_list, copy);
	}
	assert_equal(control, &test_list, data_length,
		     "test_push_front_2: got invalid list.\n");
	flist_for_each(&test_list, &free);
}
	
/* pop front tests */
/* 1: pop 1 elemenet */
void test_flist_pop_front_1()
{
 	INIT_TEST_DATA(control, test_list, 1);

	/* push front 1 element */
	struct point_t *t = copy_point(control);
	flist_push_front(&test_list, t);

	/* pop one */
	struct point_t *popped = flist_pop_front(&test_list);
	
	ASSERT_TRUE(point_equal(popped, &control[0]),
		    "test_pop_front_1: popped does not match pushed.\n");
	ASSERT_TRUE(test_list.first == NULL,
		    "test_pop_front_1: test_list->first was not null after popping only element");
	ASSERT_TRUE(test_list.length == 0,
		    "test_pop_front_1: test_list->length was not zero after popping only element");
	free(popped);
}

/* 2: pop many elements */
void test_flist_pop_front_many()
{
	struct point_t *tmp;
	INIT_TEST_DATA(control, test_list, data_length);
	
	for (size_t i = 0; i < data_length; i++) {
		tmp = copy_point(&control[data_length - (i+1)]);
		flist_push_front(&test_list, tmp);
	}

	for (size_t i = 0; i < data_length; i++) {
		tmp = flist_pop_front(&test_list);
		ASSERT_TRUE(point_equal(tmp, &control[i]),
			    "test_pop_front_2: *popped was not equal to control[i]");
		free(tmp);
	}
	ASSERT_TRUE(test_list.first == NULL,
		    "test_pop_front_2: test_list->first was not null after "
		    "popping all elements\n");
	ASSERT_TRUE(test_list.length == 0,
		    "test_pop_front_2: test_list->length was not zero after "
		    "popping all elements\n");
}

/* splice test */
void test_flist_splice()
{
	struct point_t *copy;
	INIT_TEST_DATA(control, test_list, data_length);
	INIT_TEST_DATA(control2, test_list2, data_length);

	/* generate the two test lists */
	for (size_t i = 0; i < data_length; i++) {
		copy = copy_point(&control[data_length - (i+1)]);
		flist_push_front(&test_list, copy);
	}
	for (size_t i = 0; i < data_length; i++) {
		copy = copy_point(&control2[data_length - (i+1)]);
		flist_push_front(&test_list2, copy);
	}	
	
	/* splice the control data by hand */
	struct point_t control3[data_length*2];
	for (size_t i = 0; i < data_length*2; i++) {
		if (i < data_length/2)
			control3[i] = control[i];
		else if (i < data_length/2 + data_length)
			control3[i] = control2[i - data_length/2];
		else
			control3[i] = control[i - data_length];
	}

	/* get the element to splice at and do the splice */
	struct point_t *where = flist_first(&test_list);
	for (size_t i = 0; i < data_length/2 - 1; i++, 
		     where = flist_next(&test_list, where))
		;
	flist_splice(&test_list, where, &test_list2);

	/* check for correctness */
	assert_equal(control3, &test_list, data_length*2,
		     "test_splice: got invalid list.\n");
	ASSERT_TRUE(test_list2.first == NULL,
		    "test_splice: test_list2.first was not null\n");
	ASSERT_TRUE(test_list2.length == 0,
		    "test_splice: test_list2.length was not zero\n");

	/* clean up */
	flist_for_each(&test_list, &free);
}

/* for each is tested in each test with &free, correctness confirmed my 
 * valgrind, so we don't write an actual test for it  */

/* for each range test */
void test_flist_for_each_range()
{
	INIT_TEST_DATA(control, test_list, data_length);

	/* initialize the test list */
	for (size_t i = 0; i < data_length; i++) {
		struct point_t *copy = copy_point(&control[data_length - (i+1)]);
		flist_push_front(&test_list, copy);
	}

	/* mutate the original data */
	for (size_t i = data_length/4; i < 3*(data_length/4); i++) {
		mutate_point(&control[i]);
	}

	/* get pointers to the list elements at the start and end of the
	 * mutation range */
	struct point_t *start = flist_first(&test_list);
	for (size_t i = 0; i < data_length/4; 
	     start = flist_next(&test_list, start), i++)
		;
	struct point_t *end = start;
	for (size_t i = 0; i < 3*(data_length/4) - data_length/4;
	     end = flist_next(&test_list, end), i++)
		;

	flist_for_each_range(&test_list, (void (*)(void *))&mutate_point,
			     start, end);

	/* check for correctness */
	assert_equal(control, &test_list, data_length,
		     "test_for_each_range: got invalid list.\n");
	
	/* clean up (and use flist_for_each_range again) */
	flist_for_each_range(&test_list, &free, flist_first(&test_list), NULL);
}

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	srand(time(NULL));
	REGISTER_TEST(test_flist_insert_after_1);	
	REGISTER_TEST(test_flist_insert_after_many);
	REGISTER_TEST(test_flist_push_front_1);
	REGISTER_TEST(test_flist_push_front_many);
	REGISTER_TEST(test_flist_pop_front_1);
	REGISTER_TEST(test_flist_pop_front_many);
	REGISTER_TEST(test_flist_splice);
	REGISTER_TEST(test_flist_for_each_range);
	return run_all_tests();
}
