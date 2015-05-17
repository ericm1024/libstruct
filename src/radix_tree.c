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
 * \file radix_tree.c
 *
 * \author Eric Mueller
 * 
 * \brief Implementation of a linux kernel-syle radix tree.
 *
 * \detail This radix tree attempts to be smart in that it only branches
 * when it needs to. If the set of keys stored by the tree is distributed
 * in such a way that a naive radix tree would have branches that go several
 * nodes without branching, this radix tree will not actually create those
 * nodes unless the need arrises later (i.e. branches need to be created).
 */

#include "radix_tree.h"
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

#define BITS_PER_LONG (sizeof(long)*CHAR_BIT)
#define LONG_SHIFT (BITS_PER_LONG == 64 ? 6 : 5)

/**
 * minimum number of bits each level of the tree will use (except possibly
 * the lowest level, if this value does not evenly divide BITS_PER_LONG)
 */
#define RADIX_TREE_SHIFT (6L)

/* number of children per node */
#define RADIX_TREE_CHILDREN (1UL << RADIX_TREE_SHIFT)

/* mask in the low order bits that is RADIX_TREE_SHIFT bits wide */
#define RADIX_TREE_MASK (RADIX_TREE_CHILDREN - 1)

/* tag to mark a node a a leaf. Stored in the node's parent pointer */
#define RADIX_TAG_LEAF ((uintptr_t)0x1)

#define RADIX_UNTAG_LEAF (~RADIX_TAG_LEAF)

/*
 * number of bits at the end of a key that we don't use. Currently
 * set to the size of a page.
 */
#define RADIX_KEY_CUTOFF (12L)

#define RADIX_KEY_DIFF (1L << RADIX_KEY_CUTOFF)

/* actual number of bits in the key that we use */
#define RADIX_KEY_SIZE (BITS_PER_LONG - RADIX_KEY_CUTOFF)

/* length of prefix for a leaf node */
#define RADIX_LEAF_PREFIX_LEN					\
	(RADIX_KEY_SIZE - RADIX_KEY_SIZE % RADIX_TREE_SHIFT)

struct radix_node {
	/*
	 * parent node to this node and tag to denote if current node is
	 * a leaf
	 */
	union parent_union {
		struct radix_node *node;
		uintptr_t tag;
	} parent;
	
	/*
	 * array of children -- either nodes or values, depending on
	 * whether the tag field of parent has RADIX_TAG_LEAF
	 */
	union child_union {
		struct radix_node *node;
		const void *val;
	} children[RADIX_TREE_CHILDREN];

	/*
	 * all elements in the subtree rooted at a node match this prefix
	 * up through a certain length from the highest bit. For example,
	 * if the prefix was 0xFA15_0000_0000_0000 and the prefix length
	 * was 16, then all elements in the subtree rooted at the node
	 * have indicies starting with 0xFA15. See RADIX_LEN_TO_MASK.
	 */
	long prefix;
	unsigned int pref_len:LONG_SHIFT;

	/* index in parent */
	unsigned int parent_index:RADIX_TREE_SHIFT;

	/* nr valid children */
	unsigned int entries:RADIX_TREE_SHIFT;
};


/* ====== generic helper functions ====== */

/**
 * \brief helper for generating a "match mask". Used to help determine
 * membership of an key within a node.
 *
 * \param len   The length the mask to declare. len bits starting at the
 *              highest bit are set high, the rest are low. For example, giving
 *              a length of 8 would produce the result 0xFF00_0000_0000_0000 if
 *              longs are 64 bits.
 *
 * \detail Given a node with prefix p and prefix length l, a key k belongs in
 * that node iff (radix_prefix_to_mask(l) & k) ^ p evaluates to 0.
 */
static inline long radix_node_mask(unsigned int pref_len)
{
	return ~((1L << (BITS_PER_LONG - pref_len)) - 1L);
}	

