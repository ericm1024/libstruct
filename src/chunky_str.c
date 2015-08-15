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
 * \file chunky_str.c
 *
 * \author Eric Mueller
 *
 * \node Credit to the HMC CS department for introducing me to this structre,
 * though they do it in C++
 * 
 * \brief Implementation of a string as an unrolled doubly-linked list.
 *
 * \detail Each node ('chunk') in the list contains an array of characters,
 * and the list as a whole represents an entire string. This representation
 * allows for O(1) insertion and deletion, fast iteration, and relatively
 * minimal memory overhead.
 *
 * note: (having not started implementing this yet) I have a strong suspicion
 * that the biggest source of overhead in this data structure will be memory
 * management (malloc/free). If this ends up being the case, I may implement
 * a simple slab allocator to manage memory for list chunks. This would greatly
 * reduce the number of calls to malloc/free and would probably make the
 * structure quite snappy. But we'll see. For now, I need to write the damn
 * thing.
 */

#include "chunky_str.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define NULL_BYTE ('\0')

/* !!! not portable */
#define CACHELINE (64U)
/* wild guess of optimal size with no basis  */
#define CHUNKSIZE (CACHELINE*2)
/*
#define NCHARS								\
	((CHUNKSIZE - (sizeof(struct list) + sizeof(short)))/sizeof(char))
*/
#define NCHARS 8

struct cs_chunk {
	struct list link; /* chunk list. do NOT move this from offset 0 */
	unsigned short end; /* next free index in the chunk */
	char chars[NCHARS]; /* array of chars */
};

struct cs_cursor {
	struct chunky_str * const owner; /* string the cursor is in */
	struct cs_chunk *chunk; /* chunk the cursor is in */
	unsigned short index; /* index of the cursor into the chunk */
};



/* ========================================================================== */
/*                       helper functions & macros                            */
/* ========================================================================== */

#define CURSOR_DEREF(curs) (curs)->chunk->chars[(curs)->index]

static inline bool chunk_is_full(struct cs_chunk *chunk)
{
	return chunk->end == NCHARS;
}

static inline bool split_chunk(struct chunky_str *cs, struct cs_chunk *chunk)
{
	struct cs_chunk* new_chunk = malloc(sizeof (struct cs_chunk));
	unsigned long old;
	unsigned long new; /* indicies */

	assert(chunk_is_full(chunk));
	if (!new_chunk)
		return false;
	
	/*
	 * put the chunk in the list; give it half the chars
	 * from cursor->chunk
	 */
	list_insert_after(&cs->str, chunk, new_chunk);
	for (old = NCHARS/2, new = 0; old < NCHARS; old++, new++)
		new_chunk->chars[new] = chunk->chars[old];

	/* at this point 'new' is the number of chars we moved to new_chunk */
	chunk->end -= new;
	new_chunk->end = new;
	return true;
}

static bool split_chunk_cursor(struct cs_cursor *cursor)
{
	if (!split_chunk(cursor->owner, cursor->chunk))
		return false;

	if (cursor->index >= cursor->chunk->end) {
		cursor->index -= cursor->chunk->end;
		cursor->chunk = list_next(&cursor->owner->str, cursor->chunk);
	}
	return true;
}

/*
 * merge chunk prev with next. fees next and removes it from the list.
 * returns the number of chars that were added to prev.
 */ 
static unsigned long merge_two_chunks(struct chunky_str *cs,
				      struct cs_chunk *prev,
				      struct cs_chunk *next)
{
	unsigned long i;
	unsigned long j;
	
	for (i = prev->end, j = 0; j < next->end; i++, j++)
		prev->chars[i] = next->chars[j];

	list_delete(&cs->str, next);
	free(next);
	prev->end = i;
	return j;
}

#define SHIFT_FORWARD (1L)
#define SHIFT_REVERSE (-1L)

/*
 * Shift characters within a chunk to either clobber the character at index
 * start or make a space for a character at index start. Shift must be
 * SHIFT_FORWARD or SHIFT_REVERSE
 */ 
static void shift_chars(struct cs_chunk *chunk, unsigned long start, long shift)
{
	char *chars = chunk->chars;
	unsigned long end = chunk->end;

	assert((shift == SHIFT_FORWARD && end < NCHARS)
	       || shift == SHIFT_REVERSE);

	if (shift == SHIFT_FORWARD)
		/* make room for a character */
		for (; end > start; end--)
			chars[end] = chars[end - 1];
	else
		/* clobber a character */
		for (; start < end; start++)
			chars[start] = chars[start + 1];

	chunk->end += shift;
	assert(chunk->end <= NCHARS);
}



