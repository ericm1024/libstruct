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
 * \file rb_tree.c
 *
 * \author Eric Mueller
 * 
 * \brief Implementation of a red-black tree.
 */

#include "rbtree.h"
#include "bitops.h"
#include <stdint.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>

/*
 * macros:
 *   constant-like:
 *     LEFT
 *     RIGHT
 *     BLACK
 *     RED
 *   function-like:
 *     IS_RED
 *     IS_BLACK
 *     MAKE_RED
 *     MAKE_BLACK
 *     SET_PARENT
 *     GET_PARENT
 */ 

/* DO NOT CHANGE THSE. CODE RELIES ON THEM BEING EXACTLY AS THEY ARE */
#define LEFT (0UL)
#define RIGHT (1UL)
#define BLACK (0UL)
#define RED (1UL)

#define IS_RED(n) ((uintptr_t)(n)->parent & 1UL)
#define IS_BLACK(n) (!((uintptr_t)(n)->parent & 1UL))
#define MAKE_RED(n) \
        (n)->parent = (rb_node_t*)((uintptr_t)(n)->parent | 1UL)
#define MAKE_BLACK(n) \
	(n)->parent = (rb_node_t*)((uintptr_t)(n)->parent & ~1UL)
#define SET_PARENT(n, p) \
	(n)->parent = (rb_node_t*)((uintptr_t)(p) \
				   | ((uintptr_t)(n)->parent & 1UL))
#define GET_PARENT(n) ((rb_node_t*)((uintptr_t)(n)->parent & ~1UL))
#define GET_COLOR(n) ((unsigned long)((uintptr_t)(n)->parent & 1UL))

static inline void link_parent_child(rb_head_t *hd,
				     rb_node_t *parent,
				     rb_node_t *child,
				     unsigned long dir)
{
	if (parent) {
		parent->chld[dir] = child;
	} else {
		hd->root = child;
	}
	if (child)
		SET_PARENT(child, parent);
}

static inline void make_black(rb_node_t *n)
{
	if (n)
		MAKE_BLACK(n);
}

static inline unsigned long is_black(rb_node_t *n)
{
	return !n || IS_BLACK(n);
}

static inline unsigned long is_red(rb_node_t *n)
{
	return n && IS_RED(n);
}

static inline void set_color(rb_node_t *n, unsigned long color)
{
	n->parent = (rb_node_t*)((uintptr_t)n->parent & ~1UL);
	n->parent = (rb_node_t*)((uintptr_t)n->parent | (color & 1));
}

/*
 * helpers:
 *     cmp_to_index
 *     data_to_node
 *     node_to_data
 *     rb_cradle
 *     rb_swap
 *     rb_rotate_single
 *     rb_rotate_double
 *     rb_inorder_sucessor
 */

static inline unsigned long cmp_to_index(long cmp)
{
	/*
	 * fuck branches. all we need is the negation of the sign bit.
	 * negative --> less --> left child --> return 0
	 * positive --> greater --> right child --> return 1
         */
	return (~cmp >> (sizeof(cmp) * CHAR_BIT - 1)) & 1L;
}

static inline rb_node_t *data_to_node(rb_head_t *hd, void *data)
{
	return (rb_node_t*)((uintptr_t)data + hd->offset);
}

static inline void *node_to_data(rb_head_t *hd, rb_node_t *n)
{
	return (void *)((uintptr_t)n - hd->offset);
}

static inline rb_node_t *init_node(rb_node_t *n)
{
	n->parent = NULL;
	n->chld[LEFT] = NULL;
	n->chld[RIGHT] = NULL;
	MAKE_RED(n);
	return n;
}

/*
 * get the index of a node in its parent.
 */
static inline unsigned long get_cradle(rb_node_t *n)
{
	rb_node_t *p = GET_PARENT(n);
	return p && p->chld[1] == n;
}

/*
 * swap two nodes
 * assumptions/preconditions:
 *     - @high is higher up in the tree than low
 *     - @high has two non-null children
 */ 
