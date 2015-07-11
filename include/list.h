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
 * \brief Header file for a doubly linked list. Based heavily off of linux kernel
 * linked lists.
 *
 * \detail This is a fairly straightforward circular doubly-linked list. It is
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
 * The 'head' of the list is no more that another list link. An empty list
 * is thus represented as a list whose next and prev members point to itself.
 * This, combined with the cyclic representation, ensure that there are never
 * null pointers in the list, which means this code is almost entirely
 * branchless.
 * 
 * This should go without saying, but the list does no memory allocation.
 *
 * Synchronization is left to the caller.
 */

#ifndef STRUCT_LIST_H
#define STRUCT_LIST_H 1

#include <stddef.h>
#include <stdint.h>
#include <assert.h>

#include "util.h"

struct list {
        /* next element, or first if this is the head of a list */
	struct list *next;
        
        /* previous element, or last if this is the head of a list */
	struct list *prev;
};

/**
 * \brief Initialize a list.
 * 
 * \param name    struct list to initizlize. (not a pointer)
 */
#define LIST_INIT(list) { .next = &(list), .prev = &(list) }

/* FIXME */
static inline void list_headswap(struct list_head *one,
                                 struct list_head *other)
{
        assert(one->offset == other->offset);

        swap_t(size_t, one->length, other->length);
        swap_t(struct list *, one->first, other->first);
        swap_t(struct list *, one->last, other->last);
}

static inline bool list_is_empty(struct list *list)
{
        return list->next == list;
}

/**
 * \brief Insert a list element before a given list element. If before is the
 * head of a list, this is equivalent to pushing onto the back of a list.
 *
 * \param before    Element to insert before. May not be null.
 * \param insertee  Element to insert.
 */
static inline void
list_insert_before(struct list *before, struct list *insertee)
{
        before->prev->next = insertee;
        insertee->prev = before->prev;
        insertee->next = before;
        before->prev = insertee;
}

/**
 * \brief Insert a list element after a given element. If after is the head
 * of a list, this is equivalent to pushing onto the front of a list.
 *
 * \param after     Element to insert after. May not be null.
 * \param insertee  Element to insert.
 */
static inline void
list_insert_after(struct list *after, struct list *insertee)
{
        after->next->prev = insertee;
        insertee->next = after->next;
        insertee->prev = after;
        after->next = insertee;
}

/**
 * \brief Remove an element from a list.
 *
 * \param victim    List element to remove.
 */
static inline void list_delete(struct list *victim)
{
        victim->next->prev = victim->prev;
        victim->prev->next = victim->next;
}

/**
 * \brief Remove and return the first element of a list.
 *
 * \param hd  head of the list.
 * \retrn     The first element of the list or NULL if the list is empty.
 */
static inline struct list *list_pop_front(struct list *hd)
{
        struct list *front = hd->next;
        list_delete(front);

        /* if the head of the list was itself, it was empty */
        return front == hd ? NULL : front;
}

/**
 * \brief Remove and return the last element of a list.
 *
 * \param hd  Head of the list. 
 * \return    The last element of the list or NULL if the list is empty.
 */
static inline struct list *list_pop_back(struct list *hd)
{
        struct list *back = hd->next;
        list_delete(back);

        return back == hd ? NULL : back;
}

/**
 * \brief Insert an entire list after the node 'after'.
 * 
 * \param after    Pointer to the element of the first list to insert after.
 * \param splicee  Head of the list to splice in. This list is re-initialized
 *                 so that we don't end up with a list with two heads.
 */
static inline void list_splice(struct list *after, struct list *splicee)
{
        if (list_is_empty(splicee))
                return;

        /* before:
         * A -- after -- B
         * C -- splicee -- D
         *
         * after:  
         * A -- after -- D -- (rest of splicee list) -- C -- B
         */ 
        after->next->prev = splicee->prev;
        splicee->prev->next = after->next;
        after->next = splicee->next;
        splicee->next->prev = after;

        LIST_INIT(splicee);
}

/**
 * \brief get the struct enclosing a list
 * 
 * \param list    pointer to a struct list
 * \param type    type of the enclosing struct
 * \param member  name of the struct list member in type
 */
#define list_entry(list, type, member) container_of(list, type, member)

#define list_first_entry(head, type, member)    \
        container_of((head)->next, type, member)

#define list_last_entry(head, type, member)    \
        container_of((head)->prev, type, member)

#define list_next_entry(itter, type, member)    \
        container_of((itter)->member.next, type, member)

#define list_prev_entry(itter, type, member)    \
        container_of((itter)->member.prev, type, member)

/**
 * \brief itterate over every element in a list.
 *
 * \param head   Pointer to the head of the list.
 * \param list   Pointer to a struct list to use as the loop variable.
 *
 * \detail This itterates over all the struct lists in a list, not the
 * enclosing structures (which is probably what you want -- see
 * list_for_each_entry)
 */
#define list_for_each(head, list)                               \
	for (list = (head)->next; list != (head); list = list->next)

/**
 * \brief itterate over enclosing structure in a list
 *
 * \param head    Pointer to the head of the list to itterate over. Note that
 *                the macro will evaluate this expression multiple times.
 * \param itter   Itterator variable with typeof(itter) == type *
 * \param type    The type of the enclosing struct.
 * \param member  The name of the struct list member in type.
 */
#define list_for_each_entry(head, itter, type, member)          \
        for (itter = list_entry((head)->first, type, member);   \
             &itter->member != (head);                          \
             itter = list_next_entry(itter, type, member))

/**
 * \brief Same as list_for_each_entry, but memory safe. i.e. you can safely
 * release any memory assicated with the itterator varaible within the loop.
 *
 * \param head    Pointer to the head of the list to itterate over. Note that
 *                the macro will evaluate this expression multiple times.
 * \param itter   Itterator variable with typeof(itter) == type *
 * \param type    The type of the enclosing struct.
 * \param member  The name of the struct list member in type.
 * \param tmp     Temporary varaible of the same type as itter.
 */
#define list_for_each_entry_safe(head, itter, type, member, tmp)        \
        for (itter = list_entry((head)->first, type, member),           \
                             tmp = list_next_entry(itter, type, member);\
             &itter->member != (head);                                  \
             itter = tmp, tmp = list_next_entry(tmp, type, member))
             

#endif /* STRUCT_LIST_H */
