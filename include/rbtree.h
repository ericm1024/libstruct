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
 *
 * \detail This is a fairly standard red-black tree. red-black trees are
 * described in great detail in most data strutures textbooks and on Wikipedia,
 * so I won't decribe them in full. In essence they are a binary tree that
 * maintains relatively strict balance by 'coloring' the nodes of the tree
 * and enforcing a set of rules based on those colors. The rules are such that
 * the height of the tree is bounded by 2*log(n + 1). This ensures that inert,
 * erase, and query operations can be performed in O(log n) time in the worst
 * case.
 *
 * This implementation is meant to be used as a structure member and not a
 * container. In other words, if you want your struct foo to be organized in a
 * red-black tree, add a field of type rb_node_t to your struct declaration, ex:
 *
 *   struct foo {
 *             .
 *             .
 *           rb_node_t rb_link;
 *             .
 *             .
 *   };
 *
 * Then declare a red-black tree with the RB_TREE macro, ex:
 *
 *     RB_TREE(foo_tree, lt_op, foo, avl_link);
 *
 * The requirements of the less than operator (lt_op) are described in the
 * macro's doccumentation.
 *
 * At this point, the tree can be modified with calls to rb_insert and
 * rb_delete, queried with calls to rb_find, and traversed in order with
 * rb_first, rb_last, rb_next, and rb_prev. The convenience macros rb_for_each
 * and rb_for_each_range can be used to iterate over an entire tree or some
 * portion of the tree. Finally, the tree can be traversed in postorder
 * fasion using the rb_postorder_iterate function.
 *
 * It's worth noting that none of these functions allocate memory.
 *
 * Synchronization is the job of the caller.
 */

#ifndef STRUCT_RBTREE_H
#define STRUCT_RBTREE_H 1

#include <stddef.h>

typedef struct rb_node {
	struct rb_node *parent;
	struct rb_node *chld[2];
} rb_node_t;

typedef long (*rb_cmp_t)(void *lhs, void *rhs);

typedef struct rb_head {
	rb_node_t *root;
	const size_t offset;
	/* offset of rb_nodes in enclosing structs */
	rb_cmp_t cmp;
	size_t nnodes;
	/* number of nodes in the tree */
} rb_head_t;

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
#define RB_TREE(name, lt, container, member)				\
	rb_head_t name = {						\
		.root = NULL,						\
		.offset = offsetof(container, member),			\
		.cmp = (rb_cmp_t)lt,					\
		.nnodes = 0};

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
 * \param hd     Head of the tree containing start.
 * \return The element imediately after start.
 */
extern void *rb_inorder_next(rb_head_t *hd, void *start);

/**
 * \brief Get the in order previous element in a tree.
 * 
 * \param start  The element to start at.
 * \param hd     Head of the tree containing start.
 * \return The element imediately before start.
 */
extern void *rb_inorder_prev(rb_head_t *hd, void *start);

/**
 * \brief Get the next element in the tree in postorder.
 * \param hd  Head of the tree.
 * \param f   Function to apply to each structure in the tree in postorder
 * fashion.
 *
 * \detail Postorder iteration entails visiting each child node before
 * its parent. This is generally only used for safely freeing memory
 * associated With every node in the tree.
 */ 
extern void rb_postorder_iterate(rb_head_t *hd, void(*f)(void *));

/**
 * Loop over the elements in a tree in order.
 *
 * \param head       Head of the tree to iterate over.
 * \param type       (token) Type of the iterator to declare (type of the
 *                   enclosing struct, not a pointer type).
 * \param iter_name  (token) Name of the iterator to declare (use this in
 *                   your loop). The macro declares a variable of type @type
 *                   with this name. Don't declare one yourself.
 *                   
 */
#define rb_for_each_inorder(head, type, iter_name)			\
	for (type *iter_name = (type*)rb_first(head); iter_name;        \
	     iter_name = (type*)rb_inorder_next(head, iter_name))
	
/**
 * Loop over the elements in the tree in the range [first, last)
 *
 * \param head       Head of the tree to iterate over.
 * \param type       (token) Type of the iterator to declare (type of the
 *                   enclosing struct, not a pointer type).
 * \param iter_name  (token) Name of the iterator to declare (use this in
 *                   your loop). The macro declares a variable of type @type
 *                   with this name. Don't declare one yourself.
 * \param first      (pointer) First element in the tree to apply the
 *                   function to.
 * \param last       (pointer) The node after the last node to apply the
 *                   function to. Matching is determined by the return value
 *                   of the tree's cmp function.
 */
#define rb_for_each_range(head, type, iter_name, first, last)		\
	for (type *iter_name = first; 					 \
	     (head)->cmp((void*)iter_name, (void*)(last)) != 0;		 \
	     iter_name = (type*)rb_inorder_next(head, (void*)iter_name))

#endif /* STRUCT_RBTREE_H */
