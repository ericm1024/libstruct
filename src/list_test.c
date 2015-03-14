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
 * \file list_test.c
 *
 * \author Eric Mueller
 *
 * \brief Tests for list.
 */

#include "list.h"
#include "test.h"
#include <stddef.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

/**** global tesing types, variables, and macros ****/

/* size of test lists */
#define data_length 1000

struct point_t {
	int x;
	int y;
	int z;
	struct list l;
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
	*p = (struct point_t) {rand(), rand(), rand(),
			       (struct list){NULL, NULL}};
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

/* initialize an array of point_t's pointed to by control */
static void gen_test_data(struct point_t *control, size_t size)
{
	for (size_t i = 0; i < size; i++)
		rand_point(&control[i]);
}

/* assert equality of an array and test list */
static void assert_equal(struct point_t *control, struct list_head *hd,
			 size_t size, const char *msg)
{
	/* test for size */
	ASSERT_TRUE(hd->length == size, "assert_equal: bad size.\n");
	
	/* test for correct data and ordering */
	struct point_t *l = list_first(hd);
	ASSERT_TRUE(!list_prev(hd, l), "assert_equal: first->prev was not null.\n");

	struct point_t *prev = NULL;
	for (size_t i = 0; i < size; i++, l = list_next(hd,l)) {
		ASSERT_TRUE(point_equal(l, &control[i]), msg);
		ASSERT_TRUE(list_prev(hd, l) == prev,
			    "assert_equal: bad list->prev ptr.\n");
		prev = l;
	}
	
	/* test for null termination */
	ASSERT_TRUE(!l, "assert_equal: last->next was not null.\n");
}

 /**** more generic testing macros ****/