/* ========================================================================== */
/*                             cursor ops                                     */
/* ========================================================================== */

cs_cursor_t cs_cursor_get(struct chunky_str *cs)
{
	struct cs_cursor init = {.owner = cs, .chunk = 0, .index = 0};
	struct cs_cursor* cursor = malloc(sizeof(struct cs_cursor));
	if (cursor) {
		memcpy(cursor, &init, sizeof init);
		cs_cursor_begin(cursor);
	}
	return cursor;
}

cs_cursor_t cs_cursor_clone(cs_cursor_t jango)
{
        /*
	 * if you don't like the variable naming in this function,
	 * go watch star wars episode 2. If you still don't like the
	 * variable naming, then I don't care.
	 */
	struct cs_cursor* boba = malloc(sizeof(struct cs_cursor));
	if (boba)
		memcpy(boba, jango, sizeof *jango);
	return boba;
}

bool cs_cursor_equal(cs_cursor_t lhs, cs_cursor_t rhs)
{
	return lhs->owner == rhs->owner 
		&& lhs->chunk == rhs->chunk
		&& lhs->index == rhs->index;
}

void cs_cursor_begin(cs_cursor_t cursor)
{
	cursor->chunk = list_first(&cursor->owner->str);
	cursor->index = 0;
}

void cs_cursor_end(cs_cursor_t cursor)
{
	cursor->chunk = NULL;
	cursor->index = 0;
}

bool cs_cursor_in_range(cs_cursor_t cursor)
{
	return cursor->chunk;
}

void cs_cursor_destroy(cs_cursor_t cursor)
{
	free(cursor);
}

char cs_cursor_next(cs_cursor_t cursor)
{
	cursor->index++;
	if (cursor->index >= cursor->chunk->end) {
		cursor->chunk = list_next(&cursor->owner->str, cursor->chunk);
		cursor->index = 0;
	}
	return cursor->chunk ? CURSOR_DEREF(cursor) : NULL_BYTE;
}

char cs_cursor_prev(cs_cursor_t cursor)
{
	cursor->index--;
	if (!cursor->chunk) {
		cursor->chunk = list_last(&cursor->owner->str);
		cursor->index = cursor->chunk->end - 1;
	} else if (cursor->index >= cursor->chunk->end) {
		cursor->chunk = list_prev(&cursor->owner->str, cursor->chunk);
		if (cursor->chunk)
			cursor->index = cursor->chunk->end - 1;
	}
	return cursor->chunk ? CURSOR_DEREF(cursor) : NULL_BYTE;
}

char cs_cursor_getchar(cs_cursor_t cursor)
{
	return CURSOR_DEREF(cursor);
}

bool cs_cursor_insert(cs_cursor_t cursor, char c)
{
	/* end cursor. also accounts for empty string */
	if (!cursor->chunk) {
		if (!cs_push_back(cursor->owner, c))
			return false;
		cs_cursor_end(cursor);
		return true;
	} else if (chunk_is_full(cursor->chunk) && !split_chunk_cursor(cursor))
		return false;
	
	shift_chars(cursor->chunk, cursor->index, SHIFT_FORWARD);
	CURSOR_DEREF(cursor) = c;
	cursor->owner->nchars++;
	cs_cursor_next(cursor);
	return true;
}

bool cs_cursor_insert_clobber(cs_cursor_t cursor, char c)
{
	CURSOR_DEREF(cursor) = c;
	return true;
}

void cs_cursor_erase(cs_cursor_t cursor)
{
	struct cs_chunk *chunk = cursor->chunk;
	struct cs_chunk *prev;
	struct cs_chunk *next;
	
	/* clobber the character */
	shift_chars(chunk, cursor->index, SHIFT_REVERSE);
	cursor->owner->nchars--;

	/* we emptied the string */
	if (cursor->owner->nchars == 0) {
		list_delete(&cursor->owner->str, chunk);
		free(chunk);
		cursor->index = 0;
		cursor->chunk = NULL;
		return;
	}
	
	prev = list_prev(&cursor->owner->str, chunk);
	next = list_next(&cursor->owner->str, chunk);

	/* merge chunks, if we can */
	if (prev && prev->end + chunk->end <= NCHARS) {
		cursor->index += merge_two_chunks(cursor->owner, prev, chunk);
		cursor->chunk = prev;
	} else if (next && next->end + chunk->end <= NCHARS) {
		merge_two_chunks(cursor->owner, chunk, next);
	}
	
	/* fix up the cursor */
	if (cursor->index >= cursor->chunk->end) {
		cursor->chunk = list_next(&cursor->owner->str, cursor->chunk);
		cursor->index= 0;
	}
}

