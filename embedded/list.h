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

/* 
 * Eric Mueller
 * November 2014
 * list.h
 *
 * Header file for a doubly linked list implementation.
 */

#ifndef STRUCT_LIST_H
#define STRUCT_LIST_H 1

#include "list_types.h"
#include <stddef.h>

static inline void link(list *a, list *b)
{
	a->next = b;
	b->prev = a;
}

/**
 * Create and initialize a new list.
 * @name The name of the list to create.
 */
#define LIST_HEAD(name) \
struct list name = {&(name), &(name)}

/**
 * This macro is taken out of the Linux kernel and is not my own code.
 * (This comment however, is)
 * 
 * @ptr     Pointer to a member of a struct.
 * @type    The type of the enclosing struct.
 * @member  The name of the member in the structure declaration.
 */
#define container_of(ptr, type, member) ({                      \
	const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
	(type *)( (char *)__mptr - offsetof(type,member) );})

/**
 * Insert a list element before a given list element.
 *
 * @before    Pointer to the element to insert before.
 * @insertee  Pointer to the element to insert.
 */
inline void list_insert_before(list *before, list *insertee)
{
	if (before->prev) 
		link(before->prev, insertee);
	else /* first node */
		insertee->prev = NULL;
	link(insertee, before);
}

/**
 * Insert a list element after a given element.
 *
 * @after     Pointer to the element to insert after.
 * @insertee  Pointer to the element to insert.
 */
inline void list_insert_after(list *after, list *insertee)
{
	if (after->next)
		link(insertee, after->next);
	else /* last node */
		insertee->next = NULL;
	link(after, insertee);
}

/**
 * Remove an element from a list
 *
 * @elem  Pointer to the list element to remove.
 */
inline void list_delete(list *elem)
{
	if (elem->prev && elem->next) 
		link(elem->prev, elem->next);
	else if (elem->prev) /* removing the end */
		elem->prev.next = NULL;
	else if (elem->next) /* removing the beginning */
		elem->next.prev = NULL;
	else /* the list only had 1 element */
		;
}

// splice

// push front

// push back




#endif /* STRUCT_LIST_H */
