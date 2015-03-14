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
 * \file flist.h 
 *
 * \author Eric Mueller
 * 
 * \brief Header file for a forward list (singly linked).
 *
 * \detail This is a fairly straightforward forward-linked list (i.e. singly
 * linked list). It is implemented as a structure member and not a container,
 * so if you want your struct foo to be in a list, add a struct flist member
 * to your struct foo, i.e.
 *
 *     struct foo {
 *               .
 *               .
 *             struct flist link;
 *               .
 *               .
 *     };
 *
 * To use the list, first declare a struct flist_head with the FLIST_HEAD
 * macro, ex:
 *
 *     FLIST_HEAD(foo_list);
 *
 * Then use any combination of flist_push_front, flist_pop_front,
 * flist_insert_after, flist_splice, flist_for_each, and flist_for_each_range.
 *
 * This should go without saying, but the list does no memory allocation.
 *
 * Synchronization is left to the caller.
 */

#ifndef STRUCT_FLIST_H
#define STRUCT_FLIST_H 1

#include <stddef.h>
#include <stdint.h>

struct flist {
	struct flist *next;
};

struct flist_head {
	struct flist *first;
	unsigned long length;
	const unsigned long offset;
};

/**
 * Declares a new flist head.
 *
 * \param  The name of the new list.
 * \param type    The type of the enclosing struct.
 * \param member  The name of the member in the struct declaration.
 */
#define FLIST_HEAD(name, type, member)			                 \
	struct flist_head name = (struct flist_head){                    \
		                     .first = NULL,	                 \
				     .length = 0,	                 \
	                             .offset = offsetof(type, member)};


/**
 * Insert a list element after another element.
 *
 * \param hd        Pointer to the head of the list.
 * \param after     Object to insert after. If null, insertee is inserted at the
 *                  front of the list.
 * \param insertee  Object to insert.
 */
extern void flist_insert_after(struct flist_head *hd, void *after,
			       void *insertee);

/**
 * Insert an element at the front of a list.
 *
 * \param hd        Pointer to the head of the list.
 * \param insertee  Object to insert.
 */
extern void flist_push_front(struct flist_head *hd, void *insertee);

/**
 * Remove the first element of the list.
 *
 * \param hd  Pointer to the head of the list.
 * \return Pointer to the first element in the list.
 */
extern void *flist_pop_front(struct flist_head *hd);

/**
 * Insert an entire list into the list hd after the element after. The head
 * of the inserted list is invalidated, i.e. its first pointer is set to null
 * and length is set to zero.
 *
 * \param hd       Pointer to the list to insert into. 
 * \param after    Splicee is inserted after this element
 * \param splicee  Pointer to the list to insert. Invalidated by calling this
 *                 function.
 */
extern void flist_splice(struct flist_head *hd, void *after,
			 struct flist_head *splicee);

/**
 * Execute a function on each element in the list. The function is applied to
 * the container, not the list node itself.
 *
 * \param hd      Pointer to the head of the list.
 * \param f       Pointer to the function to apply to each element.
 */
extern void flist_for_each(struct flist_head *hd, void (*f)(void *data));

/**
 * Execute a function on each element in the list in the range [first, last).
 * The function is applied to the container, not the list node itself.
 *
 * \param hd      Pointer to the head of the list.
 * \param f       Pointer to the functiuon to apply to each element.
 * \param first   Pointer to the first element to apply the function to.
 * \param last    Pointer to the element after the last element on which the
 *                function will be called.
 */
extern void flist_for_each_range(struct flist_head *hd, void (*f)(void *data),
				 void *first,void *last);

/**
 * Get the first element in a list from the list head.
 * 
 * \param hd  Head of the list
 * \return The first element in the list
 */
static inline void *flist_first(struct flist_head *hd)
{
	return hd->first ? (void*)((uintptr_t)hd->first - hd->offset) : NULL;
}

/**
 * Get the next element in a list given a current element.
 *
 * \param hd    The head of the list.
 * \param elem  The element we are at now.
 * \return The next element in the list
 */
static inline void *flist_next(struct flist_head *hd, void *elem)
{
	struct flist *e = (struct flist *)((uintptr_t)elem + hd->offset);
	return e->next ? (void*)((uintptr_t)e->next - hd->offset) : NULL;
}

#endif /* STRUCT_FLIST_H */
