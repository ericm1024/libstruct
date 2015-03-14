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
 * \file list.c
 * \author Eric Mueller
 *
 * \brief Doubly linked list implementation.
 */

#include "list.h"
#include <assert.h>
#include <stdbool.h>

static inline void link(struct list *a, struct list *b)
{
	if (a)
		a->next = b;
	if (b)
		b->prev = a;
}

static inline void link3( struct list *a, struct list *b, struct list *c)
{
	link(a,b);
	link(b,c);
}

static inline bool is_first(struct list *a)
{
	return !a->prev;
}

static inline bool is_last(struct list *a)
{
	return !a->next;
}

static inline bool is_empty(struct list_head *hd)
{
	return !hd->first && !hd->last && hd->length == 0;
}

static inline void terminate_with_nulls(struct list *a)
{
	a->prev = NULL;
	a->next = NULL;
}

void list_insert_before(struct list_head *hd, struct list *before,
			struct list *insertee)
{
	assert(hd);
	assert(insertee);
	
	if (!before) {
		list_push_back(hd, insertee);
		return;
	}
	
	if (is_first(before))
		hd->first = insertee;
	link3(before->prev, insertee, before);
	hd->length++;
}

void list_insert_after(struct list_head *hd, struct list *after,
		       struct list *insertee)
{
	assert(hd);
	assert(insertee);

	if (!after) {
		list_push_front(hd, insertee);
		return;
	}
	
	
	if (is_last(after))
		hd->last = insertee;
	link3(after, insertee, after->next);
	hd->length++;
}

void list_delete(struct list_head *hd, struct list *victim)
{
	assert(hd);

	if (!victim)
		return;
	
	if (is_last(victim))
		hd->last = victim->prev;
	if (is_first(victim))
		hd->first = victim->next;
	link(victim->prev, victim->next);
	hd->length--;
}

void list_push_front(struct list_head *hd, struct list *pushee)
{
	assert(hd);
	assert(pushee);
	
	terminate_with_nulls(pushee);
	link(pushee, hd->first);
	if (is_empty(hd))	
		hd->last = pushee;
	hd->first = pushee;
	hd->length++;
}

void list_push_back(struct list_head *hd, struct list *pushee)
{
	assert(hd);
	assert(pushee);

	terminate_with_nulls(pushee);
	link(hd->last, pushee);
	if (is_empty(hd))
		hd->first = pushee;
	hd->last = pushee;
	hd->length++;
}

struct list *list_pop_front(struct list_head *hd)
{
	assert(hd);
	
	struct list *victim = hd->first;
	list_delete(hd, victim);
	return victim;
}

struct list *list_pop_back(struct list_head *hd)
{
	assert(hd);
	
	struct list *victim = hd->last;
	list_delete(hd, victim);
	return victim;
}

void list_splice(struct list_head *hd, struct list *after,
		 struct list_head *splicee)
{
	assert(hd);

	if (!splicee || is_empty(splicee))
		return;

	if (after) {
		link(splicee->last, after->next);
		link(after, splicee->first);
	} else {
		link(splicee->last, hd->first);
		hd->first = splicee->first;
	}
		
	hd->length += splicee->length;

	/* invalidate splicee */
	splicee->first = NULL;
	splicee->last = NULL;
	splicee->length = 0;
}

void list_for_each(struct list_head *hd, void (*f)(void *data),
		   ptrdiff_t offset)
{
	assert(hd);
	assert(f);

	for (struct list *i = hd->first; i; ) {
		struct list *next = i->next;
		f( (void *)((char *)i - offset) );
		i = next;
	}	
}

void list_for_each_range(struct list_head *hd, void (*f)(void *data),
			 ptrdiff_t offset, struct list *first,
			 struct list *last )
{
	assert(hd);
	assert(first);
	assert(f);

	for (struct list *i = first; i && i != last; ) {
		struct list *next = i->next;
		f( (void *)((char *)i - offset) );
		i = next;
	}
}

void list_reverse(struct list_head *hd)
{
	assert(hd);
	
	for (struct list *i = hd->first; i; ) {
		struct list *next = i->next;
		i->next = i->prev;
		i->prev = next;
		i = next;
	}
	struct list *first = hd->first;
	hd->first = hd->last;
	hd->last = first;
}