/**
 * \brief given a prefix length, generate a mask that will flag the bits
 * in a key needed to index into the next child
 *
 * \param pref_len   The length of the prefix to generate a mask for. For
 *                   example, a prefix length of 8 and a RADIX_TREE_SHIFT
 *                   of 6 would prouce the mask 0x00FC_0000_0000_0000 for
 *                   64 bit longs
 */
static inline long radix_key_mask(unsigned int pref_len)
{
	long mask = radix_node_mask(pref_len);
	return mask ^ (mask >> RADIX_TREE_SHIFT);
}

static inline bool node_contains_key(const struct radix_node *node, long key)
{
	return ((radix_node_mask(node->pref_len) & key) ^ node->prefix) == 0;
}

/**
 * \brief Get the index of a key within a given node.
 * 
 * \param node  The node.
 * \param key   The key.
 * 
 * \return the index of the child node containing key.
 */
static inline unsigned int radix_get_index(const struct radix_node *node,
					   long key)
{
	/* key actually belongs in this node */
	assert(node_contains_key(node, key));
	unsigned int pref_len = node->pref_len;
	unsigned int index = radix_key_mask(pref_len) & key;
	return (index >> (BITS_PER_LONG - pref_len)) & RADIX_TREE_MASK;
}

/* marks a node as being a leaf */
static inline void node_mark_leaf(struct radix_node *node)
{
	node->shared.tag |= RADIX_TAG_LEAF;
}

/* predicate for determining if a node is a leaf */
static inline bool node_is_leaf(struct radix_node *node)
{
	return node->shared.tag & RADIX_TAG_LEAF;
}

/* predicate for determining if a prefix is a leaf */
static inline bool prefix_is_leaf(unsigned int pref_len)
{
	return RADIX_KEY_SIZE - prefix_len <= RADIX_TREE_SHIFT;
}

static inline struct radix_node *get_parent(const struct radix_node *node)
{
	return (struct radix_node *)(node->parent.tag & RADIX_UNTAG_LEAF);
}

static inline void set_parent(struct radix_node *node,
			      const struct radix_node *parent)
{
	/* gross */
	uintptr_t old_tag = node->parent.tag & RADIX_TAG_LEAF;
	uintptr_t parent_tagged = (uintptr_t)parent;
	parrent_tagged |= old_tag;
	node->parent.tag = parent_tagged;
}

/**
 * \brief allocate and initialize a new node and insert it into its parent.
 *
 * \param head       The head of the tree to insert into
 * \param parent     Parent of the new node.
 * \param prefix     Prefix of the new node.
 * \param pref_len   Length of new node's prefix.
 *
 * \return the new node, or null if memory allocation failed.
 */
static radix_node *alloc_node(struct radix_head *head,
			      struct radix_node *parent,
			      long prefix,
			      unsigned int pref_len)
{
	assert(pref_len < RADIX_KEY_SIZE - 1);
	
	struct radix_node *node = malloc(sizeof(struct radix_node));
	if (!node)
		return node;

	/* initialize the node */
	if (prefix_is_leaf(pref_len))
		node_mark_leaf(node);
	
	for (unsigned long i = 0; i < RADIX_TREE_CHILDREN; i++)
		node->children[i].node = NULL;

	node->prefix = prefix;
	node->prefx_len = pref_len;
	set_parent(node, parent);
	node->entries = 0;

	/* physically put the node in the tree */
	head->nnodes++;
	if (parent) {
		assert(head->root);
		node->parent_index = radix_get_index(parent, prefix);
		parent->children[node->parent_index].node = node;
		parent->entries++;
	} else {
		assert(!head->root);
		node->parent_index = 0;
		head->root = parent;
	}

	return node;
}

static inline void insert_into_node(struct radix_head *restrict head,
				    struct radix_node *restrict node,
				    long key, const void *value)
{
	assert(node_is_leaf(node));
	head->nentries++;
	node->nentries++;
	node->children[radix_get_index(node, key)].val = value;
}

