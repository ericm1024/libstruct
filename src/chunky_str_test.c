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
 * \file chunky_str_test.c
 *
 * \author Eric Mueller
 *
 * \brief Test suite for functions defined in chunky_str.h
 */

#include "chunky_str.h"
#include "test.h"
#include <stddef.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>

/*
 * What needs to be tested:
 *     1. Basic cursor functionality:
 *         a. We can get a cursor with cs_cursor_get and destroy it with
 *            cs_cursor_destroy.
 *         b. We can clone a cursor with cs_cursor_clone and the two cursors
 *            will be equal when checked with cs_cursor_equal.
 *         c. We can two cursors, move them around in different ways (ending at
 *            the same spot), and cs_cursor_equal will still say they're equal
 *         d. cs_cursor_equal will not tell us two cursors are equal that should
 *            not be (i.e. cursors to different strings, cursors to different
 *            characters within the same string, cursors to different characters
 *            within the same chunk, etc.
 * 
 *     2. Cursor iteration functionality:
 *         a. We can move a cursor to the beginning or end of a string with
 *            cs_cursor_begin and cs_cursor_end and propperly get the first
 *            and last characters in the string with cs_cursor_getchar.
 *         b. cs_cursor_next moves the cursor to the next character in the string
 *            and will not point to character locations within a partially full
 *            chunk that do not represent actual inserted characters.
 *         c. ditto for cs_cursor_prev, except obviously that it moves to the
 *            previous character in the string.
 *         d. cs_cursor_in_range will tell us exactly when a cursor is out of
 *            the valid range of a string.
 *         e. cs_cursor_getchar will give us the charater at the cursors current
 *            location as we go through the entire string.
 *
 *     3. Cursor insertion/deletion functionality:
 *         a. cs_insert insertes a new character without clobbering any old
 *            characters, increases the length of the string by one, and
 *            keeps the cursor pointed at the same character it was previously
 *            pointed at. If the string was empty, it puts the character at the
 *            beginning of the string and points the cursor at that character.
 *         b. cs_insert_clobber overwrites the character that the cursor points
 *            to without altering any other part of the string or moving the
 *            cursor.
 *         c. cs_erase removes the character under the cursor and moves the
 *            cursor one char to the left. If the cursor refers to the beginning
 *            of the string, the cursor does not move.
 *         d. cs_cursor_erase_get behaves the same way and also gets the
 *            characer that was erased.
 *         
 *     4. Chunky string functionality.
 *         a. cs_push_back puts a character onto the end of a string, even if
 *            the string is empty.
 *         b. cs_push_front puts a character onto the front of a string, even
 *            if the string is empty.
 *         c. cs_destroy frees all memory associated with a string.
 *         d. cs_clone creates a deep copy of a string, i.e. we can modify the
 *            new string and not affect the old string, and we can modify the
 *            old string and not affect the new string.
 *         e. cs_to_cstring creates a heap allocated string containing all
 *            chars in the string up to the first null byte.
 *         f. cs_write puts the string in a buffer, including any and all null
 *            bytes. The function should NOT write past the end of the buffer
 *         g. cs_for_each iterates over every character in the string.
 */

static unsigned long string_size = 0;

/* used in several tests as the number of times to move some cursor arround */
#define NMOVES (string_size * 2)

/*
======================   HELPERS   ========================
 */

char random_char()
{
	return random(); /* not sure if this works... */
}

char *get_test_string(unsigned long length)
{
	char *str = malloc(sizeof(char) * length);
	if (!str) {
		ASSERT_TRUE(0, "get_test_string: malloc barfed, exiting.\n");
		exit(1);
	}
	for (unsigned long i = 0; i < length; i++)
		str[i] = random_char();
	return str;
}

void init_cs(struct chunky_str *cs, char *cstr,
			  unsigned long length)
{
	for (unsigned long i = 0; i < length; i++)
		ASSERT_TRUE(cs_push_back(cs, cstr[i]), 
			    "init_cs: cs_push_back failed.\n");
}

