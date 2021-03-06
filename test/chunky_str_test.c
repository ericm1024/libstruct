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
	ASSERT_TRUE(cs->nchars == length, "init_cs: cs->nchars was wrong.\n");
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
		if (!cs_cursor_in_range(cursor))
			cs_cursor_begin(cursor);
		cs_cursor_next(cursor);
	}
	if (!cs_cursor_in_range(cursor))
		cs_cursor_begin(cursor);
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

void cs_equal_control(struct chunky_str *cs, char *control, unsigned long len)
{
	cs_cursor_t cursor = cs_cursor_get(cs);
	unsigned long i;
	ASSERT_TRUE(cs->nchars == len,
		    "cs_equal_control: cs->nchars was wrong.\n");
	
	for (i = 0;  i < len; i++) {
		ASSERT_TRUE(cs_cursor_getchar(cursor) == control[i],
			    "cs_equal_control: got wrong char.\n");
		cs_cursor_next(cursor);
	}
	ASSERT_FALSE(cs_cursor_in_range(cursor),
		     "cs_equal_control: cursor still in range after traversing "
		     "entire string.\n");
	cs_cursor_destroy(cursor);
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
	cs_destroy(&test);
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
	cs_destroy(&test);	
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
	cs_destroy(&test);
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
	cs_destroy(&test);
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
	cs_destroy(&test_a);
	cs_destroy(&test_b);
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

	/* flip them */
	cs_cursor_begin(end);
	cs_cursor_end(begin);
	ASSERT_TRUE(cs_cursor_getchar(end) == control[0],
		    "new begin cursor did not give first char in string.\n");

	/* shuffle them around a bit, then move them back */
	walk_cursor_wrap(begin, rand() % string_size);
	walk_cursor_wrap(end, rand() % string_size);
	cs_cursor_begin(begin);
	cs_cursor_end(end);
 	ASSERT_TRUE(cs_cursor_getchar(begin) == control[0],
		    "begin cursor did not give first char in string.\n");

	cs_cursor_destroy(begin);
	cs_cursor_destroy(end);
	free(control);
	cs_destroy(&test);
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

	cs_cursor_destroy(cursor);
	free(control);
	cs_destroy(&test);
}

void test_cursor_prev()
{
	CHUNKY_STRING(test);
	char *control = get_test_string(string_size);
	init_cs(&test, control, string_size);
	cs_cursor_t cursor = cs_cursor_get(&test);
	cs_cursor_end(cursor);

	for (long i = string_size - 1; i >= 0; i--) {
		ASSERT_TRUE(cs_cursor_prev(cursor) == control[i],
			    "cs_cursor_prev give invalid character.\n");
		ASSERT_TRUE(cs_cursor_getchar(cursor) == control[i],
			    "cs_cursor_getchar gave invalid character.\n");
	}

	cs_cursor_destroy(cursor);
	free(control);
	cs_destroy(&test);
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

	cs_cursor_destroy(cursor);
	free(control);
	cs_destroy(&test);
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
	ASSERT_FALSE(cs_cursor_in_range(cursor),
		     "end cursor was not out of range.\n");

	cs_cursor_begin(cursor);
	for (unsigned long i = 0; i < string_size; i++) {
		ASSERT_TRUE(cs_cursor_in_range(cursor),
			    "cursor in middle of string was out of range.\n");
		cs_cursor_next(cursor);
	}
	ASSERT_FALSE(cs_cursor_in_range(cursor),
		     "cursor was in range after traversing entire string.\n");
	cs_cursor_destroy(cursor);
	free(control);
	cs_destroy(&test);
}

/*
 * cs_cursor_getchar is tested by all of the previous tests, so it doesn't
 * really warrant its own test.
 */


/*
======================   CURSOR ITERATION/DELETION TESTS   ========================
 */

void test_cursor_insert_begin()
{
	CHUNKY_STRING(test);
	char *control = get_test_string(string_size);
	cs_cursor_t cursor = cs_cursor_get(&test);
	
	for (long i = string_size - 1; i >=0; i--) {
		cs_cursor_begin(cursor);
		cs_cursor_insert(cursor, control[i]);
	}

	cs_equal_control(&test, control, string_size);
	cs_cursor_destroy(cursor);
	free(control);
	cs_destroy(&test);
}