char cs_cursor_erase_get(cs_cursor_t cursor)
{
	char c = CURSOR_DEREF(cursor);
	cs_cursor_erase(cursor);
	return c;
}



/* ========================================================================== */
/*                          chunky string ops                                 */
/* ========================================================================== */

bool cs_push_back(struct chunky_str *cs, char c)
{
	struct cs_chunk *last = list_last(&cs->str);

	/* empty string */
	if (!last) {
		last = malloc(sizeof(struct cs_chunk));
		if (!last)
			return false;
		last->end = 0;
		list_push_back(&cs->str, last);
	} else if (chunk_is_full(last)) {
		if (!split_chunk(cs, last))
			return false;
		/* last is no longer last, so grab the actual last chunk */
		last = list_last(&cs->str);
	}
	
	last->chars[last->end] = c;
	last->end++;
	cs->nchars++;
	return true;
}

bool cs_push_front(struct chunky_str *cs, char c)
{
	struct cs_chunk *first = list_first(&cs->str);

	/* empty string */
	if (!first) {
		first = malloc(sizeof(struct cs_chunk));
		if (!first)
			return false;
		first->end = 0;
		list_push_front(&cs->str, first);
	} else if (chunk_is_full(first) && !split_chunk(cs, first))
		return false;

	shift_chars(first, 0, SHIFT_FORWARD);
	first->chars[0] = c;
	cs->nchars++;
	return true;
}

void cs_destroy(struct chunky_str *cs)
{
	list_for_each(&cs->str, struct cs_chunk, i)
		free(i);
	cs->nchars = 0;
	cs->str.first = NULL;
	cs->str.last = NULL;
	cs->str.length = 0;
}

bool cs_clone(struct chunky_str *cs, struct chunky_str *clone)
{
	struct cs_chunk *chunk;

	/*
	 * *sign* this is gross, but the offset field of the list head in
	 * struct chunky_string is const, so we can't just assign the
	 * pointer. Also fun fact: clang doen't catch this error. :/
	 */ 
	struct chunky_str init = CHUNKY_STRING_DEFAULT;
	memcpy(clone, &init, sizeof init); 
	
	list_for_each(&cs->str, struct cs_chunk, node) {
		chunk = malloc(sizeof (struct cs_chunk));
		if (!chunk) 
			goto free_clone;

		*chunk = *node;
		list_push_back(&clone->str, chunk);
	}
	
	clone->nchars = cs->nchars;
	return true;

free_clone:
	list_for_each(&clone->str, struct cs_chunk, node)
		free(node);
	return false;
}

char *cs_to_cstring(struct chunky_str *cs, unsigned long *length)
{
	/*
	 * we malloc one extra char's worth of space in case @cs is not
	 * null terminated
	 */
	char *cstr = malloc(sizeof(char)*(cs->nchars + 1));
	unsigned long i = 0;
	struct cs_cursor cursor = {.owner = cs, .chunk = NULL, .index = 0};
	char c;

	if (!cstr) {
		*length = 0;
		return cstr;
	}

	cs_for_each(&cursor) {
		c = cs_cursor_getchar(&cursor);
		cstr[i++] = c;
		if (c == NULL_BYTE) {
			*length = i;
			return cstr;
		}
	}
	cstr[i++] = NULL_BYTE;
	*length = i;
	return cstr;
}

unsigned long cs_write(struct chunky_str *cs, char *buf, unsigned long size)
{
	unsigned long nchars = size/sizeof(char);
	unsigned long i = 0;
	struct cs_cursor cursor = {.owner = cs, .chunk = NULL, .index = 0};
	char c;

	cs_for_each(&cursor) {
		c = cs_cursor_getchar(&cursor);
		if (i >= nchars)
			break;
		buf[i++] = c;
	}

	return i*sizeof(char);
}