void walk_cursor(cs_cursor_t cursor, long offset)
{
	if (offset == 0)
		return;
	else if (offset < 0)
		for (long i = 0; i > offset; i--)
			cs_cursor_prev(cursor);
	else
		for (long i = 0; i < offset; i++)
			cs_cursor_next(cursor);
}

void walk_cursor_wrap(cs_cursor_t cursor, unsigned long offset)
{
	for (unsigned long i = 0; i < offset; i++) {
		cs_cursor_next(cursor);
		if (!cs_cursor_in_range(cursor))
			cs_cursor_begin(cursor);
	}
}

void three_equal_cursors(cs_cursor_t a, cs_cursor_t b, cs_cursor_t c)
{
	cs_cursor_t cursors[3] = {a, b, c};

	/*
	 * This will test for transitive, symmetric, and reflexive properties.
	 */ 
	for (unsigned long i = 0; i < 3; i++) {
		ASSERT_TRUE(cs_cursor_in_range(cursors[i]),
		    "three_equal_cursors: cursor was out of range.\n");
		for (unsigned long j = 0; j < 3; j++)
			ASSERT_TRUE(cs_cursor_equal(cursors[i], cursors[j]),
				    "three_equal_cursors: got unequal cursors.\n");
	}
}

/*
======================   BASIC TESTS   ========================
 */

void test_cursor_get_destroy()
{
	CHUNKY_STRING(test);
	char *control = get_test_string(string_size);
	init_cs(&test, control, string_size);
	cs_cursor_t cursor = cs_cursor_get(&test);
	cs_cursor_destroy(cursor);
	free(control);
	/*
	 * valgrind will catch memory leaks, which is all we really care about
	 * here
	 */
}

void test_cursor_clone()
{
	CHUNKY_STRING(test);
	char *control = get_test_string(string_size);
	init_cs(&test, control, string_size);
	cs_cursor_t a = cs_cursor_get(&test);
	cs_cursor_t b = cs_cursor_clone(a);
	cs_cursor_t c = cs_cursor_clone(b);
	
	three_equal_cursors(a, b, c);

	cs_cursor_destroy(a);
	cs_cursor_destroy(b);
	cs_cursor_destroy(c);
	free(control);
}

void test_cursor_equal_move()
{

	CHUNKY_STRING(test);
	char *control = get_test_string(string_size);
	init_cs(&test, control, string_size);
	cs_cursor_t a = cs_cursor_get(&test);
	cs_cursor_t b = cs_cursor_clone(a);
	cs_cursor_t c = cs_cursor_clone(b);
	unsigned long offset = 0;

	three_equal_cursors(a, b, c);

	/*
	 * The idea is to move the cursors around semi-randomly (without going
	 * out of bounds) and make sure they are still equal at all steps.
	 */ 
	for (unsigned long i = 0; i < NMOVES; i++) {
		offset = rand() % string_size;
		walk_cursor_wrap(a, offset);
		walk_cursor_wrap(b, offset);
		walk_cursor_wrap(c, offset);
		three_equal_cursors(a, b, c);
	}

	cs_cursor_destroy(a);
	cs_cursor_destroy(b);
	cs_cursor_destroy(c);
	free(control);
}

void test_cursor_equal_falsepos_same()
{
	CHUNKY_STRING(test);
	char *control = get_test_string(string_size);
	init_cs(&test, control, string_size);
	cs_cursor_t a = cs_cursor_get(&test);
	cs_cursor_t b = cs_cursor_get(&test);
	unsigned long a_index = 0;
	unsigned long b_index = 0;
	unsigned long offset = 0;

	for (unsigned long i = 0; i < NMOVES; i++) {
		offset = rand() % string_size;
		a_index = (a_index + offset) % string_size;
		walk_cursor_wrap(a, offset);
		do {
			offset = rand() % string_size;
		} while ((b_index + offset) % string_size == a_index);
		b_index = (b_index + offset) % string_size;
		walk_cursor_wrap(b, offset);
		ASSERT_FALSE(cs_cursor_equal(a, b), 
			     "got equal cursors that should not be equal.\n");
	}

	cs_cursor_destroy(a);
	cs_cursor_destroy(b);
	free(control);
}

