/* Eric Mueller
 * December 2014
 * avl_tree.c
 * 
 * Implementation of an avl tree.
 */

#include "avl_tree.h"
#include <stddef.h>
#include <stdbool.h>
#include <limits.h> /* CHAR_BIT */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "bitops.h"

/* used as indices for children */
#define AVL_LEFT ((unsigned short)0)
#define AVL_RIGHT ((unsigned short)1)

/* used for balance factors */
#define LEFT_OVERWEIGHT ((short)-2)
#define LEFT_HEAVY ((short)-1)
#define BALANCED ((short)0)
#define RIGHT_HEAVY ((short)1)
#define RIGHT_OVERWEIGHT ((short)2)

#define DEBUG 1

/*
 * setters and getters for all the sh*t in struct avl_node. We have these
 * because if we want to change the way anything is stored (i.e. if we decide
 * we want to store the balance factor in the parent pointer, if we even want
 * to store a parent pointer, etc). Liberal use of these functions mean that
 * changing such low level implementation details is easy.
 */

static inline void set_lchild(avl_node_t *node, avl_node_t *child)
{
	node->children[AVL_LEFT] = child;
}

static inline void set_rchild(avl_node_t *node, avl_node_t *child)
{
	node->children[AVL_RIGHT] = child;
}

static inline void set_parent(avl_node_t *node, avl_node_t *parent)
{
	node->parent = parent;
}

static inline void set_balance(avl_node_t *node, short bf)
{
	node->balance = bf;
}

static inline void set_cradle(avl_node_t *node, unsigned short cr)
{
	node->cradle = cr;
}

static inline avl_node_t *get_lchild(avl_node_t *node)
{
	return node->children[AVL_LEFT];
}

static inline avl_node_t *get_rchild(avl_node_t *node)
{
	return node->children[AVL_RIGHT];
}

static inline avl_node_t *get_parent(avl_node_t *node)
{
	return node->parent;
}

static inline short get_balance(avl_node_t *node)
{
	return node->balance;
}

static inline unsigned short get_cradle(avl_node_t *node)
{
	return node->cradle;
}

/*
 * Various other helper functions
 */ 

static inline unsigned short opposite_dir(unsigned short dir)
{
#ifdef DEBUG
	assert(dir == AVL_RIGHT || dir == AVL_LEFT);
#endif	
	return 1 - dir;
}

/* convert direction (i.e. AVL_LEFT or AVL_RIGHT) to a balance factor (i.e.
 * LEFT_HEAVY or RIGHT_HEAVY */
static inline short dir_to_bf(unsigned short dir)
{
	/* trans[AVL_LEFT] gives -1, the balance factor offset for left
	 * rotations.
	 *
	 * trans[AVL_RIGHT] gives 1, the balance factor offset for right
	 * rotations.
	 */
#ifdef DEBUG
	assert(dir == AVL_LEFT || dir == AVL_RIGHT);
#endif	
	static const short trans[2] = {LEFT_HEAVY, RIGHT_HEAVY};
	return trans[dir];
}

/* does the opposite of dir_to_bf */
static inline unsigned short bf_to_dir(short bf)
{
#ifdef DEBUG	
	assert(bf == RIGHT_HEAVY || bf == LEFT_HEAVY); /* i.e. bf != 0 */
#endif	
	static const unsigned short trans[3] = {AVL_LEFT,0,AVL_RIGHT};
	return trans[bf + 1];
}

static inline avl_node_t *data2node(avl_head_t *hd, void *d)
{
	return d ? (avl_node_t *)((uintptr_t)d + hd->offset) : NULL;
}

static inline void *node2data(avl_head_t *hd, avl_node_t *n)
{
	return n ? (void *)((uintptr_t)n - hd->offset) : NULL;
}

/*
 * returns AVL_LEFT if lhs < rhs, AVL_RIGHT if lhs > rhs, and -1 if lhs == rhs
 */
static inline short relative_dir(avl_head_t *hd, avl_node_t *lhs,
				 avl_node_t *rhs)
{
	/* find which subtree 'in' resides in  */
	int cmp = hd->cmp(node2data(hd, lhs), node2data(hd, rhs));
	if (cmp == -1)
		return AVL_LEFT;
	else if (cmp == 1)
		return AVL_RIGHT;
	else if (cmp == 0)
		return -1;
	else {
		printf("relative_dir: hd->cmp returned unknown cmp value %i.\n",cmp);
		return -1;
	}	
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
static inline avl_node_t *rotate_single(avl_node_t *root,
					unsigned short right)
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
		p->children[get_cradle(root)] = b;
	set_parent(b, get_parent(root));
	set_cradle(b, get_cradle(root));
	b->children[right] = root;
	set_balance(b, dir_to_bf(right) * (~get_balance(b) & 1));
                                   // TODO: ditch the imul

	/* update d (root) */
	set_parent(root, b);
	set_cradle(root, right);
	set_balance(root, -get_balance(b));
	root->children[left] = c;

	/* update c */
	if (c) {
		set_parent(c, root);
		set_cradle(c, left);
	}

	return b; /* return the new root */
}

