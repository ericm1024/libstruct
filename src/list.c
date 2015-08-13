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

static inline void *node_to_data(struct list_head *hd, struct list *n)
{
        return n ? (void *)((uintptr_t)n - hd->offset) : NULL;
}

static inline struct list *data_to_node(struct list_head *hd, void *d)
{
        return d ? (struct list *)((uintptr_t)d + hd->offset) : NULL;
}

static inline void list_link(struct list *a, struct list *b)
{
	if (a)
		a->next = b;
	if (b)
		b->prev = a;
}

static inline void list_link3(struct list *a, struct list *b, struct list *c)
{
	list_link(a,b);
	list_link(b,c);
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

void list_insert_before(struct list_head *hd, void *before,
			void *insertee)
{
	struct list *l_before = data_to_node(hd, before);
	struct list *l_in = data_to_node(hd, insertee);
	
	if (!before) {
		list_push_back(hd, insertee);
		return;
	}
	
	if (is_first(l_before))
		hd->first = l_in;
	list_link3(l_before->prev, l_in, l_before);
	hd->length++;
}

void list_insert_after(struct list_head *hd, void *after, void *in)
{
	struct list *l_after = data_to_node(hd, after);
	struct list *l_in = data_to_node(hd, in);

	if (!after) {
		list_push_front(hd, in);
		return;
	}
	
	if (is_last(l_after))
		hd->last = l_in;
	list_link3(l_after, l_in, l_after->next);
	hd->length++;
}

void list_delete(struct list_head *hd, void *victim)
{
	struct list *l_vic = data_to_node(hd, victim);

	if (!victim)
		return;
	
	if (is_last(l_vic))
		hd->last = l_vic->prev;
	if (is_first(l_vic))
		hd->first = l_vic->next;
	list_link(l_vic->prev, l_vic->next);
	hd->length--;
}

void list_push_front(struct list_head *hd, void *pushee)
{
	struct list *push = data_to_node(hd, pushee);
	
	terminate_with_nulls(push);
	list_link(push, hd->first);
	if (is_empty(hd))	
		hd->last = push;
	hd->first = push;
	hd->length++;
}

void list_push_back(struct list_head *hd, void *pushee)
{
	struct list *push = data_to_node(hd, pushee);

	terminate_with_nulls(push);
	list_link(hd->last, push);
	if (is_empty(hd))
		hd->first = push;
	hd->last = push;
	hd->length++;
}

void *list_pop_front(struct list_head *hd)
{
	void *victim = node_to_data(hd, hd->first);
	list_delete(hd, victim);
	return victim;
}

void *list_pop_back(struct list_head *hd)
{
	void *victim = node_to_data(hd, hd->last);
	list_delete(hd, victim);
	return victim;
}

void list_splice(struct list_head *hd, void *after,
		 struct list_head *splicee)
{
	struct list *l_after = data_to_node(hd, after);
	if (!splicee || is_empty(splicee))
		return;

	assert(hd->offset == splicee->offset);

	if (after) {
		list_link(splicee->last, l_after->next);
		list_link(l_after, splicee->first);
	} else {
		list_link(splicee->last, hd->first);
		hd->first = splicee->first;
	}
		
	hd->length += splicee->length;

	/* invalidate splicee */
	splicee->first = NULL;
	splicee->last = NULL;
	splicee->length = 0;
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