void test_cursor_insert_end()
{
	CHUNKY_STRING(test);
	char *control = get_test_string(string_size);
	cs_cursor_t cursor = cs_cursor_get(&test);

	for (unsigned long i = 0; i < string_size; i++)
		cs_cursor_insert(cursor, control[i]);

	cs_equal_control(&test, control, string_size);
	cs_cursor_destroy(cursor);
	free(control);
	cs_destroy(&test);
}

void test_cursor_insert_middle()
{
	CHUNKY_STRING(test);
	char *control = get_test_string(string_size);
	cs_cursor_t cursor = cs_cursor_get(&test);
	unsigned long begin = 0;
	unsigned long end = string_size - 1;

	/*
	 * The idea is to insert characters in a zipper like fashion. First,
	 * last, second, second to last, etc. Yes, we need both break
	 * statements. We're inserting two characters on each iteration, so
	 * if the string has odd length, we'll need to stop in the middle of
	 * an iteration.
	 */
	for (;;) {
		cs_cursor_insert(cursor, control[begin]);
		if (begin >= end)
			break;
		cs_cursor_insert(cursor, control[end]);
		cs_cursor_prev(cursor);
		end--;
		begin++;
		if (begin > end)
			break;
	}

	cs_equal_control(&test, control, string_size);
	cs_cursor_destroy(cursor);
	free(control);
	cs_destroy(&test);
}

void test_cursor_insert_clobber()
{
	CHUNKY_STRING(test);
	char *control_a = get_test_string(string_size);
	char *control_b = get_test_string(string_size);
	init_cs(&test, control_a, string_size);
	cs_cursor_t cursor = cs_cursor_get(&test);

	for (unsigned long i = 0; i < string_size; i++) {
		cs_cursor_insert_clobber(cursor, control_b[i]);
		cs_cursor_next(cursor);
	}

	cs_equal_control(&test, control_b, string_size);
	cs_cursor_destroy(cursor);
	free(control_a);
	free(control_b);
	cs_destroy(&test);
}

void test_cursor_erase_begin()
{
	CHUNKY_STRING(test);
	char *control = get_test_string(string_size);
	init_cs(&test, control, string_size);
	cs_cursor_t cursor = cs_cursor_get(&test);

	for (unsigned long i = 0; i < string_size; i++)
		cs_cursor_erase(cursor);
	
	ASSERT_TRUE(test.nchars == 0, "chunky_str->nchars was not zero after "
		    "erasing entire string.\n");
	ASSERT_TRUE(test.str.first == NULL, "chunky_str.str.first was not "
		    "null after erasing entire string.\n");
	free(control);
	cs_cursor_destroy(cursor);
}

void test_cursor_erase_end()
{
	CHUNKY_STRING(test);
	char *control = get_test_string(string_size);
	init_cs(&test, control, string_size);
	cs_cursor_t cursor = cs_cursor_get(&test);
	cs_cursor_end(cursor);

	for (unsigned long i = 0; i < string_size; i++) {
		cs_cursor_prev(cursor);
		cs_cursor_erase(cursor);
	}
	
	free(control);
	cs_cursor_destroy(cursor);
	ASSERT_TRUE(test.nchars == 0, "chunky_str->nchars was not zero after "
		    "erasing entire string.\n");
	ASSERT_TRUE(test.str.first == NULL, "chunky_str.str.first was not "
		    "null after erasing entire string.\n");	
}

void test_cursor_erase_middle()
{
	CHUNKY_STRING(test);
	char *control = get_test_string(string_size);
	init_cs(&test, control, string_size);
	cs_cursor_t cursor = cs_cursor_get(&test);
	long length = string_size;
	
	walk_cursor(cursor, string_size/2);
	
	while (length > 0)
		for (cs_cursor_begin(cursor), walk_cursor_wrap(cursor, length/2);
		     cs_cursor_in_range(cursor);
		     length--)
			cs_cursor_erase(cursor);

	ASSERT_TRUE(length == 0, "not enough characters were erased.\n");
	ASSERT_TRUE(test.nchars == 0, "chunky_str->nchars was not zero after "
		    "erasing entire string.\n");
	ASSERT_TRUE(test.str.first == NULL, "chunky_str.str.first was not "
		    "null after erasing entire string.\n");
	ASSERT_FALSE(cs_cursor_in_range(cursor),
		     "cursor is still in range.\n");
	
	free(control);
	cs_cursor_destroy(cursor);
}

