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
 * \file btree.c
 *
 * \author Eric Mueller
 *
 * \brief Implementation of a B-tree. 
 */  

#include "btree.h"
#include <assert.h>
#include <string.h>

#define BT_ORDER ((unsigned)32) /*! Order of the B-tree. */
#define BT_MAX_KEYS (BT_ORDER * 2) /*! max number of keys in each node */
#define BT_MAX_CHILDREN (BT_ORDER * 2 + 1) /*! actual number of spaces in nodes */

/* node classes */
#define ROOT_NODE (0x1u) /*! root node bit flag */
#define BODY_NODE (0x2u) /*! body node bit flag */
#define LEAF_NODE (0x4u) /*! leaf node bit flag */

/* macros to determine node class membership */
#define IS_ROOT(n) (!!((n)->class & ROOT_NODE))
#define IS_BODY(n) (!!((n)->class & BODY_NODE))
#define IS_LEAF(n) (!!((n)->class & LEAF_NODE))

/* error conditions */
#define BT_EEXISTS (-1)
#define BT_ENOMEM (-2)

struct bt_node;
typedef union {const void* leaf, struct bt_node *node} child_t;

/*! internal btree node. */
struct bt_node {
        uint32_t class;
	unsigned int nkeys; /*! number of keys currently in the node */
        uint64_t keys[BT_MAX_KEYS]; /*! keys seperating children !*/
        child_t c[BT_MAX_CHILDREN]; /*! children */
	union {
		struct bt_node *parent; /*! parent of the node */
		struct btree_t *head; /*! head of the tree (if root node) */
	};
};
typedef struct bt_node node_t;

/*
 * Allocate a new node an initialize it with the given class.
 */ 
static node_t *bt_alloc_node(uint32_t class)
{
	node_t *n = (node_t *)malloc(sizeof(node_t));
	if (n) {
		n->class = class;
		n->nkeys = 0;
		n->parent = NULL;
	}
        return n;
}

/*
 * Find the index of the child node containing key. Binary search.
 */ 
static int bt_binsearch_node(node_t *n, uint64_t key)
{
	int win_s = -1; /* start of search window */
	int win_e = n->nkeys; /* end of search window */
	int i; /* index under consideration */
	for (;;) {
		/* compute i */
		i = win_s + (win_e - win_s)/2;

		/* termination conditions */
		if (n->keys[i] == key || win_s - win_e == 1)
			return win_e;

		/* recompute win_s and win_e */
		if (n->keys[i] < key)
			win_s = i;
		else
			win_e = i;
	}
}

/**
 * \brief key/value shifting helper function
 *
 * \param n  Node to shift in
 * \param sindex  Starting index of shift. 
 * \param shift  Direction of shift. 1 for right, -1 for left
 *
 * \detail Either openes up a space at sindex or clobbers whatever is at sindex
 * with whatever is after it. If the n is a leaf, its leaves are shifted
 * exactly as the keys, otherwise if n is a body node, the child nodes are
 * shifted starting at the index one after sindex.
 */
static void bt_shift_kv(node_t *n, unsigned int sindex, int shift)
{
	int start;
	int end;
	int lc_offset IS_LEAF(n) ? 0 : 1;; /* are we shifting child (node) ptrs or leaf ptrs? */

	assert(shift == 1 || shift == -1);

	/*
	 * The below diagrams are for body nodes. The operation for leaf nodes
	 * looks almost identical, except the value indicies are shifted left by
	 * one. (because there are equally many children as keys, where as
	 * body nodes have one more child than key)
	 * 
	 * right shift case
	 *   before:                after:
	 *         start   end           start    end  
	 *          |       |             |        |
	 *     {k0,k1,k2,k3,_,_}      {k0,_,k1,k2,k3,_}
	 *   {v0,v1,v2,v3,v4,_,_}   {v0,v1,_,v2,v3,v4,_}
	 *           |       |             |       |
	 *          start   end           start   end
	 *
	 * left shift case
	 *   before:                after:
	 *         end   start            end  start
	 *          |     |                |    |
	 *     {k0,k1,k2,k3,_,_}      {k0,k2,k3,_,_,_}
	 *   {v0,v1,v2,v3,v4,_,_}   {v0,v1,v3,v4,_,_,_}
	 *          |      |               |     |
	 *         end    start           end   start
	 */
	if (shift < 0) {
		/* left */
		start = sindex + 1;
		end = n->nkeys - 1;
	} else {
		/* right */
		assert(n->nkeys < BT_MAX_KEYS);
		start = n->nkeys - 1;
		end = sindex;
	}

	for (int i = start; i != end; i += shift) {
		n->keys[i + shift] = n->keys[i];
		n->c[i + shift + lc_offset] = n->c[i + lc_offset];
	}
}

static void bt_insert_into_body(node_t *n, uint64_t key, node_t *child)
{
	int i = bt_binsearch_node(n, key);
	bt_shift_kv(n, i, 1);
	n->keys[i] = key;
	n->c[i+1].node = child;
	n->nkeys++;
}

static void bt_insert_into_leaf(node_t *n, uint64_t key, const void *val)
{
	int i = bt_binsearch_node(n, key);
	bt_shift_kv(n, i, 1);
	n->keys[i] = key;
	n->c[i].leaf = val;
	n->nkeys++:
}

/* fuck this function fuck btrees */
static int bt_split_insert_body(node_t *n, uint64_t key, node_t *child,
				uint64_t *seg, node_t **neighbor)
{
	node_t *right = bt_alloc_node(n->class);
	if (!right)
		return BT_ENOMEM;
	unsigned int split_k = n->nkeys/2;
	right->nkeys = n->nkeys - split_k;
	n->nkeys = split_k;
}

int btree_init(btree_t *bt)
{
	node_t *n = bt_alloc_node(ROOT_NODE | LEAF_NODE);
	if (!n)
		return BT_ENOMEM;
	n->head = bt;
	bt->root = n;
        return 0;
}

void btree_destroy(btree_t *bt)
{

}

int btree_insert(btree_t *bt, uint64_t key, const void *value)
{
	/* traverse to leaf */
	int i;
	for (node_t *n = bt->root; !IS_LEAF(n); n = n->children[i]) {
		i = bt_binsearch_node(n, key);
	}

	/* insert into leaf */
	if (n->nkeys == BT_MAX_KEYS) {
		
	}
	bt_insert_into_node(n, key, value);

	/* traverse back up and rebalance */
	while (n->nkeys > BT_MAX_KEYS) {
		
	}

        return 1;
}

int btree_contains(btree_t *bt, uint64_t key)
{
        return 1;
}

int btree_getval(btree_t *bt, uint64_t key, const void **val_ptr)
{
        return 1;
}

void btree_remove(btree_t *bt, uint64_t key)
{

}
