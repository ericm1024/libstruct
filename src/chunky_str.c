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

/* in case we want to reuse this implementation for other types */
typedef char char_t;

/* !!! not portable */
#define CACHELINE (64U)
/* wild guess of optimal size with no basis  */
#define CHUNKSIZE (CACHELINE*2)
#define NCHARS								\
	((CHUNKSIZE - (sizeof(struct list) + sizeof(short)))/sizeof(char_t))

struct cs_chunk {
	struct list link; /* chunk list. do NOT move this from offset 0 */
	unsigned short end; /* next free index in the chunk */
	char_t chars[NCHARS]; /* array of chars */
};

struct cs_cursor {
	struct chunky_str * const owner;
	struct cs_chunk *chunk;
	unsigned short index;
};

#define CURSOR_DEREF(curs) (curs)->chunk->chars[(curs)->index]

static inline bool valid_cursor(struct cs_cursor *cursor)
{
	return cursor->chunk;
}

static inline bool chunk_is_full(struct cs_chunk *chunk)
{
	return chunk->end == NCHARS;
}

static bool split_chunk(struct cs_cursor *cursor)
{
	struct cs_chunk* new_chunk = malloc(sizeof (struct cs_chunk));
	unsigned long old;
	unsigned long new; /* indicies */

	assert(chunk_is_full(cursor->chunk));
	if (!new_chunk)
		return false;
	
	/*
	 * put the chunk in the list; give it half the chars
	 * from cursor->chunk
	 */
	list_insert_after(&cursor->owner->str, cursor->chunk, new_chunk);
	for (old = NCHARS/2, new = 0; old < NCHARS; old++, new++)
		new_chunk->chars[new] = cursor->chunk->chars[old];

	/* at this point 'new' is the number of chars we moved to new_chunk */
	cursor->chunk->end -= new;
	new_chunk->end = new;
	if (cursor->index >= cursor->chunk->end) {
		cursor->chunk = new_chunk;
		cursor->index -= new;
	}
	return true;
}

static void maybe_merge_chunk(struct cs_cursor *cursor)
{
	struct cs_chunk *current = cursor->chunk;
	struct cs_chunk *prev;
	struct cs_chunk *next;
	unsigned long i;
	unsigned long j;
	unsigned long end;

	assert(current);

	prev = list_prev(cursor->owner->str, current);
	next = list_next(cursor->owner->str, current);
	
	/* Put the two adjacent chunks we want to merge into next and prev */ 
	if (prev && prev->end + current->end <= NCHARS)
		next = current;
	else if (next && next->end + current->end <= NCHARS)
		prev = current;
	else
		return;
	
	/* merge next into prev */
	end = next->end;
	for (i = prev->end, j = 0; j < end; i++, j++)
		prev->chars[i] = next->chars[j];
	prev->end = i;
	list_delete(cursor->owner->str, next);
	free(next);

	/* fix cursor, if necessary */
	if (current == next) {
		cursor->chunk = prev;
		cursor->index += prev->end;
		assert(cursor->index < NCHARS);
	}
}

#define SHIFT_FORWARD (1L)
#define SHIFT_REVERSE (-1L)

/*
 * Shift characters within a chunk to either clobber the character at the
 * cursor or make a space for a character at the cursor. Shift must be
 * SHIFT_FORWARD or SHIFT_REVERSE
 */ 
static void shift_chars(struct cs_cursor *cursor, long shift)
{
	char *chars = cursor->chunk->chars;
	unsigned long end = cursor->chunk->end;
	unsigned long start = cursor->index;

	assert(shift == SHIFT_FORWARD && end < NCHARS 
	       || shift == SHIFT_REVERSE);

	if (shift > 0)
		/* make room for a character */
		for (; start < end; end--)
			chars[end] = chars[end - 1];
	else
		/* clobber a character */
		for (; start < end; start++)
			chars[start] = chars[start + 1];
}

cs_cursor_t cs_get_cursor(struct chunky_str *cs)
{
	struct cs_cursor* cursor = malloc(sizeof(struct cs_cursor));
	if (cursor) {
		cursor->owner = cs;
		cs_cursor_begin(cursor);
	}
	return cursor;
}

cs_cursor_t cs_clone_cursor(cs_cursor_t jango)
{
        /*
	 * if you don't like the variable naming in this function,
	 * go watch star wars episode 2. If you still don't like the
	 * variable naming, then I don't care.
	 */
	struct cs_cursor* boba = malloc(sizeof(struct cs_cursor));
	if (boba)
		*boba = *jango;
	return boba;
}

