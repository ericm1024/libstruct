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

typedef char char_t;

/* !!! not portable */
#define CACHELINE (64U)
/* wild guess of optimal size with no basis  */
#define CHUNKSIZE (CACHELINE*2)
#define NCHARS ((CHUNKSIZE - (sizeof(struct list) + sizeof(short))/sizeof(char_t))

struct cs_chunk {
	struct list link; /* do NOT move this from offset 0 */
	unsigned short next; /* next free index in the chunk */
	char_t chars[NCHARS];
};

struct cs_cursor {
	struct list link; /* do NOT move this from offset 0 */
	struct chunky_str *owner;
	struct cs_chunk *chunk;
	unsigned short index;
	bool valid;
};

/*
 * reasons for a cursor to be invalid
 *     - insert or erase was called elsewhere in the same list.
 *     - the cursor was initialized for an empty string
 *     - the user invalidated the cursor
 */ 

cs_cursor_t cs_get_cursor(struct chunky_str *cs)
{
	cs_cursor_t crs = malloc(sizeof(struct cs_curor));
	if (!crs)
		return crs;
	crs->owner = cs;
	crs->chunk = cs->str.first;
	crs->index = 0;
}
