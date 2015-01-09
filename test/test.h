/**
 * \file test.h
 *
 * \author Eric Mueller
 *
 * \brief Header file for a test framework.
 */

#include "list.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

/**** macros, constants, and types to keep track of tests ****/

/* used to keep track of each test case */
struct test_case {
	void (*test)();
	const char *name;
};

/* 256 \aprox big enough (this will definitely bite me in the ass at some
 * point...) */
#define __MAX_NUM_TESTS 256
struct test_case __g_tests[__MAX_NUM_TESTS];

/* number of functions we put into the above array */
size_t __g_num_tests = 0;
/* global number of failed tests */
size_t __g_cases_failed = 0;
size_t __g_cases_ran = 0;

/* give the name of a function, put it in the gloabal array */
#define REGISTER_TEST(test) \
	__g_tests[__g_num_tests++] = (struct test_case){&(test), #test}

/* where to write when tests fail */
#define OUT_FILE stderr


/**** generic testing functions ****/

void generic_assert(bool condition, const char *msg);

/* generic non-exiting asserts */
#define ASSERT_TRUE(condition, msg) generic_assert(condition, msg)
#define ASSERT_FALSE(condition, msg) generic_assert(!condition, msg)

/* run all the tests in the test_functions array. Returns the number of
 * failed tests
 */
size_t run_all_tests();