/*
 *      f               d
 *     / \            /   \ 
 *    b   G   --->   b     f
 *   / \            / \   / \ 
 *  A   d          A   C E   G
 *     / \
 *    C   E
 *
 * Double rotation. Equivalent to rotating left at 'b' then right at 'f', but
 * coding this as its own function saves branches elsewhere. Pictured above
 * is a right double rotation, but the code is written generically, making use
 * of the 'right' argument.
 *
 * Note that such a right-rotation is only necessary when 'f' is left-overweight
 * and 'b' is right heavy. The corresponding mirrored left-rotation is similarly
 * only necessary when 'f' is right-overweight and 'b' is left-heavy. If 'b' is
 * not right (left) heavy, then rebalancing is acomplished with a single
 * rotation. Thus this function should never be called unless those
 * preconditions are met.
 */ 
static inline avl_node_t *rotate_double(avl_node_t *root,
					unsigned short right)
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
		p->children[get_cradle(root)] = d;
	d->children[right] = root;
	d->children[left] = get_parent(d);
	set_cradle(d, get_cradle(root));
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
	if (c) {
		set_parent(c, d->children[left]);
		set_cradle(c, right);
	}

	/* update E */
	if (e) {
		set_parent(e, d->children[right]);
		set_cradle(e, left);
	}

	/* update f (root) */
	root->children[left] = e;
	set_parent(root, d);
	set_cradle(root, right);

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

	return d; /* return the new root */
}

/* used below in balance_callback_vec to do nothing */
static inline avl_node_t *rotate_nop(avl_node_t *root, unsigned short right)
{
	(void)right;
	return root;
}

typedef avl_node_t *(*rot_func_t)(avl_node_t *n, unsigned short right);

/* balancing callback vector */
static const rot_func_t balance_callback_vec[3] = {rotate_nop,
						   rotate_single,
						   rotate_double};

/*
 * see slides 40-42 of this:
 * http://courses.cs.washington.edu/courses/cse373/06sp/handouts/lecture12.pdf
 * for an explanation of the iterative insetion algorithm
 */
void avl_insert(avl_head_t *hd, void *insertee)
{
#ifdef DEBUG
	assert(hd);
	assert(insertee);
#endif
	avl_node_t *in = data2node(hd, insertee);
	avl_node_t *p = hd->root;
	short dir;
	
	set_lchild(in, NULL);
	set_rchild(in, NULL);
	set_parent(in, NULL);
	set_balance(in, 0);
	
	if (!hd->root) {
		hd->root = in;
		set_cradle(in, 0);
		hd->n_nodes++;
		return;
	}

	/* find where we're going to insert in */
	while (true) {
		dir = relative_dir(hd, in, p);
		/* if we tried to insert the same node twice, bail */
		if (dir == -1) {
			fprintf(stderr,
				"avl_insert: tried to insert same node twice\n");
			return;
		}
		
		if (!p->children[dir])
			break;
		p = p->children[dir];
	}

	/* insert 'in' */
	p->children[dir] = in;
	set_parent(in, p);
	set_cradle(in, dir);
	hd->n_nodes++;
	
	/* traverse back until we hit the node in need of rebalancing */
	for (short bf = dir_to_bf(dir);
	     p != hd->root && get_balance(p) == 0;
	     bf = dir_to_bf(get_cradle(p)), p = get_parent(p))
		set_balance(p, bf);
	
	dir = relative_dir(hd, in, p);
	
	/*
	 * Now we need to fix up the node in need of rebalancing.
	 *
	 * If the node has bf of zero, we know it must be the root (see the
	 * loop invarient in the above for loop if you don't believe me). So
	 * we just change its balance factor.
	 *
	 * If the node was left (right) heavy and we inserted in its right
	 * (left) subtree, then yay! it became more balanced.
	 *
	 * Otherwise, we need to do one or two rotations. If it was an outer
	 * insertion (an insertion into either the right-right subtree or
	 * left-left subtree) then we need one rotation. Else we need two.
	 */
	avl_node_t *n = NULL;
	if (get_balance(p) == 0)
		set_balance(p, dir_to_bf(dir));
	else if (dir_to_bf(dir) == -get_balance(p))
		set_balance(p, 0);
	else if (dir == relative_dir(hd, in, p->children[dir]))
		n = rotate_single(p, opposite_dir(dir));
	else 
		n = rotate_double(p, opposite_dir(dir));

	if (n && !get_parent(n))
		hd->root = n;
}