void test_cursor_erase_get()
{
	CHUNKY_STRING(test);
	char *control = get_test_string(string_size);
	init_cs(&test, control, string_size);
	cs_cursor_t cursor = cs_cursor_get(&test);
	unsigned long i;
	
	for (i = 0; i < string_size; i++)
		ASSERT_TRUE(cs_cursor_erase_get(cursor) == control[i],
			    "erase_get returned wrong character.\n");
	ASSERT_TRUE(test.nchars == 0, "chunky_str->nchars was not zero after "
		    "erasing entire string.\n");
	ASSERT_TRUE(test.str.first == NULL, "chunky_str.str.first was not "
		    "null after erasing entire string.\n");

	free(control);
	cs_cursor_destroy(cursor);
}

void test_cursor_insert_erase_mixed()
{
	CHUNKY_STRING(test);
	char *control = get_test_string(string_size);
	init_cs(&test, control, string_size);
	cs_cursor_t cursor = cs_cursor_get(&test);
	unsigned long i;
	
	/*
	 * The idea is to pummel insert and erase at random locations in
	 * the string in an effort to cause a segfault. Getting this to match
	 * a c-string would be obnoxious so we don't have a control.
	 */
	for (i = 0; i < NMOVES && test.nchars; i++) {
		if (rand() & 1) {
			walk_cursor_wrap(cursor, rand() % test.nchars);
			cs_cursor_insert(cursor, random_char());
		} else {
			walk_cursor_wrap(cursor, rand() % test.nchars);
			cs_cursor_erase(cursor);
		}
	}

	cs_cursor_begin(cursor);
	while (test.nchars)
		cs_cursor_erase(cursor);

	free(control);
	cs_cursor_destroy(cursor);

	ASSERT_TRUE(test.nchars == 0, "chunky_str->nchars was not zero after "
		    "erasing entire string.\n");
	ASSERT_TRUE(test.str.first == NULL, "chunky_str.str.first was not "
		    "null after erasing entire string.\n");
}


/*
======================   CHUNKY STRING TESTS   ========================
 */

void test_cs_push_back()
{
	CHUNKY_STRING(test);
	char *control = get_test_string(string_size);
	unsigned int i;
	cs_cursor_t cursor = cs_cursor_get(&test);
	
	for (i = 0; i < string_size; i++) {
		cs_push_back(&test, control[i]);
		cs_cursor_end(cursor);
		cs_cursor_prev(cursor);
		ASSERT_TRUE(cs_cursor_getchar(cursor) == control[i],
			    "end cursor does not match pushed back character.\n");
		ASSERT_TRUE(test.nchars == i+1,
			    "bad string.nchars.\n");
		
	}
	cs_equal_control(&test, control, string_size);
	free(control);
	cs_cursor_destroy(cursor);
	cs_destroy(&test);
}

void test_cs_push_front()
{
	CHUNKY_STRING(test);
	char *control = get_test_string(string_size);
	unsigned int i;
	cs_cursor_t cursor = cs_cursor_get(&test);
	
	for (i = string_size; i > 0; i--) {
		cs_push_front(&test, control[i-1]);
		cs_cursor_begin(cursor);
		ASSERT_TRUE(cs_cursor_getchar(cursor) == control[i-1],
			    "begin cursor does not match pushed front character.\n");
		ASSERT_TRUE(test.nchars == string_size - i + 1,
			    "bad string.nchars.\n");
		
	}
	cs_equal_control(&test, control, string_size);
	cs_destroy(&test);
	free(control);
	cs_cursor_destroy(cursor);
}

void test_cs_destroy()
{
	CHUNKY_STRING(test);
	char *control = get_test_string(string_size);
	init_cs(&test, control, string_size);
	cs_equal_control(&test, control, string_size);
	free(control);
	cs_destroy(&test);
}

void test_cs_clone()
{
	CHUNKY_STRING(test);
	CHUNKY_STRING(clone);
	char *control = get_test_string(string_size);
	char *control_b = get_test_string(string_size);
	init_cs(&test, control, string_size);
	cs_clone(&test, &clone);
	cs_cursor_t cursor = cs_cursor_get(&clone);
	
	cs_equal_control(&test, control, string_size);
	cs_equal_control(&clone, control, string_size);

	/*
	 * The idea is to modify the clone string, then make sure that
	 * the clone has changed properly and that the original has not
	 */ 
	for (unsigned long i = 0; i < string_size; i++) {
		cs_cursor_insert_clobber(cursor, control_b[i]);
		cs_cursor_next(cursor);
	}

	cs_equal_control(&test, control, string_size);
	cs_equal_control(&clone, control_b, string_size);
	
	free(control);
	free(control_b);
	cs_destroy(&test);
	cs_destroy(&clone);
	cs_cursor_destroy(cursor);
}

