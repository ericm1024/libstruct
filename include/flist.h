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
 */

#ifndef STRUCT_FLIST_H
#define STRUCT_FLIST_H 1

#include "libstruct.h"

/**
 * Declares a new flist head.
 *
 * \param  The name of the new list.
 */
#define FLIST_HEAD(name) \
	struct flist_head name = {NULL, 0};

/**
 * This macro is taken out of the Linux kernel and is not my own.
 * (This comment however, is)
 *
 * \param ptr     Pointer to a member of a struct.
 * \para mtype    The type of the enclosing struct.
 * \param member  The name of the member in the struct declaration.
 */
#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

/**
 * Insert a list element after another element.
 *
 * \param hd        Pointer to the head of the list.
 * \param after     Pointer to the list node to insert after. If null, insertee is
 *                  inserted at the front of the list.
 * \param insertee  Pointer the the list node to insert.
 */
extern void flist_insert_after(struct flist_head *hd, struct flist *after,
			       struct flist *insertee);

/**
 * Insert an element at the front of a list.
 *
 * \param hd        Pointer to the head of the list.
 * \param insertee  Pointer to the list node to insert.
 */
extern void flist_push_front(struct flist_head *hd, struct flist *insertee);

/**
 * Remove the first element of the list.
 *
 * \param hd  Pointer to the head of the list.
 * \return Pointer to the first element in the list.
 */
extern struct flist *flist_pop_front(struct flist_head *hd);

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
extern void flist_splice(struct flist_head *hd, struct flist *after,
			 struct flist_head *splicee);

/**
 * Execute a function on each element in the list. The function is applied to
 * the container, not the list node itself.
 *
 * \param hd      Pointer to the head of the list.
 * \param f       Pointer to the function to apply to each element.
 * \param offset  The offset of the list member in the enclosing data structure.
 */
extern void flist_for_each(struct flist_head *hd, void (*f)(void *data),
			   ptrdiff_t offset);

/**
 * Execute a function on each element in the list in the range [first, last).
 * The function is applied to the container, not the list node itself.
 *
 * \param hd      Pointer to the head of the list.
 * \param f       Pointer to the functiuon to apply to each element.
 * \param offset  The offset of the list member in the enclosing data structure.
 * \param first   Pointer to the first element to apply the function to.
 * \param last    Pointer to the element after the last element on which the
 *                function will be called.
 */
extern void flist_for_each_range(struct flist_head *hd, void (*f)(void *data),
				 ptrdiff_t offset, struct flist *first,
				 struct flist *last);

#endif /* STRUCT_FLIST_H */
