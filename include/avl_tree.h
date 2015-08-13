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
 * \file avl_tree.h
 *
 * \author Eric Mueller
 * 
 * \brief Header file for an avl tree.
 *
 * \detail This is a fairly standard avl tree. avl trees are described in
 * great detail in most data strutures textbooks and on Wikipedia, so I won't
 * decribe them in full. In essence they are a binary tree that maintains
 * strict balance by requiring that the difference in height between the left
 * and right subtree of any node be at most 1. This ensures that inert, erase,
 * and query operations can be performed in O(log n) time in the worst case.
 *
 * This implementation is meant to be used as a structure member and not a
 * container. In other words, if you want your struct foo to be organized in an
 * avl tree, add a field of type struct avl_node to your struct declaration, ex:
 *
 *   struct foo {
 *             .
 *             .
 *           struct avl_node avl_link;
 *             .
 *             .
 *   };
 *
 * Then declare an avl tree with the AVL_TREE macro, ex:
 *
 *     AVL_TREE(foo_tree, lt_op, foo, avl_link);
 *
 * The requirements of the less than operator (lt_op) are described in the
 * macro's doccumentation.
 *
 * At this point, the tree can be modified with calls to avl_insert and
 * avl_delete, queried with calls to avl_find, and traversed in order with
 * avl_first, avl_last, avl_next, and avl_prev. Finally, the convenience
 * macros avl_for_each and avl_for_each_range can be used to iterate over an
 * entire tree or some portion of the tree.
 * 
 * It's worth noting that none of these functions here allocate memory and
 * none of them try to aquire or otherwise use locks in any way. Thread
 * safety is the job of the caller.
 */

#ifndef STRUCT_AVL_TREE_H
#define STRUCT_AVL_TREE_H 1

#include <stddef.h>

/** avl tree node */
struct avl_node {
        /** parent node */
	struct avl_node *parent;

        /** 0 is left child, 1 is right child */
	struct avl_node *children[2];

        /** balance factor. -1, 0, or 1 */
	short balance;

        /** where am I in my parent? (0 or 1) */
	unsigned short cradle;
};

typedef int (*avl_cmp_t)(void *lhs, void *rhs);

/** metadata/head structure for avl tree */
struct avl_head {
        /* pointer to the root node */
	struct avl_node *root;

        /* number of nodes in the tree */
	size_t n_nodes;

        /* less than comparator */
	avl_cmp_t cmp;

        /** offset of the avl node in the enclosing struct */
	const size_t offset;
};

/**
 * \brief Declare an avl tree head.
 *
 * \param name       (token) The name of the avl_head to declare.
 * \param lt         (function pointer) The less than operator for the tree. This
 *                   function is expected to return -1 when lhs < rhs, 0 when
 *                   lhs == rhs, and 1 when lhs > rhs.
 * \param container  (type) Type of the enclosing container.
 * \param member     (token) Name of the struct avl_node member in container.
 */
#define AVL_TREE(name, lt, container, member)				\
	struct avl_head name = {                                        \
		.root = NULL,						\
		.n_nodes = 0,						\
		.cmp = (avl_cmp_t)(lt),					\
		.offset = offsetof(container, member)};

/**
 * Insert an element into the tree.
 *
 * \param hd        Pointer to the head of the tree.
 * \param insertee  Pointer to the node to insert.
 */
void avl_insert(struct avl_head *hd, void *insertee);

/**
 * Find an return an element matching the given element.
 *
 * \param hd  
 * \param findee  Pointer to an element matching the elememt to find.
 * \return Pointer to the node being looked for, or NULL if no such node was
 *         found.
 */
void *avl_find(struct avl_head *hd, void *findee);

/**
 * Delete an element from the tree.
 *
 * \param hd      Pointer to the head of the tree
 * \param victim  Pointer to the node to remove.
 */
void avl_delete(struct avl_head *hd, void *victim);

/**
 * Get the next (in order) element in the tree.
 *
 * \param elem  Pointer to the element before the desired element.
 * \return Pointer to the next element in the tree.
 */
void *avl_next(struct avl_head *hd, void *elem);

/**
 * Get the previous (in order) element in the tree.
 *
 * \param elem  Pointer to the element after the desired element.
 * \return Pointer to the next element in the tree.
 */
void *avl_prev(struct avl_head *hd, void *elem);

/**
 * Get the in-order first element in the tree.
 *
 * \param hd  Pointer to the head of the tree.
 * \return Pointer to the first element in the tree.
 */
void *avl_first(struct avl_head *hd);

/**
 * Get the in order last element in the tree.
 *
 * \param hd  Pointer to the head of the tree.
 * \return Pointer to the last element in the tree.
 */
void *avl_last(struct avl_head* hd);

/**
 * Insert all the elements of another avl tree into the avl tree in hd. This
 * invalidates splicee. Specifically, splicee's members are nulled out.
 *
 * \param hd       Pointer to the avl tree to insert into.
 * \param splicee  Pointer to the avl tree to insert.
 */
void avl_splice(struct avl_head *hd, struct avl_head *splicee);

/**
 * Loop over the elements in the tree, in order.
 *
 * \param head       Pointer to the head of the tree to apply the function to.
 * \param iter_name  Name of the loop variable. Raw token. A variable of type
 *                   void * with this name is declared by this macro (not by the
 *                   caller).
 */
#define avl_for_each(head, iter_name)                           \
	for (void *iter_name = avl_first(head); iter_name;	\
	     iter_name = avl_next(head, iter_name))

/**
 * Loop over the elements in the tree in the range [first, last)
 *
 * \param head       Pointer to the head of the tree to apply the function to.
 * \param iter_name  Name of the loop variable. Raw token. A variable of type
 *                   void * with this name is declared by this macro (not by the
 *                   caller).
 * \param first      Pointer to the first element in the tree to apply the
 *                   function to.
 * \param last       Pointer to the node after the last node to apply the
 *                   function to. Matching is determined by the return value of
 *                   the tree's acl_cmp_t function.
 */
#define avl_for_each_range(head, iter_name, first, last)                  \
	for (void *iter_name = first; (head)->cmp(iter_name, last) != 0;  \
	     iter_name = avl_next(head, iter_name))

#endif /* STRUCT_AVL_TREE_H */