bool cs_cursor_equal(cs_cursor_t lhs, cs_cursor_t rhs)
{
	return lhs->owner == rhs->owner 
		&& lhs->chunk == rhs->chunk
		&& lhs->index == rhs->index;
}

bool cs_cursor_begin(cs_cursor_t cursor)
{
	cursor->chunk = list_first(&cursor->owner->str);
	cursor->index = 0;
	return valid_cursor(cursor);
}

bool cs_cursor_end(cs_cursor_t cursor)
{
	cursor->chunk = list_last(&cursor->owner->str);
	cursor->index = cursor->chunk ? cursor->chunk->end - 1 : 0;
	return valid_cursor(cursor);
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
	return cursor->chunk ? CURSOR_DEREF(cursor) : NULL;
}

char cs_cursor_prev(cs_cursor_t cursor, int *status)
{
	cursor->index--;
	if (cursor->index >= cursor->chunk->end) {
		cursor->chunk = list_prev(&cursor->owner->str, cursor->chuk);
		if (cursor->chunk)
			cursor->index = cursor->chunk->end - 1;
	}
	return cursor->chunk ? CURSOR_DEREF(cursor) : NULL;
}

char cs_cursor_getchar(cs_cursor_t cursor)
{
	return cursor->chunk->chars[cursor->index];
}

bool cs_insert_before(cs_cursor_t cursor, char c)
{
	if (chunk_is_full(cursor->chunk))
		if (!split_chunk(cursor))
			return false;
	
	shift_chars(cursor, SHIFT_FORWARD);
	CURSOR_DEREF(cursor) = c;
	cursor->owner->nchars++;
	return true;
}

bool cs_insert_after(cs_cursor_t cursor, char c)
{
	if (chunk_is_full(cursor->chunk))
		if (!split_chunk(cursor))
			return false;

	cs_cursor_next(cursor);
	shift_chars(cursor, SHIFT_FORWARD);
	CURSOR_DEREF(cursor) = c;
	cursor->owner->nchars++;
	return true;
}

bool cs_insert_clobber(cs_cursor_t cursor, char c)
{
	CURSOR_DEREF(cursor) = c;
	return true;
}

bool cs_cursor_erase(cs_cursor_t cursor)
{
	if (cursor->owner->nchars == 0)
		return false;
	shift_chars(cursor, SHIFT_REVERSE);
	maybe_merge_chunk(cursor);
	cursor->owner->nchars--;
	return true;
}

bool cs_cursor_erase_get(cs_cursor_t cursor, char *c)
{
	if (cursor->owner->nchars == 0)
		return false;
	*c = CURSOR_DEREF(cursor);
	return cs_cursor_erase(cursor);
}

void cs_destroy(struct chunky_str *cs)
{
	list_for_each(cs->str, struct cs_chunk, i)
		free(i);
	cs->nchars = 0;
	cs->str->first = NULL;
	cs->str->last = NULL;
	cs->str->length = 0;
}

bool cs_clone(struct chunky_str *cs, struct chunky_str *clone)
{
	struct cs_chunk *pool;
	unsigned long i = 0;
	*clone = CHUNKY_STRING;

	/*
	 * It would be prettier to use the cursors we've spent so much time
	 * writing, but it's faster to just malloc all the chunks in one big
	 * blob because malloc is slow.
	 */
	pool = malloc(sizeof(struct cs_chunk) * cs->str->length);
	if (!pool)
		return false;
	list_for_each(cs->str, struct cs_chunk, i) {
		pool[i] = *i;
		list_push_back(clone->str, &pool[i]);
	}
	clone->nchars = cs->nchars;
	return true;
}

char *cs_to_cstring(struct chunky_str *cs, unsigned long *length)
{
	/*
	 * we malloc one extra char_t's worth of space in case @cs is not
	 * null terminated
	 */
	char_t *cstr = malloc(sizeof(char_t)*(cs->nchars + 1))
	unsigned long i = 0;
	struct cs_cursor cursor;

	if (!cstr) {
		*length = 0;
		return cstr;
	}

	cs_for_each(c, &cursor) {
		if (!c)
			break;
		cstr[i] = c;
		i++;
	}
	cstr[i] = NULL;
	*length = i;
	return cstr;
}

unsigned long cs_write(struct chunky_str *cs, char *buf, unsigned long size)
{
	unsigned long nchars = size/sizeof(char_t);
	unsigned long i = 0;
	struct cs_cursor cursor;

	cs_for_each(c, &cursor) {
		if (i > nchars)
			break;
		buf[i] = c;
		i++;
	}

	return i*sizeof(char_t);
}
