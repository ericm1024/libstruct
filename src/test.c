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
 * \file test.c
 *
 * \author Eric Mueller
 *
 * \brief Implementation file for a test framework.
 */

#include "test.h"

struct test_case __g_tests[__MAX_NUM_TESTS];
size_t __g_num_tests = 0;
size_t __g_cases_failed = 0;
size_t __g_cases_ran = 0;

/**** generic testing functions ****/

void __attribute__((noinline)) __failed_assert_debug_hook()
{
	/* don't want this omptimzed away */
	__asm__ volatile ("" ::: "memory");
}

void __generic_assert(bool condition, const char *msg)
{
	__g_cases_ran++;
	if (condition != true) {
		__failed_assert_debug_hook();
		fprintf(OUT_FILE, "%s", msg);
		__g_cases_failed++;
	}
}

/* run all the tests in the test_functions array. Returns the number of
 * failed tests
 */
size_t run_all_tests()
{	
	/* otherwise we overwrote the end of the array and god knows
	 * what we broke */
	assert(__g_num_tests < __MAX_NUM_TESTS);
	
	size_t num_failed = 0;
	for (size_t i = 0; i < __g_num_tests; i++) {
		struct test_case t = __g_tests[i];
		fprintf(OUT_FILE, "Running %s:\n", t.name);
		size_t failed_b4 = __g_cases_failed;
		t.test();
		if (__g_cases_failed - failed_b4 == 0)
			fprintf(OUT_FILE, "    Passed.\n");
		else {
			fprintf(OUT_FILE, "    Failed.\n");
			num_failed++;
		}
	}
	
	if (__g_cases_failed != 0)
		fprintf(OUT_FILE, "Finished. Failed %zu out of %zu assertions in"
			" %zu out of %zu tests.\n", __g_cases_failed,
			__g_cases_ran, num_failed, __g_num_tests);
	else
		fprintf(OUT_FILE, "Finished running %zu assertions from %zu tests.\n",
			__g_cases_ran, __g_num_tests);
	
	return num_failed > 0 ? 1 : 0;
}
