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
 * \file avl_tree.c
 *
 * \author Eric Mueller
 * 
 * \brief Implementation of an avl tree.
 */

#include "avl_tree.h"
#include "bitops.h"
#include "util.h"

#include <stddef.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

/* used as indices for children */
#define AVL_LEFT ((unsigned short)0)
#define AVL_RIGHT ((unsigned short)1)

/* used for balance factors */
#define LEFT_OVERWEIGHT ((short)-2)
#define LEFT_HEAVY ((short)-1)
#define BALANCED ((short)0)
#define RIGHT_HEAVY ((short)1)
#define RIGHT_OVERWEIGHT ((short)2)

/*
 * setters and getters for all the sh*t in struct avl_node. We have these
 * because if we want to change the way anything is stored (i.e. if we decide
 * we want to store the balance factor in the parent pointer, if we even want
 * to store a parent pointer, etc). Liberal use of these functions mean that
 * changing such low level implementation details is easy.
 */

static void set_parent(struct avl_node *node, struct avl_node *parent)
{
	node->parent = parent;
}

static void set_balance(struct avl_node *node, short bf)
{
	node->balance = bf;
}

static struct avl_node *get_parent(struct avl_node *node)
{
	return node->parent;
}

static short get_balance(struct avl_node *node)
{
	return node->balance;
}

/*
 * Various other helper functions
 */

static struct avl_node *closest_child(struct avl_node *n,
					unsigned short right)
{
	struct avl_node *child;
	unsigned short left = 1 - right;
#ifdef DEBUG
	assert(right == AVL_RIGHT || right == AVL_LEFT);
#endif	
	if (!n)
		return NULL;
	
        child = n->children[right];
	while (child && child->children[left])
		child = child->children[left];
	return child;
}

static unsigned short child_index(struct avl_node *child)
{
        struct avl_node *parent = get_parent(child);
        return !parent || parent->children[AVL_LEFT] == child ? AVL_LEFT : AVL_RIGHT;
}

static short dir_to_bf(unsigned short right)
{
	return  right == AVL_RIGHT ? RIGHT_HEAVY : LEFT_HEAVY;
}

/*
 * Time for some fun... rotate, with almost no branching.
 *
 *      d            b
 *     / \          / \
 *    b   E  --->  A   d
 *   / \              / \
 *  A   C            C   E
 *
 */
static struct avl_node *rotate_single(struct avl_node *root, unsigned short right)
{
	unsigned short left = 1 - right;
	struct avl_node *b = root->children[left];
	struct avl_node *c = b->children[right];
	struct avl_node *p = get_parent(root);
        short new_bf;

#ifdef DEBUG
	assert(right == AVL_RIGHT || right == AVL_LEFT);
	assert((get_balance(root) == 1 && get_balance(b) != -1)
		|| (get_balance(root) == -1 && get_balance(b) != 1));
#endif

	/* update b (and p) */
	if (p)
		p->children[child_index(root)] = b;
	set_parent(b, get_parent(root));
	b->children[right] = root;

	/*
	 * Table for new bf
	 * 
	 *  dir   bf(b)   new_bf
	 *  --------------------
	 *    1 |  -1  |   0
	 *    1 |   0  |   1
	 *   -1 |   1  |   0
	 *   -1 |   0  |  -1
	 *
	 * Other cases are handled by rotate_double
	 */ 
	new_bf = dir_to_bf(right) * (~get_balance(b) & 1);
#ifdef DEBUG
	if (right == AVL_RIGHT) {
		if (get_balance(b) == LEFT_HEAVY)
			assert(new_bf == BALANCED);
		else
			assert(new_bf == RIGHT_HEAVY);
	} else {
		if (get_balance(b) == RIGHT_HEAVY)
			assert(new_bf == BALANCED);
		else
			assert(new_bf == LEFT_HEAVY);
	}
#endif
	set_balance(b, new_bf);
	                             // TODO: ditch the imul

	/* update d (root) */
	set_parent(root, b);
	set_balance(root, -get_balance(b));
	root->children[left] = c;

	/* update c */
	if (c)
		set_parent(c, root);

	return b; /* return the new root */
}

static struct avl_node *rotate_double(struct avl_node *root, unsigned short right)
{
        unsigned short left = 1 - right;
	struct avl_node *d = root->children[left]->children[right];
	struct avl_node *c = d->children[left];
	struct avl_node *e = d->children[right];
	struct avl_node *p = get_parent(root);
	short new_bf, bal = get_balance(d);

#ifdef DEBUG
	assert(
		(get_balance(root) == 1 && get_balance(root->children[left]) == -1)
		|| (get_balance(root) == -1 && get_balance(root->children[left]) == 1));
#endif

