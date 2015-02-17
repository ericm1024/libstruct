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

/*! Order of the B-tree. */
#define BT_ORDER ((unsigned)32)
/*! Number of keys stored in each btree node */
#define BT_NUM_KEYS (BT_ORDER * 2)
/*! Number of child pointers in each btree node */
#define BT_NUM_CHILDREN (BT_ORDER * 2 + 1)

/* node class */
#define ROOT_NODE (0x1u)
#define BODY_NODE (0x2u)
#define LEAF_NODE (0x4u)

/* error conditions */
#define BT_EEXISTS (-1)
#define BT_ENOMEM (-2)

/*! internal btree node. */
struct bt_node {
        uint32_t class;
	unsigned int nkeys; /*! number of keys currently in the node */
        uint64_t keys[BT_NUM_KEYS]; /*! keys seperating children !*/
        union {  
                const void *leaves[BT_NUM_KEYS]; 
                             /*! data, if the node is a leaf */
                struct bt_node *children[BT_NUM_CHILDREN];
                             /*! child nodes, if the node is a body node */
        };
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

/* shift all keys and leaves/children starting at
 * index and offseting everything by shift */
static void bt_shift_leaves(node_t *n, size_t index, int shift)
{
	
}

/*
 * Find the index of the child node containing key. Binary search.
 * Returns BT_EEXISTS if the key already exists
 */ 
static int bt_binsearch_node(node_t *n, uint64_t key)
{
	int win_s = -1; /* start of search window */
	int win_e = n->nkeys; /* end of search window */
	int i; /* index under consideration */
	do {
		/* compute i */
		i = win_s + (win_e - win_s)/2;

		/* termination conditions */
		if (n->keys[i] == key)
			return BT_EEXISTS;
		else if (win_s - win_e == 1)
			return win_e;

		/* recompute win_s and win_e */
		if (n->keys[i] < key)
			win_s = i;
		else
			win_e = i;
	} while (1);
}

int btree_init(btree_t *bt)
{
	node_t *n = bt_alloc_node(ROOT_NODE | LEAF_NODE);
	if (!n) {
		return BT_ENOMEM;
	}
	n->head = bt;
	bt->root = n;
        return 0;
}

void btree_destroy(btree_t *bt)
{

}

int btree_insert(btree_t *bt, uint64_t key, const void *value)
{
	/* -- empty tree case --
	 * Allocate a new node, initialize it as both a root and leaf node,
	 * then use common co
	 */

	/* -- traverse to leaf --
	 * 
         */

	/* insert into leaf */

	/* traverse back up and rebalance */
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
