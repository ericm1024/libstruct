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
#include <stddef.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

/**** global tesing types, variables, and macros ****/

/* used to keep track of each test case */
struct test_case {
	size_t (*test)();
	const char *name;
};

/* 64 \aprox big enough (this will definitely bite me in the ass at some
 * point...) */
#define MAX_NUM_TESTS 64
static struct test_case test_functions[MAX_NUM_TESTS];

/* number of functions we put into the above array */
static size_t num_tests = 0;

/* size of test lists */
static const size_t data_length = 100;

/* where to write when tests fail */
#define OUT_FILE stderr


/**** types and functions for testing with points ****/

#define TESTING_WITH_POINTS 1

struct point_t {
	int x;
	int y;
	int z;
	struct list l;
};

/* 0 on sucess, 1 on failure. takes test and control */
static int point_equal(struct point_t *t, struct point_t *c,
		       const char *msg)
{
	if (t->x == c->x && t->y == c->y && t->z == c->z) {
		return 0;
	} else {
		fprintf(OUT_FILE, "%s", msg);
		return 1;
	}
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


/**** generic testing types & macros ****/

#ifdef TESTING_WITH_POINTS

/* generic type to run tests on */
typedef struct point_t test_t;

/* generic assert-equal-or-print-an-error-message macro (size_t return value) */
#define ASSERT_EQUAL_TEST_T(test, control, msg) \
	point_equal( (struct point_t *)(test), (struct point_t *)(control), msg)

/* puts a random test_t value into t (no return value) */
#define RAND_TEST_T(t) rand_point((struct point_t *)(t))

/* copies the provided point. allocates memory */
#define COPY_TEST_T(src) (test_t *)copy_point((struct point_t *)(src))

#define CONTAINER(node) container_of(node, struct point_t, l)

#define MUTATE_TEST_T(data) mutate_point((struct point_t *)data)

#endif /* TESTING_WITH_POINTS */


/**** generic testing functions ****/

/* initialize an array of test_t's pointed to by control */
static void gen_test_data(test_t *control, size_t size)
{
	for (size_t i = 0; i < size; i++)
		RAND_TEST_T( &(control[i]) );
}

static size_t generic_assert(bool condition, const char *msg)
{
	if (condition != true) {
		fprintf(OUT_FILE, "%s", msg);
		return 1;
	}
	return 0;
}

/* assert equality of an array and test list */
static size_t assert_equal(test_t *control, struct list_head *hd,
			   size_t size, const char *msg)
{
	/* test for size */
	size_t ret = generic_assert(hd->length == size,
				    "assert_equal: bad size.\n");
	
	/* test for correct data and ordering */
	struct list *l = hd->first;
	ret += generic_assert(l->prev == NULL,
		       "assert_equal: first->prev was not null.\n");
	struct list *prev = NULL;
	for (size_t i = 0; i < size; i++, l = l->next) {
		ret += ASSERT_EQUAL_TEST_T(
			CONTAINER(l), &(control[i]), msg);
		ret += generic_assert(l->prev == prev,
				      "assert_eqal: bad list->prev ptr.\n");
		prev = l;
	}
	
	/* test for null termination */
	ret += generic_assert(l == NULL,
			      "assert_equal: last->next was not null.\n");
	return ret > 0 ? 1 : 0;
}


/* run all the tests in the test_functions array. Returns the number of
 * failed tests
 */
static size_t run_all_tests()
{
	/* otherwise we overwrote the end of the array and god knows
	 * what we broke */
	assert(num_tests < MAX_NUM_TESTS);
	size_t failed = 0;
	for (size_t i = 0; i < num_tests; i++) {
		struct test_case t = test_functions[i];
		fprintf(OUT_FILE, "Running %s:\n", t.name);
		size_t ret = t.test();
		if (ret == 0)
			fprintf(OUT_FILE, "    Passed.\n");
		else
			fprintf(OUT_FILE, "    Failed.\n");
		failed += ret;
	}
	
	if (failed != 0)
		fprintf(OUT_FILE, "Finished. Passed %zu out of %zu tests.\n",
			num_tests - failed, num_tests);
	else
		fprintf(OUT_FILE, "Finished. Passed all %zu tests!\n",
			num_tests);
	return failed;
}


 /**** more generic testing macros ****/

 /* VERY UNSAFE MACRO. DO NOT USE IN CONDITIONAL CODE */
#define INIT_TEST_DATA(controll_name, test_name, size)          \
	test_t controll_name[(size)];				\
	gen_test_data(controll_name, (size));			\
        LIST_HEAD(test_name);

 /* give the name of a function, put it in the gloabal array */
#define REGISTER_TEST(test) test_functions[num_tests++] = (struct test_case){&(test), #test}

/* generic non-exiting asserts */
#define ASSERT_TRUE(condition, msg) generic_assert(condition, msg)
#define ASSERT_FALSE(condition, msg) generic_assert(!condition, msg)


/***************************************
 *               TESTS                 *
 ***************************************/

// insert before
size_t test_list_insert_before_many()
{
	INIT_TEST_DATA(control, tlist, data_length);

	test_t *insertee = COPY_TEST_T(&control[data_length - 1]);
	list_insert_before(&tlist, tlist.first, &(insertee->l));
	for (size_t i = 0; i < data_length - 1; i++) {
		insertee = COPY_TEST_T(&(control[i]));
		list_insert_before(&tlist, tlist.last, &(insertee->l));
	}
	
	size_t ret = assert_equal(control, &tlist, data_length,
		"test_list_insert_before_many: got invalid list\n.");
	list_for_each(&tlist, &free, offsetof(test_t, l));
	return ret;
}

size_t test_list_insert_before_null()
{
	INIT_TEST_DATA(control, tlist, data_length);

	for (size_t i = 0; i < data_length; i++) {
		test_t *insertee = COPY_TEST_T(&(control[i]));
		list_insert_before(&tlist, NULL, &(insertee->l));
	}

	size_t ret = assert_equal(control, &tlist, data_length,
		"test_list_insert_before_null: got invalid list\n.");
	list_for_each(&tlist, &free, offsetof(test_t, l));
	return ret;
}

// insert after
size_t test_list_insert_after_many()
{
	INIT_TEST_DATA(control, tlist, data_length);

	test_t *insertee = COPY_TEST_T(&(control[0]));
	list_insert_after(&tlist, tlist.first, &(insertee->l));
	for (size_t i = 1; i < data_length; i++) {
		insertee = COPY_TEST_T(&(control[data_length - i]));
		list_insert_after(&tlist, tlist.first, &(insertee->l));
	}
	
	size_t ret = assert_equal(control, &tlist, data_length,
		"test_list_insert_after_many: got invalid list\n.");
	list_for_each_range(&tlist, &free, offsetof(test_t, l), tlist.first,
		NULL);
	return ret;
}		

size_t test_list_insert_after_null()
{
	INIT_TEST_DATA(control, tlist, data_length);
	
	for (size_t i = 1; i <= data_length; i++) {
		test_t *insertee = COPY_TEST_T(&(control[data_length - i]));
		list_insert_after(&tlist, NULL, &(insertee->l));
	}
	
	size_t ret = assert_equal(control, &tlist, data_length,
		"test_list_insert_after_null: got invalid list\n.");
	list_for_each_range(&tlist, &free, offsetof(test_t, l), tlist.first,
		NULL);
	return ret;	
}

// delete
size_t test_list_delete()
{
	INIT_TEST_DATA(control, tlist, data_length);

	for (size_t i = 0; i < data_length; i++) {
		test_t *insertee = COPY_TEST_T(&control[i]);
		list_push_back(&tlist, &(insertee->l));
	}

	size_t j = 0;
	size_t ret = 0;
	for (struct list *k = tlist.first; k; j++) {
		struct list *next = k->next;
		list_delete(&tlist, k);
		ret += ASSERT_EQUAL_TEST_T(CONTAINER(k), &(control[j]),
			  "test_list_delete: list created in wrong order.\n");
		/* the bulk of this test is really done by valgrind */
		free(CONTAINER(k));
		k = next;
	}
	
	return ret > 0 ? 1 : 0;
}

// push front
size_t test_list_push_front()
{
	INIT_TEST_DATA(control, tlist, data_length);

	for (int i = data_length - 1; i >= 0; i--) {
		test_t *insertee = COPY_TEST_T(&control[i]);
		list_push_front(&tlist, &(insertee->l));
	}

	size_t ret = assert_equal(control, &tlist, data_length,
				"test_list_push_front: got invalid list.\n");
	list_for_each(&tlist, &free, offsetof(test_t, l));
	return ret;
}

// push back
size_t test_list_push_back()
{
	INIT_TEST_DATA(control, tlist, data_length);

	for (size_t i = 0; i < data_length; i++) {
		test_t *insertee = COPY_TEST_T(&control[i]);
		list_push_back(&tlist, &(insertee->l));
	}

	size_t ret = assert_equal(control, &tlist, data_length,
				"test_list_push_back: got invalid list.\n");
	list_for_each(&tlist, &free, offsetof(test_t, l));
	return ret;
}

// pop front
size_t test_list_pop_front()
{
	INIT_TEST_DATA(control, tlist, data_length);

	for (size_t i = 0; i < data_length; i++) {
		test_t *insertee = COPY_TEST_T(&control[i]);
		list_push_back(&tlist, &(insertee->l));
	}

	size_t ret = 0;
	for (size_t i = 0; i < data_length; i++) {
		struct list *popped = list_pop_front(&tlist);
		ret += ASSERT_EQUAL_TEST_T(CONTAINER(popped), &(control[i]),
		     "test_list_pop_front: popped didn't match control[i]");
		free(CONTAINER(popped));
	}

	ret += ASSERT_TRUE(tlist.first == NULL,
	       "test_list_pop_front: empty list is not null terminated.\n");
	ret += ASSERT_TRUE(tlist.last == NULL,
	       "test_list_pop_front: empty list is not null terminated.\n");
	ret += ASSERT_TRUE(tlist.length == 0,
	       "test_list_pop_front: empty list has non-zero size.\n");
	return ret > 0 ? 1 : 0;
}

// pop back
size_t test_list_pop_back()
{
	INIT_TEST_DATA(control, tlist, data_length);

	for (size_t i = 0; i < data_length; i++) {
		test_t *insertee = COPY_TEST_T(&control[i]);
		list_push_back(&tlist, &(insertee->l));
	}

	size_t ret = 0;
	for (int i = data_length - 1; i >= 0; i--) {
		struct list *popped = list_pop_back(&tlist);
		ret += ASSERT_EQUAL_TEST_T(CONTAINER(popped), &(control[i]),
		     "test_list_pop_back: popped didn't match control[i]");
		free(CONTAINER(popped));
	}

	ret += ASSERT_TRUE(tlist.first == NULL,
	       "test_list_pop_back: empty list is not null terminated.\n");
	ret += ASSERT_TRUE(tlist.last == NULL,
	       "test_list_pop_back: empty list is not null terminated.\n");
	ret += ASSERT_TRUE(tlist.length == 0,
	       "test_list_pop_back: empty list has non-zero size.\n");
	return ret > 0 ? 1 : 0;
}

// splice
size_t test_list_splice_end()
{
	INIT_TEST_DATA(control, control_tlist, data_length);
	INIT_TEST_DATA(slice_of_control, slice_of_tlist, data_length/3);
	INIT_TEST_DATA(rest_of_control, rest_of_tlist, data_length - data_length/3);
	
	for (size_t i = 0; i < data_length; i++) {
		test_t *insertee = COPY_TEST_T(&control[i]);
		test_t *insertee_copy = COPY_TEST_T(&control[i]);
		list_push_back(&control_tlist, &(insertee_copy->l));
		if (i < data_length/3) {
			slice_of_control[i] = control[i];
			list_push_back(&slice_of_tlist, &(insertee->l));
		} else {
			rest_of_control[i - data_length/3] = control[i];
			list_push_back(&rest_of_tlist, &(insertee->l));
		}
	}

	list_splice(&slice_of_tlist, slice_of_tlist.last, &rest_of_tlist);

	/* sanity check (i.e. does push back work?) */
	size_t ret = assert_equal(control, &control_tlist, data_length,
				   "test_list_splice_end: invalid control list.\n");
	/* correctness checks */
	ret += assert_equal(control, &slice_of_tlist, data_length,
			    "test_list_splice_end: bad list after splicing.\n");
	ret += ASSERT_TRUE(rest_of_tlist.first == NULL,
			   "test_list_splice_end: splicee was not invalidated.\n");
	ret += ASSERT_TRUE(rest_of_tlist.last == NULL,
			   "test_list_splice_end: splicee was not invalidated.\n");	
	ret += ASSERT_TRUE(rest_of_tlist.length == 0,
			   "test_list_splice_end: splicee was not invalidated.\n");

	list_for_each(&slice_of_tlist, &free, offsetof(test_t, l));
	list_for_each_range(&control_tlist, &free, offsetof(test_t, l),
			    control_tlist.first, NULL);
	return ret > 0 ? 1 : 0;
}

size_t test_list_splice_middle()
{
	INIT_TEST_DATA(control, control_tlist, data_length);
	INIT_TEST_DATA(middle_of_control, middle_of_tlist, data_length/3);
	INIT_TEST_DATA(rest_of_control, rest_of_tlist, data_length - data_length/3);

	size_t splice_size = data_length/4;
	for (size_t i = 0; i < data_length; i++) {
		test_t *insertee = COPY_TEST_T(&control[i]);
		test_t *insertee_copy = COPY_TEST_T(&control[i]);
		list_push_back(&control_tlist, &(insertee_copy->l));

		/* push back the first data_length/3 elemenets onto rest_of_tlist */
		if (i < data_length/3) {
			rest_of_control[i] = control[i];
			list_push_back(&rest_of_tlist, &(insertee->l));
		}
		/* push back the next splice_size elements onto middle_of_tlist */
		else if (i < data_length/3 + splice_size){
			middle_of_control[i - data_length/3] = control[i];
			list_push_back(&middle_of_tlist, &(insertee->l));
		}
		/* push back the remaining elements onto rest_of_tlist */
		else {
			rest_of_control[i - splice_size] = control[i];
			list_push_back(&rest_of_tlist, &(insertee->l));
		}
	}

	/* grab the element in rest_of_tlist to splice after */
	struct list *where = rest_of_tlist.first;
	for (size_t i = 1; i < data_length/3; i++, where = where->next)
		;

	/* splice the list */
	list_splice(&rest_of_tlist, where, &middle_of_tlist);

	/* sanity check (i.e. does push back work?) */
	size_t ret = assert_equal(control, &control_tlist, data_length,
				   "test_list_splice_middle: invalid control list.\n");
	/* correctness checks */
	ret += assert_equal(control, &rest_of_tlist, data_length,
			    "test_list_splice_middle: bad list after splicing.\n");
	ret += ASSERT_TRUE(middle_of_tlist.first == NULL,
			   "test_list_splice_middle: splicee was not invalidated.\n");
	ret += ASSERT_TRUE(middle_of_tlist.last == NULL,
			   "test_list_splice_middle: splicee was not invalidated.\n");
	ret += ASSERT_TRUE(middle_of_tlist.length == 0,
			   "test_list_splice_midde: splicee was not invalidated.\n");

	list_for_each(&rest_of_tlist, &free, offsetof(test_t, l));
	list_for_each_range(&control_tlist, &free, offsetof(test_t, l),
			    control_tlist.first, NULL);
	return ret > 0 ? 1 : 0;	
}

size_t test_list_splice_none()
{
	INIT_TEST_DATA(control, tlist, data_length);

	for (size_t i = 0; i < data_length; i++) {
		test_t *insertee = COPY_TEST_T(&control[i]);
		list_push_back(&tlist, &(insertee->l));
	}

	LIST_HEAD(empty);
	list_splice(&tlist, tlist.first->next, &empty);
	size_t ret = assert_equal(control, &tlist, data_length,
			"test_list_splice_none: invalid control list.\n");

	list_for_each(&tlist, &free, offsetof(test_t, l));
	return ret > 0 ? 1 : 0;
}

// for each (no actual test, tested in other tests) by calling free

// for each range
size_t test_list_for_each_range()
{
	INIT_TEST_DATA(control, tlist, data_length);

	/* initialize the test list */
	for (size_t i = 0; i < data_length; i++) {
		test_t *copy = COPY_TEST_T(&control[i]);
		list_push_back(&tlist, &(copy->l));
	}

	/* mutate the original data */
	size_t mstart = data_length/4;
	size_t mend = 3*(data_length/4);

	for (size_t i = mstart; i < mend; i++) {
		MUTATE_TEST_T(&control[i]);
	}

	/* get pointers to the start and end of the mutation range */
	struct list *start = tlist.first;
	for (size_t i = 0; i < mstart; start = start->next, i++)
		;
	struct list *end = start;
	for (size_t i = 0; i < mend - mstart; end = end->next, i++)
		;

	/* mutate the test list. We have to break the testing abstrction here
	 * by calling mutate_point directly but theres not much we can do
	 * as we can't exactly pass a function pointer to a macro */
	list_for_each_range(&tlist, (void (*)(void *))(&mutate_point), // lmao
			    offsetof(test_t, l), start, end);

	/* check for correctness */
	size_t ret = assert_equal(control, &tlist, data_length,
				  "test_for_each_range: gor invalid list.\n");
	/* clean up */
	list_for_each_range(&tlist, &free, offsetof(test_t, l),
			    tlist.first, NULL);
	
	return ret;
}

// reverse
size_t test_list_reverse()
{
	INIT_TEST_DATA(control, tlist, data_length);
	typeof(control[0]) rcontrol[data_length];
	
	/* initialize the test list */
	for (size_t i = 0; i < data_length; i++) {
		test_t *copy = COPY_TEST_T(&control[i]);
		list_push_back(&tlist, &(copy->l));
	}

	/* initialize the reversed control */
	for (size_t i = 0; i < data_length; i++) {
		rcontrol[data_length - (i + 1)] = control[i];
	}

	/* sanity check */
	size_t ret = assert_equal(control, &tlist, data_length,
				  "test_list_reverse: failed to construct valid"
				  " test list. Perhaps push_back is broken?\n");

	/* first reverse and assertion */
	list_reverse(&tlist);
	ret += assert_equal(rcontrol, &tlist, data_length,
			    "test_list_reverse: reversed list does not match"
			    " reversed control.\n");
	/* reverse and assert again */
	list_reverse(&tlist);
	ret += assert_equal(control, &tlist, data_length,
			    "test_list_reverse: list does not match control"
			    " after reversing twice.\n");
	list_for_each(&tlist, &free, offsetof(test_t, l));
	return ret > 0 ? 1 : 0;
}

// main
int main(int argc, char **argv)
{
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
	return run_all_tests() == 0 ? 0 : 1;
}