 /* VERY UNSAFE MACRO. DO NOT USE IN CONDITIONAL CODE */
#define INIT_TEST_DATA(controll_name, test_name, size)			\
	struct point_t controll_name[(size)];				\
	gen_test_data(controll_name, (size));				\
        LIST_HEAD(test_name, struct point_t, l);

/***************************************
 *               TESTS                 *
 ***************************************/

/* insert before tests */
void test_list_insert_before_many()
{
	INIT_TEST_DATA(control, tlist, data_length);

	struct point_t *insertee = copy_point(&control[data_length - 1]);
	list_insert_before(&tlist, list_first(&tlist), insertee);
	for (size_t i = 0; i < data_length - 1; i++) {
		insertee = copy_point(&control[i]);
		list_insert_before(&tlist, list_last(&tlist), insertee);
	}
	
	assert_equal(control, &tlist, data_length,
		"test_list_insert_before_many: got invalid list\n.");
	list_for_each(&tlist, &free);
}

void test_list_insert_before_null()
{
	INIT_TEST_DATA(control, tlist, data_length);

	for (size_t i = 0; i < data_length; i++) {
		struct point_t *insertee = copy_point(&control[i]);
		list_insert_before(&tlist, NULL, insertee);
	}

	assert_equal(control, &tlist, data_length,
		     "test_list_insert_before_null: got invalid list\n.");
	list_for_each(&tlist, &free);
}

/* insert after tests */
void test_list_insert_after_many()
{
	INIT_TEST_DATA(control, tlist, data_length);

	struct point_t *insertee = copy_point(&control[0]);
	list_insert_after(&tlist, list_first(&tlist), insertee);
	for (size_t i = 1; i < data_length; i++) {
		insertee = copy_point(&control[data_length - i]);
		list_insert_after(&tlist, list_first(&tlist), insertee);
	}
	
	assert_equal(control, &tlist, data_length,
		    "test_list_insert_after_many: got invalid list\n.");
	list_for_each_range(&tlist, &free, list_first(&tlist), NULL);
}		

void  test_list_insert_after_null()
{
	INIT_TEST_DATA(control, tlist, data_length);
	
	for (size_t i = 1; i <= data_length; i++) {
		struct point_t *insertee = copy_point(&control[data_length - i]);
		list_insert_after(&tlist, NULL, insertee);
	}
	
	assert_equal(control, &tlist, data_length,
		"test_list_insert_after_null: got invalid list\n.");
	list_for_each_range(&tlist, &free, list_first(&tlist), NULL);
}

/* delete test */
void test_list_delete()
{
	INIT_TEST_DATA(control, tlist, data_length);

	for (size_t i = 0; i < data_length; i++) {
		struct point_t *insertee = copy_point(&control[i]);
		list_push_back(&tlist, insertee);
	}

	size_t j = 0;
	for (struct point_t *k = list_first(&tlist); k; j++) {
		struct point_t *next = list_next(&tlist, k);
		list_delete(&tlist, k);
		ASSERT_TRUE(point_equal(k, &control[j]),
			  "test_list_delete: list created in wrong order.\n");
		/* the bulk of this test is really done by valgrind */
		free(k);
		k = next;
	}
}

/* push front test */
void test_list_push_front()
{
	INIT_TEST_DATA(control, tlist, data_length);

	for (int i = data_length - 1; i >= 0; i--) {
		struct point_t *insertee = copy_point(&control[i]);
		list_push_front(&tlist, insertee);
	}

	assert_equal(control, &tlist, data_length,
		     "test_list_push_front: got invalid list.\n");
	list_for_each(&tlist, &free);
}

/* push back test */
void  test_list_push_back()
{
	INIT_TEST_DATA(control, tlist, data_length);

	for (size_t i = 0; i < data_length; i++) {
		struct point_t *insertee = copy_point(&control[i]);
		list_push_back(&tlist, insertee);
	}

	assert_equal(control, &tlist, data_length,
		     "test_list_push_back: got invalid list.\n");
	list_for_each(&tlist, &free);
}

/* pop front test */
void test_list_pop_front()
{
	INIT_TEST_DATA(control, tlist, data_length);

	for (size_t i = 0; i < data_length; i++) {
		struct point_t *insertee = copy_point(&control[i]);
		list_push_back(&tlist, insertee);
	}

	for (size_t i = 0; i < data_length; i++) {
		struct point_t *popped = list_pop_front(&tlist);
		ASSERT_TRUE(point_equal(popped, &control[i]),
			    "test_list_pop_front: popped didn't match control[i]");
		free(popped);
	}

	ASSERT_TRUE(tlist.first == NULL,
	       "test_list_pop_front: empty list is not null terminated.\n");
	ASSERT_TRUE(tlist.last == NULL,
	       "test_list_pop_front: empty list is not null terminated.\n");
	ASSERT_TRUE(tlist.length == 0,
	       "test_list_pop_front: empty list has non-zero size.\n");
}

/* pop back test */
void test_list_pop_back()
{
	INIT_TEST_DATA(control, tlist, data_length);

	for (size_t i = 0; i < data_length; i++) {
		struct point_t *insertee = copy_point(&control[i]);
		list_push_back(&tlist, insertee);
	}

	for (int i = data_length - 1; i >= 0; i--) {
		struct point_t *popped = list_pop_back(&tlist);
		ASSERT_TRUE(point_equal(popped, &control[i]),
		     "test_list_pop_back: popped didn't match control[i]");
		free(popped);
	}

	ASSERT_TRUE(tlist.first == NULL,
	       "test_list_pop_back: empty list is not null terminated.\n");
	ASSERT_TRUE(tlist.last == NULL,
	       "test_list_pop_back: empty list is not null terminated.\n");
	ASSERT_TRUE(tlist.length == 0,
	       "test_list_pop_back: empty list has non-zero size.\n");
}

/* splice tests */
void test_list_splice_end()
{
	INIT_TEST_DATA(control, control_tlist, data_length);
	INIT_TEST_DATA(slice_of_control, slice_of_tlist, data_length/3);
	INIT_TEST_DATA(rest_of_control, rest_of_tlist, data_length - data_length/3);
	
	for (size_t i = 0; i < data_length; i++) {
		struct point_t *insertee = copy_point(&control[i]);
		struct point_t *insertee_copy = copy_point(&control[i]);
		list_push_back(&control_tlist, insertee_copy);
		if (i < data_length/3) {
			slice_of_control[i] = control[i];
			list_push_back(&slice_of_tlist, insertee);
		} else {
			rest_of_control[i - data_length/3] = control[i];
			list_push_back(&rest_of_tlist, insertee);
		}
	}

	list_splice(&slice_of_tlist, list_last(&slice_of_tlist), &rest_of_tlist);

	/* sanity check (i.e. does push back work?) */
	assert_equal(control, &control_tlist, data_length,
		     "test_list_splice_end: invalid control list.\n");

	/* correctness checks */
	assert_equal(control, &slice_of_tlist, data_length,
		     "test_list_splice_end: bad list after splicing.\n");
	ASSERT_TRUE(rest_of_tlist.first == NULL,
		    "test_list_splice_end: splicee was not invalidated.\n");
	ASSERT_TRUE(rest_of_tlist.last == NULL,
		    "test_list_splice_end: splicee was not invalidated.\n");	
	ASSERT_TRUE(rest_of_tlist.length == 0,
		    "test_list_splice_end: splicee was not invalidated.\n");

	list_for_each(&slice_of_tlist, &free);
	list_for_each_range(&control_tlist, &free, list_first(&control_tlist), NULL);
}

void test_list_splice_middle()
{
	INIT_TEST_DATA(control, control_tlist, data_length);
	INIT_TEST_DATA(middle_of_control, middle_of_tlist, data_length/3);
	INIT_TEST_DATA(rest_of_control, rest_of_tlist, data_length - data_length/3);

	size_t splice_size = data_length/4;
	for (size_t i = 0; i < data_length; i++) {
		struct point_t *insertee = copy_point(&control[i]);
		struct point_t *insertee_copy = copy_point(&control[i]);
		list_push_back(&control_tlist, insertee_copy);

		/* push back the first data_length/3 elemenets onto rest_of_tlist */
		if (i < data_length/3) {
			rest_of_control[i] = control[i];
			list_push_back(&rest_of_tlist, insertee);
		}
		/* push back the next splice_size elements onto middle_of_tlist */
		else if (i < data_length/3 + splice_size){
			middle_of_control[i - data_length/3] = control[i];
			list_push_back(&middle_of_tlist, insertee);
		}
		/* push back the remaining elements onto rest_of_tlist */
		else {
			rest_of_control[i - splice_size] = control[i];
			list_push_back(&rest_of_tlist, insertee);
		}
	}

	/* grab the element in rest_of_tlist to splice after */
	struct point_t *where = list_first(&rest_of_tlist);
	for (size_t i = 1; i < data_length/3; 
	     i++, where = list_next(&rest_of_tlist, where))
		;

	/* splice the list */
	list_splice(&rest_of_tlist, where, &middle_of_tlist);

	/* sanity check (i.e. does push back work?) */
	assert_equal(control, &control_tlist, data_length,
				   "test_list_splice_middle: invalid control list.\n");
	/* correctness checks */
	assert_equal(control, &rest_of_tlist, data_length,
		     "test_list_splice_middle: bad list after splicing.\n");
	ASSERT_TRUE(middle_of_tlist.first == NULL,
		    "test_list_splice_middle: splicee was not invalidated.\n");
	ASSERT_TRUE(middle_of_tlist.last == NULL,
		    "test_list_splice_middle: splicee was not invalidated.\n");
	ASSERT_TRUE(middle_of_tlist.length == 0,
		    "test_list_splice_midde: splicee was not invalidated.\n");

	list_for_each(&rest_of_tlist, &free);
	list_for_each_range(&control_tlist, &free, list_first(&control_tlist), NULL);
}

void test_list_splice_none()
{
	INIT_TEST_DATA(control, tlist, data_length);

	for (size_t i = 0; i < data_length; i++) {
		struct point_t *insertee = copy_point(&control[i]);
		list_push_back(&tlist, insertee);
	}

	LIST_HEAD(empty, struct point_t, l);
	list_splice(&tlist, list_next(&tlist, list_first(&tlist)), &empty);
	assert_equal(control, &tlist, data_length,
		     "test_list_splice_none: invalid control list.\n");

	list_for_each(&tlist, &free);
}

/* for each (no actual test, tested in other tests) by calling free */

/* for each range */
void test_list_for_each_range()
{
	INIT_TEST_DATA(control, tlist, data_length);

	/* initialize the test list */
	for (size_t i = 0; i < data_length; i++) {
		struct point_t *copy = copy_point(&control[i]);
		list_push_back(&tlist, copy);
	}

	/* mutate the original data */
	size_t mstart = data_length/4;
	size_t mend = 3*(data_length/4);

	for (size_t i = mstart; i < mend; i++) {
		mutate_point(&control[i]);
	}

	/* get pointers to the start and end of the mutation range */
	struct point_t *start = list_first(&tlist);
	for (size_t i = 0; i < mstart; start = list_next(&tlist, start), i++)
		;
	struct point_t *end = start;
	for (size_t i = 0; i < mend - mstart; end = list_next(&tlist, end), i++)
		;

	list_for_each_range(&tlist, (void (*)(void *))&mutate_point, start, end);

	/* check for correctness */
	assert_equal(control, &tlist, data_length,
		     "test_for_each_range: gor invalid list.\n");

	/* clean up */
	list_for_each_range(&tlist, &free, list_first(&tlist), NULL);
}

/* reverse */
void test_list_reverse()
{
	INIT_TEST_DATA(control, tlist, data_length);
	struct point_t rcontrol[data_length];
	
	/* initialize the test list */
	for (size_t i = 0; i < data_length; i++) {
		struct point_t *copy = copy_point(&control[i]);
		list_push_back(&tlist, copy);
	}

	/* initialize the reversed control */
	for (size_t i = 0; i < data_length; i++) {
		rcontrol[data_length - (i + 1)] = control[i];
	}

	/* sanity check */
	assert_equal(control, &tlist, data_length,
		     "test_list_reverse: failed to construct valid"
		     " test list. Perhaps push_back is broken?\n");

	/* first reverse and assertion */
	list_reverse(&tlist);
	assert_equal(rcontrol, &tlist, data_length,
		     "test_list_reverse: reversed list does not match"
		     " reversed control.\n");

	/* reverse and assert again */
	list_reverse(&tlist);
	assert_equal(control, &tlist, data_length,
		     "test_list_reverse: list does not match control"
		     " after reversing twice.\n");
	list_for_each(&tlist, &free);
}

/* main */
int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	srand(time(NULL));
	REGISTER_TEST(test_list_insert_before_many);
	REGISTER_TEST(test_list_insert_after_many);
	REGISTER_TEST(test_list_insert_before_null);
	REGISTER_TEST(test_list_insert_after_null);
	REGISTER_TEST(test_list_delete);
	REGISTER_TEST(test_list_push_front);
	REGISTER_TEST(test_list_push_back);
	REGISTER_TEST(test_list_pop_front);
	REGISTER_TEST(test_list_pop_back);
	REGISTER_TEST(test_list_splice_end);
	REGISTER_TEST(test_list_splice_middle);
	REGISTER_TEST(test_list_splice_none);
	REGISTER_TEST(test_list_for_each_range);
	REGISTER_TEST(test_list_reverse);
	return run_all_tests();
}