static void rb_swap(rb_head_t *hd, rb_node_t *high, rb_node_t *low)
{	
	rb_node_t *tmp;
	unsigned long right;
	unsigned long left;

	/* convenience macro to make this function prettier */
#define __RB_SWAP(a, b) \
	do {		\
		tmp = a;			\
		a = b;				\
		b = a;				\
	} while (0);
	
	
	if (GET_PARENT(high)) {
		right = get_cradle(high);
		tmp = GET_PARENT(high);
		tmp->chld[right] = low;
	} else {
		hd->root = low;
		SET_PARENT(low, NULL);
	}

	/* special case where low is high's child */
	if (low == high->chld[RIGHT]
	    || low == high->chld[LEFT]) {
		right = get_cradle(low);
		left = 1 - right;

                /* swap shared links */
		low->parent = high->parent;
		high->parent = low;
		high->chld[right] = low->chld[right];
		low->chld[right] = high;
		
		/* swap other child */
		__RB_SWAP(high->chld[left], low->chld[left]);
	} else {
		right = get_cradle(low);
		tmp = GET_PARENT(low);
		tmp->chld[right] = high;

		/* swap everything */
		__RB_SWAP(high->parent, low->parent);
		__RB_SWAP(high->chld[RIGHT], low->chld[RIGHT]);
		__RB_SWAP(high->chld[LEFT], low->chld[LEFT]);
	}

	if (high->chld[RIGHT])
		SET_PARENT(high->chld[RIGHT], high);
	if (high->chld[LEFT])
		SET_PARENT(high->chld[LEFT], high);
	
	SET_PARENT(low->chld[RIGHT], low);
	SET_PARENT(low->chld[LEFT], low);
}

/*
 *      d           b
 *     / \         / \
 *    b   E  -->  A   d
 *   / \             / \
 *  A   C           C   E
 */ 
static rb_node_t *rb_rotate_single(rb_head_t *hd,
				   rb_node_t *root,
				   unsigned long right)
{
	printf("single rotate.\n");
	unsigned long left = 1 - right;
	rb_node_t *child = root->chld[left];
	rb_node_t *parent = GET_PARENT(root);

	/* update root and child's parents */
	SET_PARENT(child, parent);
	SET_PARENT(root, child);
	if (parent)
		parent->chld[get_cradle(root)] = child;
	else
		hd->root = child;
	
	/* update root's left child */
	root->chld[left] = child->chld[right];
	if (root->chld[left])
		SET_PARENT(root->chld[left], root);

	/* make root child's child */
	child->chld[right] = root;

	/* fix up colors */
	MAKE_RED(root);
	MAKE_BLACK(child);
	return child;
}

/*
 *     f               d
 *    / \            /   \
 *   b   G          b     f
 *  / \      -->   / \   / \
 * A   d          A   C E   G
 *    / \
 *   C   E
 */ 
static rb_node_t *rb_rotate_double(rb_head_t *hd,
				   rb_node_t *root,
				   unsigned long right)
{
	printf("double rotate.\n");
	unsigned long left = 1 - right;
	rb_rotate_single(hd, root->chld[left], left);
	return rb_rotate_single(hd, root, right);
}

static inline rb_node_t *closest_child(rb_node_t *n, unsigned long right)
{
	rb_node_t *i;
	unsigned long left = 1 - right;
	for (i = n->chld[right]; i->chld[left]; i = i->chld[left])
		;
	return i;
}

