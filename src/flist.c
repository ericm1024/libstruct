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

static inline void link3(struct flist *a, struct flist *b, struct flist *c)
{
	if ( a )
		a->next = b;
	if ( b )
		b->next = c;
}

static inline int is_empty(struct flist_head *hd)
{
	return hd->length == 0 && hd->first == NULL ? 1 : 0;
}

static inline struct flist *get_last(struct flist_head *hd)
{
	struct flist *last;
	for (last = hd->first; last->next != NULL; last = last->next) {}
	return last;
}

void flist_insert_after(struct flist_head *hd, struct flist *after,
			struct flist *insertee)
{
	assert(hd);
	assert(insertee);
	
	if (after)
		link3(after, insertee, after->next);
	else {
		insertee->next = hd->first;
		hd->first = insertee;
	}
		
	hd->length++;
}

void flist_push_front(struct flist_head *hd, struct flist *insertee)
{
	assert(hd);
	assert(insertee);

	link3(NULL, insertee, hd->first);
	hd->first = insertee;
	hd->length++;
}

struct flist *flist_pop_front(struct flist_head *hd)
{
	assert(hd);

	struct flist *first = hd->first;
	if ( first )
		hd->first = first->next;
	hd->length--;
	return first;
}

void flist_splice(struct flist_head *hd, struct flist *after,
		  struct flist_head *splicee)
{
	assert(hd);
	assert(after);
	assert(splicee);
  
	if ( is_empty(splicee) )
		return;

	struct flist *last = get_last(splicee);
	last->next = after->next;
	after->next = splicee->first;
	hd->length += splicee->length;

	/* invalidate splicee */
	splicee->first = NULL;
	splicee->length = 0;
}

void flist_for_each(struct flist_head *hd, void (*f)(void *data),
		    ptrdiff_t offset)
{
  assert(hd);
  assert(f);
  assert(offset >= 0);

  for (struct flist *i = hd->first; i != NULL; ) { 
	  /* We need to grab the next element in the list *before* we call the
	   * function on the current element because the function could
	   * invalidate the current element, in which case itterating with
	   * i = i->next would cause undefined behavior. An example of such a
	   * function is free.
	   */ 
	  struct flist *next = i->next;
	  f( (void *)((char *)i - offset) );
	  i = next;
  }
}

void flist_for_each_range(struct flist_head *hd, void (*f)(void *data),
			  ptrdiff_t offset, struct flist *first,
			  struct flist *last)
{
  assert(hd);
  assert(first);
  assert(f);

  for (struct flist *i = first; i != last && i != NULL; ) {
	  /* see comment in flist_for_each */ 
	  struct flist *next = i->next;
	  f( (void *)((char *)i - offset) );
	  i = next;
  }
}

