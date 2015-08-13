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
#ifdef DEBUG
	assert(right == AVL_RIGHT || right == AVL_LEFT);
#endif	
	if (!n)
		return NULL;
	
	struct avl_node *i = n->children[right];
	unsigned short left = 1 - right;
	while (i && i->children[left])
		i = i->children[left];
	return i;
}

static unsigned short child_index(struct avl_node *child, struct avl_node *parent)
{
#ifdef DEBUG
	assert(parent->children[AVL_LEFT] == child
	       || parent->children[AVL_RIGHT] == child);
#endif	
	if (parent->children[AVL_LEFT] == child)
		return AVL_LEFT;
	else /* parent->children[AVL_RIGHT] == child */
		return AVL_RIGHT;
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
#ifdef DEBUG
	assert(right == AVL_RIGHT || right == AVL_LEFT);
#endif
	unsigned short left = 1 - right;
	struct avl_node *b = root->children[left];
	struct avl_node *c = b->children[right];
	struct avl_node *p = get_parent(root);

#ifdef DEBUG
	assert( ((get_balance(root) == 1 && get_balance(b) != -1))
		|| ((get_balance(root) == -1 && get_balance(b) != 1)) );
#endif

	/* update b (and p) */
	if (p)
		p->children[child_index(root, p)] = b;
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
	short new_bf = dir_to_bf(right) * (~get_balance(b) & 1);
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
	short bal = get_balance(d);

#ifdef DEBUG
	assert(
		(get_balance(root) == 1 && get_balance(root->children[left]) == -1)
		|| (get_balance(root) == -1 && get_balance(root->children[left]) == 1));
#endif

	/* update d (and p) */
	if (p)
		p->children[child_index(root, p)] = d;
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
	short new_bf = -(((dir_to_bf(right) + bal) >> 1) & bal);
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

/*
 * see slides 40-42 of this:
 * http://courses.cs.washington.edu/courses/cse373/06sp/handouts/lecture12.pdf
 * for an explanation of the iterative insetion algorithm
 */
void avl_insert(struct avl_head *hd, struct avl_node *insertee)
{
#ifdef DEBUG
	assert(hd);
#endif
	if (!insertee)
		return;

	/* initialize the node with sane values before we do anything with it */
	insertee->children[AVL_RIGHT] = NULL;
	insertee->children[AVL_LEFT] = NULL;
	set_parent(insertee, NULL);
	set_balance(insertee, 0);

	/* find where we're going to insert in */
	struct avl_node **where = &hd->root;
	struct avl_node *parent = *where;
	while (*where) {
		parent = *where;
		int cmp = hd->cmp(insertee, *where);
		if (cmp == -1)
			where = &((*where)->children[AVL_LEFT]);
		else if (cmp == 1)
			where = &((*where)->children[AVL_RIGHT]);
		else if (cmp == 0) /* tried to insert the same value twice */
			return;
		else {
			fprintf(stderr, "avl_insert: comparator returned value "
				"other than -1, 0, or 1");
			return;
		}
	}

	/* insert 'in' */
	*where = insertee;
	set_parent(insertee, parent);
	hd->n_nodes++;

	/* traverse back until we hit the node in need of rebalancing */
	struct avl_node *child = insertee;
	unsigned short right;
	short bal;
	while (parent) {
		right = child_index(child, parent);
		bal = get_balance(parent);
		if (right == AVL_RIGHT)
			bal++;
		else /* right == AVL_LEFT */
			bal--;
		
		if (bal == RIGHT_OVERWEIGHT || bal == LEFT_OVERWEIGHT) {
			rotate(hd, parent, 1 - right);
			return;
		}
		
		set_balance(parent, bal);
		if (bal == BALANCED)
			return;
		child = parent;
		parent = get_parent(parent);
	}
}

void avl_delete(struct avl_head *hd, struct avl_node *victim)
{
	if (!victim)
		return;
	struct avl_node *path;
	struct avl_node *tmp;
	struct avl_node *child;
	unsigned short right;
	hd->n_nodes--;

	if (victim->children[AVL_LEFT] == NULL
	    && victim->children[AVL_RIGHT] == NULL) {
		/* no children case */
		tmp = get_parent(victim);
		if (tmp) {
			right = child_index(victim, tmp);
			tmp->children[right] = NULL;
		} else
			hd->root = NULL;
		path = tmp;
	} else if (victim->children[AVL_RIGHT] == NULL) {
		/* left child only case */
		tmp = get_parent(victim);
		child = victim->children[AVL_LEFT];
		if (tmp) {
			right = child_index(victim, tmp);
			tmp->children[right] = child;
		} else
			hd->root = child;
		set_parent(child, tmp);
		path = tmp;
	} else if (victim->children[AVL_LEFT] == NULL) {
		/* right child only case */
		tmp = get_parent(victim);
		child = victim->children[AVL_RIGHT];
		if (tmp) {
			right = child_index(victim, tmp);
			tmp->children[right] = child;
		} else
			hd->root = child;
		set_parent(child, tmp);
		path = tmp;
	} else {
		/* hard case -- two children */
		tmp = get_parent(victim);
		child = closest_child(victim, SIGN_BIT(~get_balance(victim)));

		/* move child's child up one, if such a child exists */
		path = get_parent(child);
		right = child_index(child, path);
		if (path == victim)
			path = child;
		if (child->children[AVL_LEFT]) {
			struct avl_node *parent = get_parent(child);
			parent->children[child_index(child, parent)] =
				child->children[AVL_LEFT];
			set_parent(child->children[AVL_LEFT], parent);
		} else if (child->children[AVL_RIGHT]) {
			struct avl_node *parent = get_parent(child);
			parent->children[child_index(child, parent)] =
				child->children[AVL_RIGHT];
			set_parent(child->children[AVL_RIGHT], parent);
		} else {
			struct avl_node *parent = get_parent(child);
			if (parent)
				parent->children[child_index(child, parent)] = NULL;
		}
		
		if (tmp)
			tmp->children[child_index(victim, tmp)] = child;
		else
			hd->root = child;
		
		child->children[AVL_LEFT] = victim->children[AVL_LEFT];
		child->children[AVL_RIGHT] = victim->children[AVL_RIGHT];
		if (child->children[AVL_LEFT])
			set_parent(child->children[AVL_LEFT], child);
		if (child->children[AVL_RIGHT])
			set_parent(child->children[AVL_RIGHT], child);
		set_balance(child, get_balance(victim));
		set_parent(child, tmp);
	}

	while (path) {
		short bal = get_balance(path);
		if (right == AVL_RIGHT)
			bal--;
		else
			bal++;

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
		right = child_index(child, path);
	}
}


struct avl_node *avl_find(struct avl_head *hd, struct avl_node *findee)
{
	if (!findee)
		return NULL;
	
	struct avl_node *n = hd->root;
	while (n) {
		int cmp = hd->cmp(findee, n);
		switch (cmp) {
		case -1:
			n = n->children[AVL_LEFT];
			break;
		case 0:
			return n;
		case 1:
			n = n->children[AVL_RIGHT];
			break;
		default:
			assert(false); /* TODO: handle bug */
		}
	}
	return NULL;
}

struct avl_node *avl_next(struct avl_node *elem)
{
	if (!elem)
		return NULL;
	
	struct avl_node *n = elem;
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
	
	struct avl_node *n = elem;
	if (n->children[AVL_LEFT])
		return closest_child(n, AVL_LEFT);
	else {
		struct avl_node *prev = NULL;
		while (n && prev == n->children[AVL_LEFT]) {
			prev = n;
			n = get_parent(n);
		}
		return n;
	}
}

struct avl_node *avl_first(struct avl_head *hd)
{
	if (!hd->root)
		return NULL;
	struct avl_node *first = hd->root;
	while (first->children[AVL_LEFT])
		first = first->children[AVL_LEFT];
	return first;
}

struct avl_node *avl_last(struct avl_head *hd)
{
	if (!hd->root)
		return NULL;
	struct avl_node *last = hd->root;
	while (last->children[AVL_RIGHT])
		last = last->children[AVL_RIGHT];
	return last;
}

void avl_splice(struct avl_head *hd, struct avl_head *splicee)
{
	if (!splicee->root)
		return;
	while (splicee->root) {
		struct avl_node *n = splicee->root;
		avl_delete(splicee, n);
		avl_insert(hd, n);
	}
}