void test_cursor_equal_falsepos_diff()
{
	CHUNKY_STRING(test_a);
	CHUNKY_STRING(test_b);
	char *control_a = get_test_string(string_size);
	char *control_b = get_test_string(string_size);
	init_cs(&test_a, control_a, string_size);
	init_cs(&test_b, control_b, string_size);
	cs_cursor_t a = cs_cursor_get(&test_a);
	cs_cursor_t b = cs_cursor_get(&test_b);

	ASSERT_FALSE(cs_cursor_equal(a, b),
		     "got equal cursors that should not be equal.\n");

	for (unsigned long i = 0; i < NMOVES; i++) {
		walk_cursor_wrap(a, rand() % string_size);
		walk_cursor_wrap(b, rand() % string_size);
		ASSERT_FALSE(cs_cursor_equal(a, b),
		     "got equal cursors that should not be equal.\n");
	}
	cs_cursor_destroy(a);
	cs_cursor_destroy(b);
	free(control_a);
	free(control_b);
}

/*
======================   CURSOR ITERATION TESTS   ========================
 */

void test_cursor_begin_end()
{
	CHUNKY_STRING(test);
	char *control = get_test_string(string_size);
	init_cs(&test, control, string_size);
	cs_cursor_t begin = cs_cursor_get(&test);
	cs_cursor_t end = cs_cursor_get(&test);

	cs_cursor_begin(begin);
	cs_cursor_end(end);
 	ASSERT_TRUE(cs_cursor_getchar(begin) == control[0],
		    "begin cursor did not give first char in string.\n");
	ASSERT_TRUE(cs_cursor_getchar(end) == control[string_size - 1],
		    "end cursor did not give last char in string.\n");

	/* flip them */
	cs_cursor_begin(end);
	cs_cursor_end(begin);
	ASSERT_TRUE(cs_cursor_getchar(end) == control[0],
		    "new begin cursor did not give first char in string.\n");
	ASSERT_TRUE(cs_cursor_getchar(begin) == control[string_size - 1],
		    "new end cursor did not give last char in string.\n");

	/* shuffle them around a bit, then move them back */
	walk_cursor_wrap(begin, rand() % string_size);
	walk_cursor_wrap(end, rand() % string_size);
	cs_cursor_begin(begin);
	cs_cursor_end(end);
 	ASSERT_TRUE(cs_cursor_getchar(begin) == control[0],
		    "begin cursor did not give first char in string.\n");
	ASSERT_TRUE(cs_cursor_getchar(end) == control[string_size - 1],
		    "end cursor did not give last char in string.\n");
}

void test_cursor_next()
{
	CHUNKY_STRING(test);
	char *control = get_test_string(string_size);
	init_cs(&test, control, string_size);
	cs_cursor_t cursor = cs_cursor_get(&test);
	char c;

	for (unsigned long i = 0; i < string_size; i++) {
		ASSERT_TRUE(cs_cursor_getchar(cursor) == control[i],
			    "cs_cursor_getchar gave invalid characer.\n");
		c = cs_cursor_next(cursor);
		if (i+1 < string_size)
			ASSERT_TRUE(c == control[i+1],
				    "cursor_next gave invalid character.\n");
		else
			ASSERT_TRUE(c == '\0',
				    "cursor_next did not give null byte at end.\n");
	}
}

void test_cursor_prev()
{
	CHUNKY_STRING(test);
	char *control = get_test_string(string_size);
	init_cs(&test, control, string_size);
	cs_cursor_t cursor = cs_cursor_get(&test);
	char c;
	cs_cursor_end(cursor);

	for (long i = string_size - 1; i >= 0; i--) {
		ASSERT_TRUE(cs_cursor_getchar(cursor) == control[i],
			    "cs_cursor_getchar gave invalid character.\n");
		c = cs_cursor_prev(cursor);
		if (i > 0)
			ASSERT_TRUE(c == control[i-1],
				    "cursor_prev gave invalid character.\n");
		else
			ASSERT_TRUE(c == '\0',
				    "cursor_prev did not give null byte at end.\n");
	}
}