void test_cs_to_cstring()
{
	CHUNKY_STRING(test);
	char *control = malloc(sizeof(char)*string_size);
	ASSERT_TRUE(control, "malloc barfed.\n");
	char c;
	unsigned long length;

	for (unsigned long i = 0; i < string_size; i++) {
		while ((c = random_char()) == '\0')
			;
		control[i] = c;
	}
	
	init_cs(&test, control, string_size);
	char *result = cs_to_cstring(&test, &length);
	ASSERT_TRUE(length == string_size + 1,
		    "cs_to_cstring gave wrong length.\n");
	for (unsigned long i = 0; i < string_size; i++) {
		ASSERT_TRUE(result[i] == control[i],
			    "cs_to_cstring gave invalid string.\n");
	}
	ASSERT_TRUE(result[string_size] == '\0',
		    "result was not null terminated.\n");

	cs_destroy(&test);
	free(result);

	control[string_size/2] = '\0';
	init_cs(&test, control, string_size);
	result = cs_to_cstring(&test, &length);
	ASSERT_TRUE(length == string_size/2 + 1,
		    "cs_to_cstring with partial string gave wrong length.\n");
	for (unsigned long i = 0; i < string_size/2 + 1; i++) {
		ASSERT_TRUE(result[i] == control[i], "cs_to_cstring with "
			    "partial string gave invalid string.\n");
	}

	cs_destroy(&test);
	free(result);
	free(control);
}

void test_cs_write()
{
	CHUNKY_STRING(test);
	char *control = get_test_string(string_size);
	init_cs(&test, control, string_size);
	char *buf = malloc(sizeof(char)*(string_size + 1));
	ASSERT_TRUE(buf, "malloc barfed.\n");
	char cannary = random_char();

	/* only fill half the buffer */
	buf[string_size/2] = cannary;
	unsigned long nbytes = cs_write(&test, buf, string_size/2);
	ASSERT_TRUE(nbytes == sizeof(char)*(string_size/2), "cs_write returned "
		    "the wrong number of bytes for half-full buf.\n");
	ASSERT_TRUE(buf[string_size/2] == cannary,
		    "cs_write overwrote end of buffer.\n");
	for (unsigned long i = 0; i < string_size/2; i++)
		ASSERT_TRUE(buf[i] == control[i], "output buffer does not "
			    "match control for half-full buf.\n");

	/* fill the whole buffer */
	buf[string_size] = cannary;
	nbytes = cs_write(&test, buf, string_size);
	ASSERT_TRUE(nbytes == sizeof(char)*string_size,
		    "cs_write returned the wrong number of bytes.\n");
	ASSERT_TRUE(buf[string_size] == cannary,
		    "cs_write overwrote end of buffer.\n");
	for (unsigned long i = 0; i < string_size; i++)
		ASSERT_TRUE(buf[i] == control[i],
			    "output buffer does not match control.\n");

	free(buf);
	free(control);
	cs_destroy(&test);
}

/*
 * cs_for_each is tested implicitly as it is used in many of the
 * implementations of the previously tested functions (I know I should
 * probably still write a test but bite me.
 */

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
	REGISTER_TEST(test_cursor_insert_begin);
	REGISTER_TEST(test_cursor_insert_end);
	REGISTER_TEST(test_cursor_insert_middle);
	REGISTER_TEST(test_cursor_insert_clobber);
	REGISTER_TEST(test_cursor_erase_begin);
	REGISTER_TEST(test_cursor_erase_end);
	REGISTER_TEST(test_cursor_erase_middle);
	REGISTER_TEST(test_cursor_erase_get);
	REGISTER_TEST(test_cursor_insert_erase_mixed);
	REGISTER_TEST(test_cs_push_back);
	REGISTER_TEST(test_cs_push_front);
	REGISTER_TEST(test_cs_destroy);
	REGISTER_TEST(test_cs_clone);
	REGISTER_TEST(test_cs_to_cstring);
	REGISTER_TEST(test_cs_write);

	/* some of the false positive tests depend on this being at least 2*/
	string_size = 5;
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
