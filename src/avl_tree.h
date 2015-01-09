/**
 * \file avl_tree.h
 *
 * \author Eric Mueller
 * 
 * \brief Header file for an avl tree.
 */

#ifndef STRUCT_AVL_TREE_H
#define STRUCT_AVL_TREE_H 1

#include <stddef.h>
#include "avl_tree_types.h"

/**
 * \brief Declare an avl tree head.
 *
 * \param name       (token) The name of the avl_head to declare.
 * \param lt         (function pointer) The less than operator for the tree. This
 *                   function is expected to return -1 when lhs < rhs, 0 when
 *                   lhs == rhs, and 1 when lhs > rhs.
 * \param container  (type) Type of the enclosing container.
 * \param member     (token) Name of the avl_node_t member in container.
 */
#define AVL_TREE(name, lt, container, member)                             \
	avl_head_t name = {NULL, 0, (avl_cmp_t)lt, offsetof(container, member)}

/**
 * Insert an element into the tree.
 *
 * \param hd        Pointer to the head of the tree.
 * \param insertee  Pointer to the node to insert.
 */
extern void avl_insert(avl_head_t *hd, void *insertee);

/**
 * Find an return an element matching the given element.
 *
 * \param hd  
 * \param findee  Pointer to an element matching the elememt to find.
 * \return Pointer to the node being looked for, or NULL if no such node was
 *         found.
 */
extern void *avl_find(avl_head_t *hd, void *findee);

/**
 * Delete an element from the tree.
 *
 * \param hd      Pointer to the head of the tree
 * \param victim  Pointer to the node to remove.
 */
extern void avl_delete(avl_head_t *hd, void *victim);

/**
 * Get the next (in order) element in the tree.
 *
 * \param elem  Pointer to the element before the desired element.
 * \return Pointer to the next element in the tree.
 */
extern void *avl_next(avl_head_t *hd, void *elem);

/**
 * Get the previous (in order) element in the tree.
 *
 * \param elem  Pointer to the element after the desired element.
 * \return Pointer to the next element in the tree.
 */
extern void *avl_prev(avl_head_t *hd, void *elem);

/**
 * Get the in-order first element in the tree.
 *
 * \param hd  Pointer to the head of the tree.
 * \return Pointer to the first element in the tree.
 */
extern void *avl_first(avl_head_t *hd);

/**
 * Get the in order last element in the tree.
 *
 * \param hd  Pointer to the head of the tree.
 * \return Pointer to the last element in the tree.
 */
extern void *avl_first(avl_head_t *hd);

/**
 * Insert all the elements of another avl tree into the avl tree in hd. This
 * invalidates splicee. Specifically, splicee's members are nulled out.
 *
 * \param hd       Pointer to the avl tree to insert into.
 * \param splicee  Pointer to the avl tree to insert.
 */
extern void avl_splice(avl_head_t *hd, avl_head_t *splicee);

/**
 * Loop over the elements in the tree, in order.
 *
 * \param head       Pointer to the head of the tree to apply the function to.
 * \param iter_name  Name of the loop variable. Raw token. A variable of type
 *                   void * with this name is declared by this macro (not by the
 *                   caller.
 */
#define avl_for_each(head, iter_name)                           \
	for (void *iter_name = avl_first(head); iter_name;	\
	     iter_name = avl_next(head, iter_name))

/**
 * Loop over the elements in the tree in the range [first, last)
 *
 * \param head    Pointer to the head of the tree to apply the function to.
 * \param first   Pointer to the first element in the tree to apply the
 *                function to.
 * \param last    Pointer to the node after the last node to apply the
 *                function to. Matching is determined by the return value of
 *                the tree's acl_cmp_t function.
 */
#define avl_for_each_range(head, iter_name, first, last)                  \
	for (void *iter_name = first; (head)->cmp(iter_name, last) != 0;  \
	     iter_name = avl_next(head, iter_name))
	      

#endif /* STRUCT_AVL_TREE_H */