	/* update d (and p) */
	if (p)
		p->children[child_index(root)] = d;
	d->children[right] = root;
	d->children[left] = get_parent(d);
	set_balance(d, 0);
	set_parent(d, get_parent(root));

	/* update b */
	d->children[left]->children[right] = c;
	set_parent(d->children[left], d);

	/*
         *   dir   bf(d)   new bf(b)   -((dir + bf(d) >> 1) & bf(d))
         *  ----------------------------------
         *    1  |  -1   |    0     |    0
         *    1  |   0   |    0     |    0
         *    1  |   1   |   -1     |   -1
         *   -1  |  -1   |    1     |    1
         *   -1  |   0   |    0     |    0
         *   -1  |   1   |    0     |    0
         */
	new_bf = -(((dir_to_bf(right) + bal) >> 1) & bal);
#ifdef DEBUG
	if (right == AVL_RIGHT) {
		if (bal == RIGHT_HEAVY)
			assert(new_bf == LEFT_HEAVY);
		else
			assert(new_bf == BALANCED);
	} else {
		assert(right == AVL_LEFT);
		if (bal == LEFT_HEAVY)
			assert(new_bf == RIGHT_HEAVY);
		else
			assert(new_bf == BALANCED);
        }
#endif
	set_balance(d->children[left], new_bf);

	/* update C */
	if (c)
		set_parent(c, d->children[left]);

	/* update E */
	if (e)
		set_parent(e, d->children[right]);

	/* update f (root) */
	root->children[left] = e;
	set_parent(root, d);

	/*
         *   dir   bf(d)   new bf(f)   (dir - bf(d) >> 1) & -bf(d)
         *  -----------------------------------
         *    1  |  -1   |    1     |    1
         *    1  |   0   |    0     |    0
         *    1  |   1   |    0     |    0
         *   -1  |  -1   |    0     |    0
         *   -1  |   0   |    0     |    0
         *   -1  |   1   |   -1     |   -1
         */
	new_bf = ((dir_to_bf(right) - bal) >> 1) & -bal;
#ifdef DEBUG
	if (right == AVL_RIGHT) {
		if (bal == LEFT_HEAVY)
			assert(new_bf == RIGHT_HEAVY);
		else
			assert(new_bf == BALANCED);
	} else {
		assert(right == AVL_LEFT);
		if (bal == RIGHT_HEAVY)
			assert(new_bf == LEFT_HEAVY);
		else
			assert(new_bf == BALANCED);
	}
#endif
	set_balance(root, new_bf);

	return d;
	/* return the new root */
}

static struct avl_node *rotate(struct avl_head *hd, struct avl_node *root,
				 unsigned short right)
{
	struct avl_node *new_root = get_balance(root->children[1 - right])
		== dir_to_bf(right) ? rotate_double(root, right)
		                    : rotate_single(root, right);
	if (root == hd->root)
		hd->root = new_root;
	return new_root;
}

static void sanitize_node(struct avl_node *node)
{
        node->children[AVL_RIGHT] = NULL;
	node->children[AVL_LEFT] = NULL;
	set_parent(node, NULL);
	set_balance(node, 0);
}

/*
 * The idea is we first traverse to an empty leaf slot and add the node there, then
 * walk back up the tree, adjusting balance factors as necessary. If we hit a node
 * that's leaning too far to the right or the left, rotate about it in the opposite
 * direction.
 */
void avl_insert(struct avl_head *hd, struct avl_node *insertee)
{
	struct avl_node *child, *parent = hd->root;
        unsigned short right;

        if (!insertee)
                return;

        sanitize_node(insertee);

        hd->n_nodes++;
        if (!parent) {
                hd->root = insertee;
        } else {
                for (;;) {
                        int cmp = hd->cmp(insertee, parent);
                        right = cmp < 0 ? AVL_LEFT : AVL_RIGHT;
                        if (!parent->children[right])
                                break;
                        parent = parent->children[right];
                }
                parent->children[right] = insertee;
        }
        set_parent(insertee, parent);

	/* traverse back until we hit the node in need of rebalancing */
	child = insertee;
	while (parent) {
                short bal = get_balance(parent);
		right = child_index(child);
		bal += right == AVL_RIGHT ? 1 : -1;

		if (bal == RIGHT_OVERWEIGHT || bal == LEFT_OVERWEIGHT) {
			rotate(hd, parent, 1 - right);
			return;
		}

		set_balance(parent, bal);
		if (bal == BALANCED)
			return;

		child = parent;
		parent = get_parent(child);
	}
}

static void link_parent_child(struct avl_head *hd, struct avl_node *parent,
                              struct avl_node *child, unsigned short dir)
{
	if (parent) {
		parent->children[dir] = child;
	} else {
		hd->root = child;
	}
	if (child)
		set_parent(child, parent);
}