void test_cursor_next_prev()
{
	CHUNKY_STRING(test);
	char *control = get_test_string(string_size);
	init_cs(&test, control, string_size);
	cs_cursor_t cursor = cs_cursor_get(&test);
	long current = 0;
	long next = 0;

	/*
	 * The idea is to walk to a random location in the string on each
	 * iteration. We leverage walk_cursor to this end, which calls
	 * cs_cursor_next and cs_cursor_prev.
	 */ 
	for (unsigned long i = 0; i < NMOVES; i++) {
		next = rand() % string_size;
		walk_cursor(cursor, next - current);
		ASSERT_TRUE(cs_cursor_getchar(cursor) == control[next],
			    "cursor did not match control after walk.\n");		
		current = next;
	}
}

void test_cs_cursor_in_range()
{
	CHUNKY_STRING(test);
	char *control = get_test_string(string_size);
	init_cs(&test, control, string_size);
	cs_cursor_t cursor = cs_cursor_get(&test);
	
	cs_cursor_begin(cursor);
	cs_cursor_prev(cursor);
	ASSERT_FALSE(cs_cursor_in_range(cursor),
		     "prev of begin cursor was not out of range.\n");

	cs_cursor_end(cursor);
	cs_cursor_next(cursor);
	ASSERT_FALSE(cs_cursor_in_range(cursor),
		     "next of end cursor was not out of range.\n");

	cs_cursor_begin(cursor);
	for (unsigned long i = 0; i < string_size; i++) {
		ASSERT_TRUE(cs_cursor_in_range(cursor),
			    "cursor in middle of string was out of range.\n");
		cs_cursor_next(cursor);
	}
	ASSERT_FALSE(cs_cursor_in_range(cursor),
		     "cursor was in range after traversing entire string.\n");
}

/*
 * cs_cursor_getchar is tested by all of the previous tests, so it doesn't
 * really warrant its own test.
 */


/*
======================   CURSOR ITERATION/DELETION TESTS   ========================
 */

void test_cursor_insert()
{
	
}

void test_cursor_insert_clobber()
{

}

void test_cursor_erase()
{

}

void test_cursor_erase_get()
{

}

void test_cursor_insert_erase_mixed()
{

}


/*
======================   CHUNKY STRING TESTS   ========================
 */

void test_cs_push_back()
{

}

void test_cs_push_front()
{

}

void test_cs_destroy()
{

}

void test_cs_clone()
{

}

void test_cs_to_cstring()
{

}

void test_cs_write()
{

}

void test_cs_for_each()
{

}

/**** main ****/

int main(int argc, char **argv)
{
	bool passed = true;
	(void)argc;
	(void)argv;
	srand(time(NULL));
	REGISTER_TEST(test_cursor_get_destroy);
	REGISTER_TEST(test_cursor_clone);
	REGISTER_TEST(test_cursor_equal_move);
	REGISTER_TEST(test_cursor_equal_falsepos_same);	
	REGISTER_TEST(test_cursor_equal_falsepos_diff);
	REGISTER_TEST(test_cursor_begin_end);
	REGISTER_TEST(test_cursor_next);
	REGISTER_TEST(test_cursor_prev);
	REGISTER_TEST(test_cursor_next_prev);
	REGISTER_TEST(test_cs_cursor_in_range);
	REGISTER_TEST(test_cursor_insert);
	REGISTER_TEST(test_cursor_insert_clobber);
	REGISTER_TEST(test_cursor_erase);
	REGISTER_TEST(test_cursor_erase_get);
	REGISTER_TEST(test_cursor_insert_erase_mixed);
	REGISTER_TEST(test_cs_push_back);
	REGISTER_TEST(test_cs_push_front);
	REGISTER_TEST(test_cs_destroy);
	REGISTER_TEST(test_cs_clone);
	REGISTER_TEST(test_cs_to_cstring);
	REGISTER_TEST(test_cs_write);
	REGISTER_TEST(test_cs_for_each);

	/* some of the false positive tests depend on this being at least 2*/
	string_size = 2;
	if (!run_all_tests())
		passed = false;

	string_size = 100;
	if (!run_all_tests())
		passed = false;

	string_size = 10000;
	if (!run_all_tests())
		passed = false;

	return passed;
}