void rb_insert(rb_head_t *hd, void *new)
{
	rb_node_t *n = init_node(data_to_node(hd, new));
	rb_node_t *path = hd->root;
	rb_node_t *aunt;
	rb_node_t *gparent;
	long cmp;
	unsigned long i;
	unsigned long stack = 0; /* bit stack of directions we traversed
				  * as we traverse down */
	
	/* empty tree */
	if (!path) {
		MAKE_BLACK(n);
		hd->root = n;
		hd->nnodes++;
		return;
	}

	/* search for the place to insert */
	for (;;) {
		cmp = hd->cmp(new, node_to_data(hd, path));
		/* duplicate entry */
		if (cmp == 0)
			return;
		
		i = cmp_to_index(cmp);
		/* push the direction we're about to traverse onto stack */
		stack <<= 1;
		stack |= i;

		/* found an emty slot, we're done */
		if (!path->chld[i])
			break;
		path = path->chld[i];
	}
	
	/* do the insertion */
	SET_PARENT(n, path);
	path->chld[i] = n;
	hd->nnodes++;

	/* rebalance */
	while(path && IS_RED(path)) {
		printf("insert: rebalancing.\n");
		gparent = GET_PARENT(path);
		if (!gparent)
			break;
		aunt = gparent->chld[RIGHT] == path
			? gparent->chld[LEFT]
			: gparent->chld[RIGHT];
		
		if (is_black(aunt)) { /* inserted into 3 node */
			/* last 2 traversed directions were oposites */
			if ((stack & 1) ^ ((stack >> 1) & 1))
				rb_rotate_double(hd, gparent, stack & 1);
			else
				rb_rotate_single(hd, gparent, ~stack & 1);
			break;
		} else { /* inserted into 4 node */
			MAKE_BLACK(path);
			MAKE_BLACK(aunt);
			MAKE_RED(gparent);
			path = GET_PARENT(gparent);
		}
		/* we move up two nodes on the tree each time, so
		 * 'pop' the last two directions off the stack*/
		stack >>= 2;
	}
	MAKE_BLACK(hd->root);
}

void rb_erase(rb_head_t *hd, void *victim)
{
	rb_node_t *n = data_to_node(hd, victim);
	rb_node_t *child;
	rb_node_t *parent;
	rb_node_t *sibling;
	rb_node_t *rniece;
	rb_node_t *lniece;
	unsigned long left;
	unsigned long right;
	unsigned long color;
	
	hd->nnodes--;
		
	/* if victim is a body node, find sucessor and swap */
	if (n->chld[RIGHT] && n->chld[LEFT])
		rb_swap(hd, n, closest_child(n, RIGHT));
	
	/* replace n with its only child */
	child = n->chld[n->chld[LEFT] ? LEFT : RIGHT];
	parent = GET_PARENT(n);
	link_parent_child(hd, parent, child, get_cradle(n));

	/* If we don't have a double black, we're done */
	if (!(is_black(n) && is_black(child))) {
		make_black(child);
		return;
	}

	/* else color it black and prepare for some fun... */
	make_black(child);
	for (;;) {
		right = parent->chld[RIGHT] == child ? RIGHT : LEFT;
		left = 1 - right;
		
		/* case reduction */
		if (IS_RED(parent->chld[left]))
			parent = rb_rotate_single(hd, parent, right);
		
		sibling = parent->chld[left];
		rniece = sibling->chld[right];
		lniece = sibling->chld[left];

		if (IS_BLACK(rniece) && IS_BLACK(lniece)) {
			MAKE_RED(sibling);
			if (IS_RED(parent)) {
				MAKE_BLACK(parent);
				return;
			}
		} else {
			color = GET_COLOR(parent);
			if (IS_RED(lniece))
				parent = rb_rotate_single(hd, parent, right);
			else
				parent = rb_rotate_double(hd, parent, right);
			set_color(parent, color);
			return;
		}
		parent = GET_PARENT(parent);
	}
	make_black(hd->root);
}

void *rb_find(rb_head_t *hd, void *findee)
{
	rb_node_t *n = hd->root;
	long cmp;
	if (!findee)
		return NULL;
	
	while(n) {
		cmp = hd->cmp(findee, node_to_data(hd, n));
		if (cmp == 0)
			return node_to_data(hd, n);
		n = n->chld[cmp_to_index(cmp)];
	}
	return NULL;
}