/*
 * swap two nodes
 * assumptions/preconditions:
 *     - @high is higher up in the tree than low
 *     - @high has two non-null children
 */
static void avl_swap(struct avl_head *hd, struct avl_node *high,
                     struct avl_node *low)
{
	struct avl_node *parent;
	unsigned short right;
	unsigned short left;

	if (get_parent(high)) {
		right = child_index(high);
		parent = get_parent(high);
		parent->children[right] = low;
	} else {
		hd->root = low;
	}

	/* special case where low is high's child */
	if (low == high->children[AVL_RIGHT]
	    || low == high->children[AVL_LEFT]) {
		right = child_index(low);
		left = 1 - right;

                /* swap shared links */
		low->parent = high->parent;
		high->parent = low;

		high->children[right] = low->children[right];
		low->children[right] = high;

		/* swap other child */
		swap_t(struct avl_node *, high->children[left],
                       low->children[left]);
                swap_t(short, high->balance, low->balance);
	} else {
		right = child_index(low);
		parent = get_parent(low);
		parent->children[right] = high;

		/* swap everything */
		swap_t(struct avl_node, *high, *low);
	}

	if (high->children[AVL_RIGHT])
		set_parent(high->children[AVL_RIGHT], high);
	if (high->children[AVL_LEFT])
		set_parent(high->children[AVL_LEFT], high);
	set_parent(low->children[AVL_RIGHT], low);
	set_parent(low->children[AVL_LEFT], low);
}

void avl_delete(struct avl_head *hd, struct avl_node *victim)
{
	struct avl_node *path, *child;
	unsigned short right;

        if (!victim)
		return;

        hd->n_nodes--;
        if (victim->children[AVL_LEFT] && victim->children[AVL_RIGHT]) {
                child = closest_child(victim, SIGN_BIT(~get_balance(victim)));
                avl_swap(hd, victim, child);
        }

        /* replace n with its only child */
        child = victim->children[victim->children[AVL_LEFT] ?
                                 AVL_LEFT : AVL_RIGHT];
        path = get_parent(victim);
        right = child_index(victim);
        link_parent_child(hd, path, child, right);

        /* walk back up the tree, adjusting balances as we go */
	while (path) {
		short bal = get_balance(path);
                bal += right == AVL_RIGHT ? -1 : 1;

		if (bal == RIGHT_OVERWEIGHT || bal == LEFT_OVERWEIGHT) {
			path = rotate(hd, path, right);
			bal = get_balance(path);
		} else
			set_balance(path, bal);

		if (bal != 0)
			return;

		child = path;
		path = get_parent(path);
		if (!path)
			return;
		right = child_index(child);
	}
}


struct avl_node *avl_find(struct avl_head *hd, struct avl_node *findee)
{
        struct avl_node *n = hd->root;

	if (!findee)
		return NULL;

	while (n) {
		int cmp = hd->cmp(findee, n);
                if (cmp < 0)
			n = n->children[AVL_LEFT];
                else if (cmp > 0)
			n = n->children[AVL_RIGHT];
		else
			return n;
	}
	return NULL;
}

struct avl_node *avl_next(struct avl_node *elem)
{
        struct avl_node *n = elem;
	if (!n)
		return NULL;

	if (n->children[AVL_RIGHT])
		return closest_child(n, AVL_RIGHT);
	else {
		struct avl_node *prev = NULL;
		while (n && prev == n->children[AVL_RIGHT]) {
			prev = n;
			n = get_parent(n);
		}
		return n;
	}
}

struct avl_node *avl_prev(struct avl_node *elem)
{
	if (!elem)
		return NULL;

	if (elem->children[AVL_LEFT])
		return closest_child(elem, AVL_LEFT);
	else {
		struct avl_node *prev = NULL;
		while (elem && prev == elem->children[AVL_LEFT]) {
			prev = elem;
			elem = get_parent(elem);
		}
		return elem;
	}
}

struct avl_node *avl_first(struct avl_head *hd)
{
        struct avl_node *first = hd->root;
	if (first)
                while (first->children[AVL_LEFT])
                        first = first->children[AVL_LEFT];
	return first;
}

struct avl_node *avl_last(struct avl_head *hd)
{
        struct avl_node *last = hd->root;
	if (last)
                while (last->children[AVL_RIGHT])
                        last = last->children[AVL_RIGHT];
	return last;
}

void avl_splice(struct avl_head *hd, struct avl_head *splicee)
{
	while (splicee->root) {
		struct avl_node *n = splicee->root;
		avl_delete(splicee, n);
		avl_insert(hd, n);
	}
}
