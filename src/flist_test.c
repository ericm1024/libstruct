/* Eric Mueller
 * flist_test.c
 * December 2014
 *
 * Tests for flist.
 */

#include "flist.h"
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
	struct flist l;
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

/* assert equality of an array and test list */
static size_t assert_equal(test_t *control, struct flist_head *hd,
			   size_t size, const char *msg)
{
	/* test for size */
	assert(hd->length == size);
	
	/* test for correct data and ordering */
	struct flist *l = hd->first;
	for (size_t i = 0; i < size; i++, l = l->next) {
		if (ASSERT_EQUAL_TEST_T(
			    CONTAINER(l), &(control[i]), msg) != 0) {
			return 1;
		}
	}
	
	/* test for null termination */
	assert(l == NULL);
	return 0;
}

static size_t generic_assert(bool condition, const char *msg)
{
	if (condition != true) {
		fprintf(OUT_FILE, "%s", msg);
		return 1;
	}
	return 0;
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
        FLIST_HEAD(test_name);

 /* give the name of a function, put it in the gloabal array */
#define REGISTER_TEST(test) test_functions[num_tests++] = (struct test_case){&(test), #test}

/* generic non-exiting asserts */
#define ASSERT_TRUE(condition, msg) generic_assert(condition, msg)
#define ASSERT_FALSE(condition, msg) generic_assert(!condition, msg)


/***************************************
 *               TESTS                 *
 ***************************************/

/* insert after tests */
/* 1: construct a list in order using flist_insert_after */
size_t test_flist_insert_after_1()
{
	INIT_TEST_DATA(controll, test_list, data_length);
	
	struct flist *prev = test_list.first;

	for (size_t i = 0; i < data_length; i++) {
		test_t *next = COPY_TEST_T(&(controll[i]));
		flist_insert_after(&test_list, prev, &(next->l));
		prev = &(next->l);
	}

	size_t ret = assert_equal(controll, &test_list, data_length,
		    "test_flist_insert_after_1: got invalid list.\n");
	 
	 flist_for_each(&test_list, &free, offsetof(test_t, l));
	 return ret;
}

/* 2: construct a list in reverse order using flist_insert_after */
size_t test_flist_insert_after_many()
{
	INIT_TEST_DATA(controll, test_list, data_length);	

         /* construct the flist */	 
	for (size_t i = 0; i < data_length; i++ ) {
		test_t *next =
			COPY_TEST_T(&(controll[ data_length - (i + 1) ]));
		flist_insert_after(&test_list, NULL, &(next->l));
	}

	/* test for equality */
	size_t ret = assert_equal(controll, &test_list, data_length,
		      "test_flist_insert_after_2: got invalid list.\n");
	/* cleanup */
	flist_for_each(&test_list, &free, offsetof(test_t, l));
	return ret;
}

/* push front tests */
/* 1: push front 1 element */
size_t test_flist_push_front_1()
{
 	INIT_TEST_DATA(control, test_list, 1);

	/* push front 1 element */
	test_t *t = COPY_TEST_T(control);
	flist_push_front(&test_list, &(t->l));

	/* test for equality */
	size_t ret = assert_equal(control, &test_list, 1,
				  "test_flist_push_front_1: got invalid list.\n");
	/* cleanup */
	flist_for_each(&test_list, &free, offsetof(test_t, l));
	return ret;
}

/* 2: push front lots of elements */
size_t test_flist_push_front_many()
{
	INIT_TEST_DATA(control, test_list, data_length);

	for (size_t i = 0; i < data_length; i++) {
		test_t *copy = COPY_TEST_T(&(control[data_length - (i+1)]));
		flist_push_front(&test_list, &(copy->l));
	}
	size_t ret = assert_equal(control, &test_list, data_length,
				  "test_push_front_2: got invalid list.\n");
	flist_for_each(&test_list, &free, offsetof(test_t, l));
	return ret;
}
	
/* pop front tests */
/* 1: pop 1 elemenet */
size_t test_flist_pop_front_1()
{
 	INIT_TEST_DATA(control, test_list, 1);

	/* push front 1 element */
	test_t *t = COPY_TEST_T(control);
	flist_push_front(&test_list, &(t->l));

	/* pop one */
	struct flist *popped = flist_pop_front(&test_list);
	
	size_t ret = ASSERT_EQUAL_TEST_T(CONTAINER(popped), &(control[0]),
			    "test_pop_front_1: popped does not match pushed.\n");
	ret += ASSERT_TRUE(test_list.first == NULL,
			   "test_pop_front_1: test_list->first was not null after popping only element");
	ret += ASSERT_TRUE(test_list.length == 0,
			   "test_pop_front_1: test_list->length was not zero after popping only element");
	free(CONTAINER(popped));
	return ret > 0 ? 1 : 0;
}

/* 2: pop many elements */
size_t test_flist_pop_front_many()
{
	INIT_TEST_DATA(control, test_list, data_length);
	
	for (size_t i = 0; i < data_length; i++) {
		test_t *copy = COPY_TEST_T(&(control[data_length - (i+1)]));
		flist_push_front(&test_list, &(copy->l));
	}

	size_t ret = 0;
	for (size_t i = 0; i < data_length; i++) {
		struct flist *popped = flist_pop_front(&test_list);
		ret += ASSERT_EQUAL_TEST_T(CONTAINER(popped), &(control[i]),
			"test_pop_front_2: *popped was not equal to control[i]");
		free(CONTAINER(popped));
	}
	ret += ASSERT_TRUE(test_list.first == NULL,
			   "test_pop_front_2: test_list->first was not null after popping all elements\n");
	ret += ASSERT_TRUE(test_list.length == 0,
			   "test_pop_front_2: test_list->length was not zero after popping all elements\n");
	return ret > 0 ? 1 : 0;
}


/* splice test */
size_t test_flist_splice()
{
	INIT_TEST_DATA(control, test_list, data_length);
	INIT_TEST_DATA(control2, test_list2, data_length);

	/* generate the two test lists */
	for (size_t i = 0; i < data_length; i++) {
		test_t *copy = COPY_TEST_T(&(control[data_length - (i+1)]));
		flist_push_front(&test_list, &(copy->l));
	}
	for (size_t i = 0; i < data_length; i++) {
		test_t *copy = COPY_TEST_T(&(control2[data_length - (i+1)]));
		flist_push_front(&test_list2, &(copy->l));
	}	
	
	/* splice the control data by hand */
	typeof(control[0]) control3[data_length*2];
	for (size_t i = 0; i < data_length*2; i++) {
		if (i < data_length/2)
			control3[i] = control[i];
		else if (i < data_length/2 + data_length)
			control3[i] = control2[i - data_length/2];
		else
			control3[i] = control[i - data_length];
	}

	/* get the element to splice at and do the splice */
	struct flist *where = test_list.first;
	for (size_t i = 0; i < data_length/2 - 1; i++, where = where->next)
		;
	flist_splice(&test_list, where, &test_list2);

	/* check for correctness */
	size_t ret = assert_equal(control3, &test_list, data_length*2,
				  "test_splice: got invalid list.\n");
	ret += ASSERT_TRUE(test_list2.first == NULL,
			   "test_splice: test_list2.first was not null\n");
	ret += ASSERT_TRUE(test_list2.length == 0,
			   "test_splice: test_list2.length was not zero\n");

	/* clean up */
	flist_for_each(&test_list, &free, offsetof(test_t, l));
	return ret > 0 ? 1 : 0;
}

/* for each is tested in each test with &free, correctness confirmed my 
 * valgrind, so we don't write an actual test for it  */

/* for each range test */
size_t test_flist_for_each_range()
{
	INIT_TEST_DATA(control, test_list, data_length);

	/* initialize the test list */
	for (size_t i = 0; i < data_length; i++) {
		test_t *copy = COPY_TEST_T(&(control[data_length - (i+1)]));
		flist_push_front(&test_list, &(copy->l));
	}

	/* mutate the original data */
	for (size_t i = data_length/4; i < 3*(data_length/4); i++) {
		MUTATE_TEST_T(&(control[i]));
	}

	/* get pointers to the list elements at the start and end of the
	 * mutation range */
	struct flist *start = test_list.first;
	for (size_t i = 0; i < data_length/4; start = start->next, i++)
		;
	struct flist *end = start;
	for (size_t i = 0; i < 3*(data_length/4) - data_length/4;
	     end = end->next, i++)
		;

	/* mutate the test list. We're breaking the testing abastraction
	 * here by using mutate_point directly, but we can't really use
	 * the MUTATE_TEST_T macro because we can't pass it as a function
	 * pointer. so this is kind of ugly, but it gets the job done. */
	flist_for_each_range(&test_list,(void (*)(void *))(&mutate_point), // lol
			     offsetof(test_t, l), start, end);

	/* check for correctness */
	size_t ret = assert_equal(control, &test_list, data_length,
				  "test_for_each_range: got invalid list.\n");
	
	/* clean up (and use flist_for_each_range again) */
	flist_for_each_range(&test_list, &free, offsetof(test_t, l),
			     test_list.first, NULL);
	return ret;
}

// main
int main(int argc, char **argv)
{
	srand(time(NULL));
	REGISTER_TEST(test_flist_insert_after_1);	
	REGISTER_TEST(test_flist_insert_after_many);
	REGISTER_TEST(test_flist_push_front_1);
	REGISTER_TEST(test_flist_push_front_many);
	REGISTER_TEST(test_flist_pop_front_1);
	REGISTER_TEST(test_flist_pop_front_many);
	REGISTER_TEST(test_flist_splice);
	REGISTER_TEST(test_flist_for_each_range);
	return run_all_tests() == 0 ? 0 : 1;
}