/**
 * \brief Split a compressed path above a node to allow for insertion
 * of a key.
 *
 * \param head    The head of the tree.
 * \param child   The child node above which the path should be split.
 * \param key     The key to make a path for.
 *
 * \detail This is the workhorse function that allows this radix tree
 * implementation to be space efficient. It is called when a key belongs in
 * the subtree rooted at <child>, but does not belong in <child> itself. This
 * occurs when a path has been previously 'compressed.'
 *
 * In essence, it looks for the longest portion of the prefix of <child>
 * that matches the given key and creates a node above <child> with that
 * prefix length.
 */
static struct radix_node *split_node_key(struct radix_head *restrict head,
					 struct radix_node *restrict child,
					 long key)
{
	/*
	 * start the new node off with an incrementally shorter prefix.
	 * note that this may be shortened below
	 */
	struct radix_node *path =
		alloc_node(head, child->shared.parent, child->prefix,
			   child->prefix_len - RADIX_TREE_SHIFT);
	if (!path)
		return NULL;

	/* fix up the new path's prefix length */
	while (!node_contains_key(path, key))
		path->prefix_len -= RADIX_TREE_SHIFT;

	assert(node_contains_key(path, child->prefix));
	assert(radix_get_index(path, child->prefix) !=
	       radix_get_index(path, key));

	/* fix up the rest of the tree */
	set_parent(child, path);
	child->parent_index = radix_get_index(path, child->prefix);
	path->children[child->parent_index] = child;
	path->entries++;
}

/*
 * tells radix_tree_walk to allocate nodes that don't exist but need to in
 * order to get to a particular key.
 */ 
#define WALK_FLAG_ALLOC (0x1)
/*
 * tells radix_tree_walk to return the closest node to the node containing
 * a key if no such node exists. "closest" is defined as the deepest existing
 * parent.
 */ 
#define WALK_FLAG_CLOSEST (0x2)
#define FLAG_BIT(flag, bit) (flag & bit)

/**
 * \brief Traverse a tree to the node containing a key, possibly allocating
 * nodes as necessary.
 *
 * \param head    Head of the tree being walked.
 * \param key     Key corresponding to the slot to walk to.
 * \param start   Node to start at, or null.
 * \param flags   Tells the function how to behave with respect to nodes that
 *                don't exist.
 *
 * \return if should_alloc is true, returns the node containing key or null
 * if memory allocation failed. Otherwise returns the node containing key, or
 * null if no such node could be foud.
 *
 * \detail This function is inlined because is composes most of almost
 * every function here, and inlining it allows us to optimize away
 * conditionals.
 */ 
static inline struct radix_node *
radix_tree_walk(struct radix_head *restrict head, long key,
		struct radix_node *restrict start, int flags)
{
	struct radix_node *path = start ? start : head->root;
	
	/* if the tree is empty, allocate something */
	if (!head->root) {
		if (FLAG_BIT(flags, WALK_FLAG_ALLOC))
			return NULL;
		
		path = alloc_node(head, NULL, key, RADIX_LEAF_PREFIX_LEN);
		if (!path)
			return NULL;
	}
	
	/*
	 * if we're deep enough down in the tree that the subtree we're
	 * in doesn't contain the key we're searching for, walk up till
	 * we find one
	 */
	while (get_parent(path) && !node_contains_key(path, key))
		path = get_parent(path);

	/* walk back down */
	while (!node_is_leaf(path)) {
		unsigned int i = radix_get_index(path, key);
		struct radix_node *child = path->children[i].node;
		if (!child) {
			if (!FLAG_BIT(flags, WALK_FLAG_ALLOC))
				return FLAG_BIT(flags, WALK_FLAG_CLOSEST)
					? path : NULL;
			
			path = alloc_node(head, path, key,
					  RADIX_LEAF_PREFIX_LEN);
			return path;
		}
		path = child;
	}

	if (node_contains_key(path, key))
		return path;
	else if (!should_alloc)
		return FLAG_BIT(flags, WALK_FLAG_CLOSEST) ? path : NULL;
	
