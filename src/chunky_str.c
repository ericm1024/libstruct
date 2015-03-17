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
 * a simple free list to manage memory for list chunks. This would greatly
 * reduce the number of calls to malloc/free and would probably make the
 * structure quite snappy. But we'll see. For now, I need to write the damn
 * thing.
 */

#include "chunky_str.h"
#include <stdlib.h>

typedef char char_t;

/* !!! not portable */
#define CACHELINE (64U)
/* wild guess of optimal size with no basis  */
#define CHUNKSIZE (CACHELINE*2)
#define NCHARS ((CHUNKSIZE - (sizeof(struct list) + sizeof(short)))/sizeof(char_t))

struct cs_chunk {
	struct list link; /* do NOT move this from offset 0 */
	unsigned short end; /* next free index in the chunk */
	char_t chars[NCHARS];
};

struct cs_cursor {
	struct list link; /* do NOT move this from offset 0 */
	struct chunky_str * const owner;
	struct cs_chunk *chunk;
	unsigned short index;
	unsigned short flags;
};

/*
 * reasons for a cursor to be invalid
 *     - insert or erase was called elsewhere in the same list.
 *     - the cursor was initialized for an empty string
 *     - the user invalidated the cursor
 */ 

#define INVAL_MODIFIED 0x1
#define INVAL_EMPTY 0x2
#define INVAL_USER 0x4
#define INVAL_RANGE 0x8

static inline bool valid_cursor(struct cs_cursor *cursor)
{
	return !cursor->flags;
}

cs_cursor_t cs_get_cursor(struct chunky_str *cs)
{
	struct cs_cursor* cursor = malloc(sizeof(struct cs_cursor));
	if (cursor) {
		cursor->owner = cs;
		cs_cursor_begin(cursor);
		list_push_back(&cs->cursors, cursor);
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
	if (boba) { 
		*boba = *jango;
		list_push_back(&jango->owner->cursors, boba);
	}
	return boba;
}

bool cs_cursor_equal(cs_cursor_t lhs, cs_cursor_t rhs)
{
	return valid_cursor(lhs) 
		&& valid_cursor(rhs)
		&& lhs->owner == rhs->owner 
		&& lhs->chunk == rhs->chunk
		&& lhs->index == rhs->index;
}

bool cs_cursor_begin(cs_cursor_t cursor)
{
	cursor->chunk = list_first(&cursor->owner->str);
	cursor->index = 0;
	cursor->flags = cursor->chunk ? 0 : INVAL_EMPTY;
	return valid_cursor(cursor);
}

bool cs_cursor_end(cs_cursor_t cursor)
{
	cursor->chunk = list_last(&cursor->owner->str);
	cursor->index = cursor->chunk ? cursor->chunk->end - 1 : 0;
	cursor->flags = cursor->chunk ? 0 : INVAL_EMPTY;
	return valid_cursor(cursor);
}

bool cs_cursor_in_range(cs_cursor_t cursor)
{
	return !(cursor->flags & INVAL_RANGE);
}

void cs_cursor_destroy(cs_cursor_t cursor)
{
	list_delete(cursor->owner->cursors, cursor);
	free(cursor);
}

char cs_cursor_next(cs_cursor_t cursor)
{
	cursor->index++;
	if (cursor->index >= cursor->chunk->end) {
		cursor->chunk = list_next(&cursor->owner->str, cursor->chunk);
		cursor->index = 0;
		if (!cursor->chunk)
			cursor->flags |= INVAL_RANGE;
	}
	return cursor->chunk ? cursor->chunk->chars[cursor->index] : NULL;
}

char cs_cursor_prev(cs_cursor_t cursor, int *status)
{
	cursor->index--;
	if (cursor->index >= cursor->chunk->end) {
		cursor->chunk = list_prev(&cursor->owner->str, cursor->chuk);
		if (cursor->chunk)
			cursor->index = cursor->chunk->end - 1;
		else
			cursor->flags |= INVAL_RANGE;
	}
	return cursor->chunk ? cursor->chunk->chars[cursor->index] : NULL;
}

char cs_cursor_get(cs_cursor_t cursor)
{
	return cursor->chunk->chars[cursor->index];
}

bool cs_insert_before(cs_cursor_t cursor, char c)
{
	return false;
}

bool cs_insert_after(cs_cursor_t cursor, char c)
{
	return false;
}

bool cs_insert_clobber(cs_cursor_t cursor, char c)
{
	return false;
}

bool cs_erase(cs_cursor_t cursor, char *c)
{
	return false;
}

bool cs_cursor_is_valid(cs_cursor_t cursor)
{
	return valid_cursor(cursor);
}

void cs_invalidate_cursor(cs_cursor_t cursor)
{
	cursor->flags |= INVAL_USER;
}

void cs_do_cursor_gc(struct chunky_str *cs)
{
	return;
}

void cs_destroy(struct chunky_str *cs)
{
	return;
}

struct chunky_str *cs_clone(struct chunky_str *cs)
{
	return NULL;
}

char *cs_to_cstring(struct chunky_str *cs, unsigned long *length)
{
	
}

unsigned long cs_print(struct chunky_str *cs, char *buf, unsigned long size)
{

}