void avl_delete(avl_head_t *hd, void *deletee)
{
	avl_node_t *victim = data2node(hd, deletee);
	avl_node_t *p = get_parent(victim);
	avl_node_t *new;
	avl_node_t *path;
	unsigned int dir;

	if (!victim)
		return;
	hd->n_nodes--;
	
	/*
	 * find the replacement node. magic on the end gets the opposite of the
	 * sign bit, which is in turn the direction (AVL_LEFT or AVL_RIGHT i.e.
	 * 0 or 1) that the subtree rooted at 'new' leans
	 */
	new = closest_child(victim, SIGN_BIT(~get_balance(victim)));
	
	if (new) {
		path = get_parent(new);
		dir = get_cradle(new);
	
		/*
		 * put the replacement node where victim was, uptating all its
		 * members as necessary
		 */
		set_parent(new, get_parent(victim));
		set_rchild(new, get_rchild(victim));
		set_lchild(new, get_lchild(victim));
		set_cradle(new, get_cradle(victim));
		set_balance(new, get_balance(victim));
	} else {
		path = get_parent(victim);
		dir = get_cradle(victim);
	}
	
	if (!p)
		hd->root = new;
	else
		p->children[dir] = new;
	
	/*
	 * starting from where the replacement was originally, traverse the tree
	 * upwards, updating balance factors untill we make an update that
	 * doesn't change the height of the tree.
	 *
	 * If the new balance factor is -2 or 2, we need to rebalance.
	 *
	 * If the balance factor (after possibly rebalancing) is nonzero,
	 * (i.e. it's magnitude either increased or stayed the same), then
	 * we're done.
	 */
	short old_bf = get_balance(path);
	while (path) {
		short bf = get_balance(path);
		bf -= dir_to_bf(dir);

		/*
		 * table for index into balanace callback vector
		 *   bf  old  i
		 *  ------------ 
		 *   2 | -1 | 2
		 *   2 |  0 | 1
		 *   2 |  1 | 1
		 *  -2 | -1 | 1
		 *  -2 |  0 | 1
		 *  -2 |  1 | 2
		 *   x |  x | 0
		 *
		 *  ((bf & -bf) >> 1) << (((bf - (old << 1)) & 4) >> 2)
		 *
		 *  So it's a lot of instructions, but I think it should be
		 *  better than the alternative branches. Plus the two halves
		 *  of the expression will be evaluated at the same time.
		 */
		size_t index = ((bf & -bf) >> 1)
			<< (((bf - (old_bf << 1)) & 4) >> 2);
#ifdef DEBUG
		if (bf == 2 || bf == -2)
			assert( ((bf & -bf) >> 1) == 1 );
		else
			assert ( ((bf & -bf) >> 1) == 0 );
		
		if (bf < 0 && old_bf > 0)
			assert( (((bf - (old_bf << 1)) & 4) >> 2) == 1);
		else if (bf > 0 && old_bf < 0)
			assert( (((bf - (old_bf << 1)) & 4) >> 2) == 1);
		
		if (bf == -2) {
			if (old_bf == 1)
				assert(index == 2);
			else
				assert(index == 1);
		} else if (bf == 2) {
			if (old_bf == -1)
				assert(index == 2);
			else
				assert(index == 1);
		} else
			assert(index == 0);
#endif
		
		path = balance_callback_vec[index](path, dir);
		
		if ( get_balance(path) != 0 )
			break;
		
		dir = get_cradle(path);
		path = get_parent(path);
		old_bf = bf;
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
		while(get_parent(n) && get_cradle(n) != AVL_LEFT)
			n = get_parent(n);
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
		while(get_parent(n) && get_cradle(n) != AVL_RIGHT)
			n = get_parent(n);
		return node2data(hd, n);
	}
}

void *avl_first(avl_head_t *hd)
{
	if (!hd->root)
		return NULL;
	avl_node_t *first = hd->root;
	while (get_lchild(first))
		first = get_lchild(first);
	return node2data(hd, first);
}

void *avl_last(avl_head_t *hd)
{
	if (!hd->root)
		return NULL;
	avl_node_t *last = hd->root;
	while (get_rchild(last))
		last = get_rchild(last);
	return node2data(hd, last);
}

void avl_splice(avl_head_t *hd, avl_head_t *splicee)
{
	for ( void *n = avl_first(splicee); n; n = avl_next(hd, n))
		avl_insert(hd, n);
}
