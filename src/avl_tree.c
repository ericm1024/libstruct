/* Eric Mueller
 * December 2014
 * avl_tree.c
 * 
 * Implementation of an avl tree.
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

static inline void set_parent(avl_node_t *node, avl_node_t *parent)
{
	node->parent = parent;
}

static inline void set_balance(avl_node_t *node, short bf)
{
	node->balance = bf;
}

static inline avl_node_t *get_parent(avl_node_t *node)
{
	return node->parent;
}

static inline short get_balance(avl_node_t *node)
{
	return node->balance;
}

/*
 * Various other helper functions
 */

static inline avl_node_t *data2node(avl_head_t *hd, void *d)
{
	return d ? (avl_node_t *)((uintptr_t)d + hd->offset) : NULL;
}

static inline void *node2data(avl_head_t *hd, avl_node_t *n)
{
	return n ? (void *)((uintptr_t)n - hd->offset) : NULL;
}

static inline avl_node_t *closest_child(avl_node_t *n,
					unsigned short right)
{
#ifdef DEBUG
	assert(right == AVL_RIGHT || right == AVL_LEFT);
#endif	
	if (!n)
		return NULL;
	
	avl_node_t *i = n->children[right];
	unsigned short left = 1 - right;
	while (i && i->children[left])
		i = i->children[left];
	return i;
}

static inline unsigned short child_index(avl_node_t *child, avl_node_t *parent)
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

static inline short dir_to_bf(unsigned short right)
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
static inline avl_node_t *rotate_single(avl_node_t *root, unsigned short right)
{
#ifdef DEBUG
	assert(right == AVL_RIGHT || right == AVL_LEFT);
#endif
	unsigned short left = 1 - right;
	avl_node_t *b = root->children[left];
	avl_node_t *c = b->children[right];
	avl_node_t *p = get_parent(root);

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

static inline avl_node_t *rotate_double(avl_node_t *root, unsigned short right)
{
        unsigned short left = 1 - right;
	avl_node_t *d = root->children[left]->children[right];
	avl_node_t *c = d->children[left];
	avl_node_t *e = d->children[right];
	avl_node_t *p = get_parent(root);
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

static inline avl_node_t *rotate(avl_head_t *hd, avl_node_t *root,
				 unsigned short right)
{
	avl_node_t *new_root = get_balance(root->children[1 - right])
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
void avl_insert(avl_head_t *hd, void *insertee)
{
#ifdef DEBUG
	assert(hd);
#endif
	if (!insertee)
		return;
	avl_node_t *in = data2node(hd, insertee);

	/* initialize the node with sane values before we do anything with it */
	in->children[AVL_RIGHT] = NULL;
	in->children[AVL_LEFT] = NULL;
	set_parent(in, NULL);
	set_balance(in, 0);

	/* find where we're going to insert in */
	avl_node_t **where = &hd->root;
	avl_node_t *parent = *where;
	while (*where) {
		parent = *where;
		int cmp = hd->cmp(insertee, node2data(hd, *where));
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
	*where = in;
	set_parent(in, parent);
	hd->n_nodes++;

	/* traverse back until we hit the node in need of rebalancing */
	avl_node_t *child = in;
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

void avl_delete(avl_head_t *hd, void *deletee)
{
	if (!deletee)
		return;
	avl_node_t *victim = data2node(hd, deletee);
	avl_node_t *path;
	avl_node_t *tmp;
	avl_node_t *child;
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
			avl_node_t *parent = get_parent(child);
			parent->children[child_index(child, parent)] =
				child->children[AVL_LEFT];
			set_parent(child->children[AVL_LEFT], parent);
		} else if (child->children[AVL_RIGHT]) {
			avl_node_t *parent = get_parent(child);
			parent->children[child_index(child, parent)] =
				child->children[AVL_RIGHT];
			set_parent(child->children[AVL_RIGHT], parent);
		} else {
			avl_node_t *parent = get_parent(child);
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


void *avl_find(avl_head_t *hd, void *findee)
{
	if (!findee)
		return NULL;
	
	avl_node_t *n = hd->root;
	while (n) {
		int cmp = hd->cmp(findee, node2data(hd,n));
		switch (cmp) {
		case -1:
			n = n->children[AVL_LEFT];
			break;
		case 0:
			return node2data(hd, n);
		case 1:
			n = n->children[AVL_RIGHT];
			break;
		default:
			assert(false); /* TODO: handle bug */
		}
	}
	return NULL;
}


void *avl_next(avl_head_t *hd, void *elem)
{
	if (!elem)
		return NULL;
	
	avl_node_t *n = data2node(hd, elem);
	if (n->children[AVL_RIGHT])
		return node2data(hd, closest_child(n, AVL_RIGHT));
	else {
		avl_node_t *prev = NULL;
		while (n && prev == n->children[AVL_RIGHT]) {
			prev = n;
			n = get_parent(n);
		}
		return node2data(hd, n);
	}
}

void *avl_prev(avl_head_t *hd, void *elem)
{
	if (!elem)
		return NULL;
	
	avl_node_t *n = data2node(hd, elem);
	if (n->children[AVL_LEFT])
		return node2data(hd, closest_child(n, AVL_LEFT));
	else {
		avl_node_t *prev = NULL;
		while (n && prev == n->children[AVL_LEFT]) {
			prev = n;
			n = get_parent(n);
		}
		return node2data(hd, n);
	}
}

void *avl_first(avl_head_t *hd)
{
	if (!hd->root)
		return NULL;
	avl_node_t *first = hd->root;
	while (first->children[AVL_LEFT])
		first = first->children[AVL_LEFT];
	return node2data(hd, first);
}

void *avl_last(avl_head_t *hd)
{
	if (!hd->root)
		return NULL;
	avl_node_t *last = hd->root;
	while (last->children[AVL_RIGHT])
		last = last->children[AVL_RIGHT];
	return node2data(hd, last);
}

void avl_splice(avl_head_t *hd, avl_head_t *splicee)
{
	if (!splicee->root)
		return;
	while (splicee->root) {
		void *n = node2data(splicee, splicee->root);
		avl_delete(splicee, n);
		avl_insert(hd, n);
	}
}