	/*
	 * we need to allocate both an intermediate node and
	 * a new leaf node. If memory allocation fails on the
	 * second allocation, we don't need to deallocate
	 * and remove the first node we allocate. It's
	 * just making one path slightly longer, it's not
	 * actually causing problems or dangling in the ether.
	 */
	path = split_node_key(head, path, key);
	if (!path)
		return NULL;
	path = alloc_node(head, path, key, RADIX_LEAF_PREFIX_LEN);
	return path;
}

/**
 * \brief Similar to radix_tree_walk, except instead of hunting for the node
 * containing a given key, this function just hunts for the next open slot
 * in a given direction (either left or right, which is what the lr is short
 * for.)
 *
 * \param start   The node to start at.
 * \param index   The index within the starting node to start at.
 * \param left    True if the function should search left, false to go right.
 *
 * \return 
 */
static inline struct radix_node *
radix_tree_walk_lr(struct radix_node *start, unsigned int index, bool left)
{
	assert(index < RADIX_TREE_CHILDREN);
	
	if (!start)
		return NULL;

	unsigned int step = left ? -1 : 1;
	unsigned int end = left ? 

	do {
		
		
		index = start->parent_index;
	} while ((start = get_parent(start)));
}

/*
 * normally we'd want to write this itteratively instead of recursively,
 * but the stack space used by each call is pretty minimal, (only 3
 * local variable really should be saved) and radix trees are short
 * (16 nodes deep at most, 11 currently on 64 bit platforms), so stack usage
 * should be less than 512 bytes, which isn't too bad.
 */
static void destroy_node(struct radix_node *restrict node,
			 void (*restrict dtor)(const void *node, void *private),
			 void *restrict private)
{
	bool is_leaf = node_is_leaf(node);
	for (unsigned long i = 0; i < RADIX_TREE_CHILDREN) {
		struct radix_node *child = node->children[i].node;
		if (child) {
			if (is_leaf)
				dtor(child, private);
			else
				destroy_node(child, dtor, private);
		}
	}
}

void radix_destroy(struct radix_head *restrict head,
		   void (*restrict dtor)(const void *node, void *private),
		   void *restrict private)
{
	destroy_node(head->root);
	head->nnodes = 0;
	head->nentries = 0;
	head->root = NULL;
}

bool radix_insert(struct radix_head *head, unsigned long key,
		  const void *value)
{
	assert(val);

	struct radix_node *node;
	node = radix_tree_walk(head, NULL, /* start at root */
			       key, true); /* allocate memory */
	if (!node)
		return false;

	insert_into_node(head, node, key, value);
	return true;
}

bool radix_lookup(const struct radix_head *restrict head,
		  unsigned long key, const void **restrict result)
{
	struct radix_node *node;
	node = radix_tree_walk(head, NULL, /* start at root */
			       key, false); /* don't allocate memory */

	if (!node)
		return false;

	unsigned int i = radix_get_index(node, key);
	const void *val = node->children[i].val;
	if (!val)
		return false;
	
	*result = val;
	return true;
	
}

void radix_cursor_begin(const struct radix_head *restrict head,
			radix_cursor_t *restrict cursor)
{
	struct radix_node *node = head->root;
	if (!node)
		return;

	unsigned int i;
	
	for (;;) {
		for (i = 0; i < RADIX_TREE_CHILDREN; i++)
			if (node->children[i].node)
				break;
		
		if (node_is_leaf(node))
			break;
		node = node->children[i].node;
	}

	cursor->owner = head;
	cursor->node = node;
	cursor->index = i;
}

void radix_cursor_end(const struct radix_head *restrict head,
		      radix_cursor_t *restrict cursor)
{
	struct radix_node *node = head->root;
	if (!node)
		return;

	unsigned int i;
	
	for (;;) {
		for (i = RADIX_TREE_CHILDREN; i-- > 0;)
			if (node->children[i].node)
				break;
		
		if (node_is_leaf(node))
			break;
		node = node->children[i].node;
	}

	cursor->owner = head;
	cursor->node = node;
	cursor->index = i;
}

bool radix_cursor_next(radix_cursor_t *cursor)
{
	cursor->
}
