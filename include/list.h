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
 * \file list.h
 *
 * \author Eric Mueller
 *
 * \brief Header file for a doubly linked list.
 *
 * \detail This is a fairly straightforward doubly-linked list. It is
 * implemented as a structure member and not a container, so if you want
 * your struct foo to be in a list, add a struct list member to your struct
 * foo, i.e.
 * 
 *     struct foo {
 *               .
 *               .
 *             struct list link;
 *               .
 *               .
 *     };
 *
 * To use the list, first declare a struct flist_head with the FLIST_HEAD
 * macro, ex:
 *
 *     LIST_HEAD(foo_list);
 *
 * Then use any combination of list_insert_before, list_insert_after,
 * list_delete, list_push_front, list_push_back, list_pop_front, list_pop_back,
 * list_splice, list_for_each, list_for_each_range, and list_revers.
 *
 * This should go without saying, but the list does no memory allocation.
 *
 * Synchronization is left to the caller.
 */

#ifndef STRUCT_LIST_H
#define STRUCT_LIST_H 1

#include <stddef.h>
#include <stdint.h>

/* doubly-linked list types */
struct list {
	struct list *next;
	struct list *prev;
};

struct list_head {
	struct list *first;
	struct list *last;
	size_t length;
	unsigned long offset;
};

/**
 * \brief Create and initialize a new empty list_head.
 * 
 * \param name    The name of the list to create.
 * \param type    Type of the enclosing struct.
 * \param member  Name of the list member in the enclosing struct declaration.
 */
#define LIST_HEAD(name, type, member)				   \
	struct list_head name =	(struct list_head){		   \
		                 .first = NULL,			   \
				 .last = NULL,			   \
				 .length = 0,			   \
				 .offset = offsetof(type, member)}

/**
 * \brief Insert a list element before a given list element.
 *
 * \param hd        Pointer to the head of the list. 
 * \param before    Pointer to the element to insert before. May be null, in
 *                  which case behavior is the same as list_push_back
 * \param insertee  Pointer to the element to insert.
 */
extern void list_insert_before(struct list_head *hd, void *before,
			       void *insertee);

/**
 * \brief Insert a list element after a given element.
 *
 * \param hd        Pointer to the head of the list.
 * \param after     Pointer to the element to insert after. May be null, in
 *                  which case behavior is the same as list_push_front
 * \param insertee  Pointer to the element to insert.
 */
extern void list_insert_after(struct list_head *hd, void *after,
			      void *insertee);

/**
 * \brief Remove an element from a list.
 *
 * \param hd      Pointer to the head of the list. 
 * \param elem    Pointer to the list element to remove. If NULL, nothing is
 *                done.
 */
extern void list_delete(struct list_head *hd, void *elem);

/**
 * \brief Insert an element at the front of a list.
 *
 * \param hd      Pointer to the head of the list.
 * \param pushee  Pointer to the element to insert.
 */
extern void list_push_front(struct list_head *hd, void *pushee);

/**
 * Insert an element at the back of a list.
 *
 * \param hd      Pointer to the head of the list.
 * \param pushee  Pointer to the element to insert.
 */
extern void list_push_back(struct list_head *hd, void *pushee);

/**
 * \brief Remove and return the first element of a list.
 *
 * \param hd  Pointer to the head of the list.
 * \retrn     A pointer to the first element in the list, or NULL if the list
 *            is empty.
 */
extern void *list_pop_front(struct list_head *hd);

/**
 * \brief Remove and return the last element of a list.
 *
 * \param hd  Pointer to the head of the list. A NULL head 
 * \return    A pointer to the last element in the list, or NULL if the list
 *            is empty.
 */
extern void *list_pop_back(struct list_head *hd);

/**
 * \brief Insert an entire list after the node 'after'.
 * \warning Splicee is no longer valid after this function is called (its length
 * and members are zero'd out).
 * 
 * \param hd       Pointer to the head of the first list.
 * \param where    Pointer to the list node to splice after.
 * \param splicee  Pointer to the head of the list to insert.
 */
extern void list_splice(struct list_head *hd, void *after,
			struct list_head *splicee);

/**
 * \brief Execute a function on each element in the list.
 * \note The function is applied to the container, not the list node itself.
 *
 * \param hd      Pointer to the head of the list.
 * \param f       Pointer to the function to apply to each element.
 */
extern void list_for_each(struct list_head *hd, void (*f)(void *data));

/**
 * \brief Execute a function on each element in the list in the range [first,
 * last).
 * \note The function is applied to the container, not the list node itself.
 *
 * \param hd      Pointer to the head of the list.
 * \param f       Pointer to the function to apply to each element.
 * \param first   Pointer to the first element to apply the function to.
 * \param last    Pointer to the element after the last element on which the
 *                function will be called.
 */
extern void list_for_each_range(struct list_head *hd, void (*f)(void *data),
				void *first, void *last);
/**
 * \brief Reverse a list.
 *
 * \param hd  Pointer to the head of the list to reverse. 
 */
extern void list_reverse(struct list_head *hd);

/**
 * Get the first element in a list.
 *
 * \param hd  The head of the list.
 * \return The first element in the list.
 */
static inline void *list_first(struct list_head *hd)
{
	return hd->first ? (void *)((uintptr_t)hd->first - hd->offset) : NULL;
}

/**
 * Get the last element in a list.
 *
 * \param hd  The head of the list.
 * \return The last element in the list.
 */
static inline void *list_last(struct list_head *hd)
{
	return hd->last ? (void *)((uintptr_t)hd->last - hd->offset) : NULL;
}

/**
 * Get the next element in a list.
 *
 * \param hd    The head of the list.
 * \param elem  The current element.
 * \return The next element in the list.
 */
static inline void *list_next(struct list_head *hd, void *elem)
{
	struct list *curent = (struct list *)((uintptr_t)elem + hd->offset);
	return curent->next ? (void *)((uintptr_t)curent->next - hd->offset) 
		            : NULL;
}

/**
 * Get the previous element in a list.
 * 
 * \param hd    The head of the list.
 * \param elem  The current element.
 * \return The previous element in the list.
 */
static inline void *list_prev(struct list_head *hd, void *elem)
{
	struct list *curent = (struct list *)((uintptr_t)elem + hd->offset);
	return curent->prev ? (void *)((uintptr_t)curent->prev - hd->offset)
		            : NULL;
}

#endif /* STRUCT_LIST_H */
