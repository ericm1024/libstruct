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
 * \file flist.c
 * 
 * \author Eric Mueller
 * 
 * \brief Implementation of a forward list (singly linked).
 */

#include "flist.h"
#include <assert.h>
#include <stdbool.h>

static inline void *node_to_data(struct flist_head *hd, struct flist *n)
{
	return (void *)((uintptr_t)n - hd->offset);
}

static inline struct flist *data_to_node(struct flist_head *hd, void *d)
{
	return (struct flist *)((uintptr_t)d + hd->offset);
}

static inline void link3(struct flist *a, struct flist *b, struct flist *c)
{
	if (a)
		a->next = b;
	if (b)
		b->next = c;
}

static inline bool is_empty(struct flist_head *hd)
{
	return hd->length == 0 && hd->first == NULL ? true : false;
}

static inline struct flist *get_last(struct flist_head *hd)
{
	struct flist *last;
	for (last = hd->first; last->next; last = last->next)
		;
	return last;
}

void flist_insert_after(struct flist_head *hd, void *after,
			void *insertee)
{
	struct flist *l_after;
	struct flist *l_in = data_to_node(hd, insertee);
	
	if (after) {
		l_after = data_to_node(hd, after);
		link3(l_after, l_in, l_after->next);
	} else {
		l_in->next = hd->first;
		hd->first = l_in;
	}
		
	hd->length++;
}

void flist_push_front(struct flist_head *hd, void *insertee)
{
	struct flist *l_in = data_to_node(hd, insertee);

	link3(NULL, l_in, hd->first);
	hd->first = l_in;
	hd->length++;
}

void *flist_pop_front(struct flist_head *hd)
{
	struct flist *first = hd->first;
	void *ret = NULL;
	if (first) {
		hd->first = first->next;
		hd->length--;
		ret = node_to_data(hd, first);
	}
	return ret;
}

void flist_splice(struct flist_head *hd, void *after,
		  struct flist_head *splicee)
{
	struct flist *l_after = data_to_node(hd, after);
  
	if (is_empty(splicee))
		return;

	assert(hd->offset == splicee->offset);

	struct flist *last = get_last(splicee);
	last->next = l_after->next;
	l_after->next = splicee->first;
	hd->length += splicee->length;

	/* invalidate splicee */
	splicee->first = NULL;
	splicee->length = 0;
}

void flist_for_each(struct flist_head *hd, void (*f)(void *data))
{
	for (struct flist *i = hd->first; i != NULL; ) { 
		/*
		 * We need to grab the next element in the list *before* we call the
		 * function on the current element because the function could
		 * invalidate the current element, in which case itterating with
		 * i = i->next would cause undefined behavior. An example of such a
		 * function is free.
		 */ 
		struct flist *next = i->next;
		f(node_to_data(hd,i));
		i = next;
	}
}

void flist_for_each_range(struct flist_head *hd, void (*f)(void *data),
			  void *first, void *last)
{
	struct flist *l_last = last ? data_to_node(hd,last) : NULL;

	for (struct flist *i = first ? data_to_node(hd, first) : NULL;
	     i && i != l_last; ) {
		/* see comment in flist_for_each */ 
		struct flist *next = i->next;
		f(node_to_data(hd,i));
		i = next;
	}
}

