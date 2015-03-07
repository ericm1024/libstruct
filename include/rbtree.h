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
 * \file rbtree.h 
 *
 * \author Eric Mueller
 * 
 * \brief Header file for a red black tree.
 */

#ifndef STRUCT_RBTREE_H
#define STRUCT_RBTREE_H 1

#include "libstruct.h"

/**
 * \brief Declare a red-black tree head.
 *
 * \param name       (token) The name of the rb_head_t to declare.
 * \param lt         (function pointer) The less than operator for the tree.
 *                   This function should return < 0 when lhs < rhs, 0 when
 *                   lhs == rhs, and > 0 when lhs > rhs.
 * \param container  (type) Type of the enclosing structure.
 * \param member     (token) name of the rb_node_t member in container.
 */
#define RB_TREE(name, lt, container, member) \
	rb_head_t name = {NULL, offsetof(container, member), (rb_cmp_t)lt, 0};

/**
 * \brief Insert an element into a tree.
 *
 * \param hd   Head of the tree.
 * \param new  Element to insert.
 */
extern void rb_insert(rb_head_t *hd, void *new);

/**
 * \brief Remove an element from a tree.
 *
 * \param hd      Head of the tree.
 * \param victim  Element to remove.
 */
extern void rb_erase(rb_head_t *hd, void *victim);

/**
 * \brief Look for an element from a tree.
 *
 * \param hd      Head of the tree.
 * \param findee  Element matching the element to find.
 * \return Element matching findee, or NULL if no such element exists.
 */
extern void *rb_find(rb_head_t *hd, void *findee);

/**
 * \brief Get the in order first element in a tree.
 * 
 * \param hd  Head of the tree.
 * \return The first element in the tree.
 */
extern void *rb_first(rb_head_t *hd);

/**
 * \brief Get the in order last element in the tree.
 *
 * \param hd  Head of the tree.
 * \return The last element in the tree.
 */
extern void *rb_last(rb_head_t *hd);

/**
 * \brief Get the in order next element in a tree.
 * 
 * \param start  The element to start at.
 * \return The element imediately after start.
 */
extern void *rb_inorder_next(void *start);

/**
 * \brief Get the in order previous element in a tree.
 * 
 * \param start  The element to start at.
 * \return The element imediately before start.
 */
extern void *rb_inorder_prev(void *start);

/* TODO: postorder next */
/* TODO: postorder previous */
/* TODO: for each macro */
/* TODO: for each range macro */

#endif /* STRUCT_RBTREE_H */
