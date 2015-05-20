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
#include "bitops.h"
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

#define BITS_PER_LONG (sizeof(unsigned long)*CHAR_BIT)
#define LONG_SHIFT (BITS_PER_LONG == 64 ? 6 : 5)

/** number of children per node */
#define RADIX_TREE_CHILDREN (1 << RADIX_TREE_SHIFT)

/** mask of the lowest RADIX_TREE_SHIFT bits */
#define RADIX_TREE_MASK (RADIX_TREE_CHILDREN - 1)

/** difference between sucessive keys in the tree */
#define RADIX_KEY_DIFF (1UL << RADIX_KEY_UNUSED_BITS)

/**
 * mask keys with this to make sure they are a propper multiple of
 * RADIX_KEY_DIFF
 */
#define RADIX_KEY_MASK (~(RADIX_KEY_DIFF - 1))

/** actual number of bits in the key that we use */
#define RADIX_BITS_PER_KEY (BITS_PER_LONG - RADIX_KEY_UNUSED_BITS)

/**
 * length of prefix for a leaf node
 *
 * \detail This requires a little explanation. We'd like to just say that the
 * prefix length of a leaf is just RADIX_BITS_PER_KEY - RADIX_TREE_SHIFT, but
 * this assumes that RADIX_TREE_SHIFT divides RADIX_BITS_PER_KEY evenly, which
 * it need not, so we need to check if it does.
 */
#define RADIX_LEAF_PREFIX_LEN						\
	((RADIX_BITS_PER_KEY % RADIX_TREE_SHIFT == 0)			\
	 ? RADIX_BITS_PER_KEY - RADIX_TREE_SHIFT			\
	 : RADIX_BITS_PER_KEY - (RADIX_BITS_PER_KEY % RADIX_TREE_SHIFT))

/** maximum value of a key within the tree */
#define RADIX_KEY_MAX				\
	((~0UL - RADIX_KEY_DIFF) + 1)

/** tag to mark a node a a leaf. Stored in the node's parent pointer */
#define RADIX_TAG_LEAF ((uintptr_t)0x1)

/** mask to remove the leaf tag */
#define RADIX_UNTAG_LEAF (~RADIX_TAG_LEAF)

/** This structure is used to represent the tree's internal nodes. */
struct radix_node {
	/**
	 * parent node to this node and tag to denote if current node is
	 * a leaf
	 */
	union {
		struct radix_node *node;
		uintptr_t tag;
	} parent;
	
	/**
	 * array of children -- either nodes or values, depending on
	 * whether the tag field of parent has RADIX_TAG_LEAF
	 */
	union {
		struct radix_node *node;
		const void *val;
	} children[RADIX_TREE_CHILDREN];

	/**
	 * all elements in the subtree rooted at a node match this prefix
	 * up through a certain length from the highest bit. For example,
	 * if the prefix was 0xFA15_0000_0000_0000 and the prefix length
	 * was 16, then all elements in the subtree rooted at the node
	 * have indicies starting with 0xFA15. See RADIX_LEN_TO_MASK.
	 */
	unsigned long prefix;
	unsigned int pref_len:LONG_SHIFT;

	/** index in parent */
	unsigned int parent_index:RADIX_TREE_SHIFT;

	/** nr valid children */
	unsigned int entries:RADIX_TREE_SHIFT;

	/*
	 * TODO: possibly make these not bit fields. may be grossly unnecessary
	 * savings right now given 32 bit ints, LONG_SHIFT == 6, and
	 * RADIX_TREE_SHIFT == 6, is 8 bytes if we just made them all unsigned
	 * ints instead of bit fields
	 */
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
static inline unsigned long radix_node_mask(unsigned int pref_len)
{
	return ~((1UL << (BITS_PER_LONG - pref_len)) - 1UL);
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
static inline unsigned long radix_key_mask(unsigned int pref_len)
{
	unsigned long mask = radix_node_mask(pref_len);
	return mask ^ (mask >> RADIX_TREE_SHIFT);
}

/** predicate to determine if a node or its subtree contains a given key */
static inline bool node_contains_key(const struct radix_node *node,
				     unsigned long key)
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
					   unsigned long key)
{
	/* key actually belongs in this node */
	assert(node_contains_key(node, key));
	unsigned int pref_len = node->pref_len;
	unsigned int index = radix_key_mask(pref_len) & key;
	return (index >> (BITS_PER_LONG - pref_len)) & RADIX_TREE_MASK;
}

/** mark a node as being a leaf */
static inline void node_mark_leaf(struct radix_node *node)
{
	node->parent.tag |= RADIX_TAG_LEAF;
}

/** predicate for determining if a node is a leaf */
static inline bool node_is_leaf(const struct radix_node *node)
{
	return node->parent.tag & RADIX_TAG_LEAF;
}

/** predicate for determining if a prefix length denotes a leaf */
static inline bool prefix_is_leaf(unsigned int pref_len)
{
	return RADIX_BITS_PER_KEY - pref_len <= RADIX_TREE_SHIFT;
}

/** get the parent node of a node */
static inline struct radix_node *get_parent(const struct radix_node *node)
{
	return (struct radix_node *)(node->parent.tag & RADIX_UNTAG_LEAF);
}

/** set the parent node of a node */
static inline void set_parent(struct radix_node *node,
			      const struct radix_node *parent)
{
	/* gross */
	uintptr_t old_tag = node->parent.tag & RADIX_TAG_LEAF;
	uintptr_t parent_tagged = (uintptr_t)parent;
	parent_tagged |= old_tag;
	node->parent.tag = parent_tagged;
}

/**
 * \brief Construct the key corresponding to an index into a leaf node.
 * 
 * \param node    Leaf node in which the constructed key will fall.
 * \param index   Index into the child array of the given node.
 *
 * \return key that maps to the slot in node indexed by index.
 */
static inline unsigned long node_index_to_key(const struct radix_node *node,
					      unsigned long index)
{
	assert(node_is_leaf(node));
	unsigned long key = node->prefix & radix_node_mask(node->pref_len);
	key |= index << RADIX_KEY_UNUSED_BITS;
	return key;
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
static struct radix_node *alloc_node(struct radix_head *head,
				     struct radix_node *parent,
				     unsigned long prefix,
				     unsigned int pref_len)
{
	assert(pref_len < RADIX_BITS_PER_KEY - 1);
	
	struct radix_node *node = malloc(sizeof(struct radix_node));
	if (!node)
		return node;

	/* initialize the node */
	if (prefix_is_leaf(pref_len))
		node_mark_leaf(node);
	
	for (unsigned long i = 0; i < RADIX_TREE_CHILDREN; i++)
		node->children[i].node = NULL;

	node->prefix = prefix;
	node->pref_len = pref_len;
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

/** insert a value into a leaf node */
static inline void insert_into_node(struct radix_head *restrict head,
				    struct radix_node *restrict node,
				    unsigned long key, const void *value)
{
	assert(node_is_leaf(node));
	assert(node_contains_key(node, key));
	head->nentries++;
	node->entries++;
	
	unsigned long index = radix_get_index(node, key);
	assert(!node->children[index].val);
	node->children[index].val = value;
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
					 unsigned long key)
{
	/*
	 * start the new node off with an incrementally shorter prefix.
	 * note that this may be shortened below
	 */
	struct radix_node *path =
		alloc_node(head, get_parent(child), child->prefix,
			   child->pref_len - RADIX_TREE_SHIFT);
	if (!path)
		return NULL;

	/* fix up the new path's prefix length */
	while (!node_contains_key(path, key))
		path->pref_len -= RADIX_TREE_SHIFT;

	assert(node_contains_key(path, child->prefix));
	assert(radix_get_index(path, child->prefix) !=
	       radix_get_index(path, key));

	/* fix up the rest of the tree */
	set_parent(child, path);
	child->parent_index = radix_get_index(path, child->prefix);
	path->children[child->parent_index].node = child;
	path->entries++;

	return path;
}

/** tells radix_tree_walk to behave normally */
#define WALK_FLAG_NONE (0x0)
/**
 * tells radix_tree_walk to allocate nodes that don't exist but need to in
 * order to get to a particular key.
 */ 
#define WALK_FLAG_ALLOC (0x1)
/**
 * tells radix_tree_walk to return the closest node to the node containing
 * a key if no such node exists. "closest" is defined as the deepest existing
 * parent.
 */ 
#define WALK_FLAG_CLOSEST (0x2)

#define FLAG_HAS_BIT(flag, bit) (flag & bit)

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
 */ 
static struct radix_node *
radix_tree_walk(struct radix_head *restrict head,
		struct radix_node *restrict start,
		unsigned long key, int flags)
{
	struct radix_node *path = start ? start : head->root;
	
	/* if the tree is empty, allocate something */
	if (!head->root) {
		if (FLAG_HAS_BIT(flags, WALK_FLAG_ALLOC))
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
			if (!FLAG_HAS_BIT(flags, WALK_FLAG_ALLOC))
				return FLAG_HAS_BIT(flags, WALK_FLAG_CLOSEST)
					? path : NULL;
			
			path = alloc_node(head, path, key,
					  RADIX_LEAF_PREFIX_LEN);
			return path;
		}
		path = child;
	}

	if (node_contains_key(path, key))
		return path;
	else if (!FLAG_HAS_BIT(flags, WALK_FLAG_ALLOC))
		return FLAG_HAS_BIT(flags, WALK_FLAG_CLOSEST) ? path : NULL;
	
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

#define WALK_LR_LEFT (true)
#define WALK_LR_RIGHT (false)

/**
 * \brief Similar to radix_tree_walk, except instead of hunting for the node
 * containing a given key, this function just hunts for the next open slot
 * in a given direction (either left or right, which is what the lr is short
 * for.)
 *
 * \param start         The node to start at.
 * \param start_index   The index within the starting node to start at. Note
 *                      that this is a signed rather than an unsigned integer
 *                      because it is meaningful to specify a start index of -1.
 * \param left          True if the function should search left, false to go
 *                      right.
 * \param next_index    The index in the returned node that the walk ended on.
 *
 * \return A node containing the next value in the tree, or NULL if there is no
 * next value
 */
static struct radix_node *
radix_tree_walk_lr(struct radix_node *start, int start_index,
		   bool left, unsigned int *next_index)
{
	assert(start_index <= RADIX_TREE_CHILDREN);
	
	struct radix_node *node = start;
	int index = start_index;
	
	while (node) {
		/* search for a non-null child */
		struct radix_node *child = NULL;
		if (left) {
			for (; index >= 0; index--) {
				child = node->children[index].node;
				if (child)
					break;
			}
		} else {
			for (; index < RADIX_TREE_CHILDREN; index++) {
				child = node->children[index].node;
				if (child)
					break;
			}
		}
		
		/* we found a child: return it if it's a leaf or keep searching */
		if (child) {
			if (node_is_leaf(node)) {
				*next_index = index;
				return node;
			}
			index = left ? RADIX_TREE_CHILDREN - 1 : 0;
			node = child;
		}
		/* otherwise grab the parent and go from there */
		else {
			index = node->parent_index + (left ? -1 : 0);
			node = get_parent(node);
		}
	}

	/* there was no next slot */
	*next_index = 0;
	return NULL;
}

/*
 * normally we'd want to write this itteratively instead of recursively,
 * but the stack space used by each call is pretty minimal, (only 3
 * local variable really should be saved) and radix trees are short
 * (16 nodes deep at most, 11 currently on 64 bit platforms), so stack usage
 * should be less than 512 bytes, which isn't too bad.
 *
 * That being said, TODO: rewrite itteratively. function calls are expensive,
 * and so is stack space.
 */
static void destroy_node(struct radix_node *restrict node,
			 void (*dtor)(void *node, void *private),
			 void *restrict private)
{
	bool is_leaf = node_is_leaf(node);
	for (unsigned long i = 0; i < RADIX_TREE_CHILDREN; i++) {
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
		   void (*dtor)(void *node, void *private),
		   void *restrict private)
{
	destroy_node(head->root, dtor, private);
	head->nnodes = 0;
	head->nentries = 0;
	head->root = NULL;
}

bool radix_insert(struct radix_head *head, unsigned long key,
		  const void *value)
{
	assert(value);

	struct radix_node *node;
	node = radix_tree_walk(head, NULL, /* start at root */
			       key, WALK_FLAG_ALLOC);
	if (!node)
		return false;

	insert_into_node(head, node, key, value);
	return true;
}

void radix_delete(struct radix_head *restrict head, unsigned long key,
		  const void **restrict out)
{
	struct radix_node *node;
	node = radix_tree_walk(head, NULL,  /* start at root */
			       key, WALK_FLAG_NONE);
	if (!node)
		return;

	unsigned int index = radix_get_index(node, key);
	if (!node->children[index].val)
		return;

	if (out)
		*out = node->children[index].val;
	node->children[index].val = NULL;

	head->nentries--;
	node->entries--;

	while (node->entries == 0) {
		struct radix_node *parent = get_parent(node);
		index = node->parent_index;
		head->nnodes--;

		free(node);

		if (!parent)
			break;
		parent->children[index].node = NULL;
		parent->entries--;
		node = parent;
	}
}

bool radix_lookup(struct radix_head *restrict head, unsigned long key,
		  const void **restrict result)
{
	struct radix_node *node;
	node = radix_tree_walk(head, NULL, /* start at root */
			       key, WALK_FLAG_NONE);

	if (!node)
		return false;

	unsigned int i = radix_get_index(node, key);
	const void *val = node->children[i].val;
	if (!val)
		return false;
	
	if (result)
		*result = val;
	return true;
	
}

static inline void
__radix_cursor_begin_end(struct radix_head *restrict head,
			 radix_cursor_t *restrict cursor,
			 bool begin)		      
{
	if (!head->root)
		return;

	unsigned int index;
	struct radix_node *node =
		radix_tree_walk_lr(head->root, 
				   begin ? 0 : RADIX_TREE_CHILDREN - 1,
				   begin ? WALK_LR_RIGHT : WALK_LR_LEFT,
				   &index);
	cursor->owner = head;
	cursor->node = node;
	cursor->key = node_index_to_key(node, index);
}

void radix_cursor_begin(struct radix_head *restrict head,
			radix_cursor_t *restrict cursor)
{
	__radix_cursor_begin_end(head, cursor, true);
}

void radix_cursor_end(struct radix_head *restrict head,
		      radix_cursor_t *restrict cursor)
{
	__radix_cursor_begin_end(head, cursor, false);
}

static inline bool __radix_cursor_next_prev(radix_cursor_t *cursor, bool next)
{
	if ((next && cursor->key >= RADIX_KEY_MAX) 
	    || cursor->key <= RADIX_KEY_DIFF)
		return false;
	
	cursor->key += next ? RADIX_KEY_DIFF : -RADIX_KEY_DIFF;
	cursor->node = radix_tree_walk(cursor->owner, cursor->node,
				       cursor->key, WALK_FLAG_CLOSEST);
	assert(cursor->node);
	return true;
}

bool radix_cursor_next(radix_cursor_t *cursor)
{
	return __radix_cursor_next_prev(cursor, true);
}

bool radix_cursor_prev(radix_cursor_t *cursor)
{
	return __radix_cursor_next_prev(cursor, false);
}

static inline bool __radix_cursor_next_prev_valid(radix_cursor_t *cursor,
						  bool next)
{
	unsigned int start_index = radix_get_index(cursor->node, cursor->key) 
		                       + (next ? 1 : -1);
	unsigned int index;
	struct radix_node *node =
		radix_tree_walk_lr(cursor->node, start_index,
				   next ? WALK_LR_RIGHT : WALK_LR_LEFT,
				   &index);
	if (!node)
		return false;

	cursor->node = node;
	cursor->key = node_index_to_key(node, index);
	return true;
}

bool radix_cursor_next_valid(radix_cursor_t *cursor)
{
	return __radix_cursor_next_prev_valid(cursor, true);
}

bool radix_cursor_prev_valid(radix_cursor_t *cursor)
{
	return __radix_cursor_next_prev_valid(cursor, false);
}

static inline bool __radix_cursor_next_prev_alloc(radix_cursor_t *cursor,
						  bool next)
{
	if ((next && cursor->key >= RADIX_KEY_MAX) 
	    || cursor->key <= RADIX_KEY_DIFF)
		return false;

	unsigned long next_key = cursor->key + (next ? RADIX_KEY_DIFF 
						     : -RADIX_KEY_DIFF);
	struct radix_node *node = radix_tree_walk(cursor->owner, cursor->node,
						  next_key, WALK_FLAG_ALLOC);
	if (!node)
		return false;

	cursor->node = node;
	cursor->key = next_key;
	return true;
}

bool radix_cursor_next_alloc(radix_cursor_t *cursor)
{
	return __radix_cursor_next_prev_alloc(cursor, true);
}

bool radix_cursor_prev_alloc(radix_cursor_t *cursor)
{
	return __radix_cursor_next_prev_alloc(cursor, false);
}

unsigned long radix_cursor_seek(radix_cursor_t *cursor, unsigned long seekdst,
				bool forward)
{
	unsigned long actual = seekdst;
	if (forward == RADIX_SEEK_FORWARD) {
		if (!uladd_ok(cursor->key, seekdst))
			actual = RADIX_KEY_MAX - cursor->key;
		actual &= RADIX_KEY_MASK;
		cursor->key += actual;
	} else {
		if (!ulsub_ok(cursor->key, seekdst))
			actual = cursor->key;
		actual &= RADIX_KEY_MASK;
		cursor->key -= actual;
	}
	
	cursor->node = radix_tree_walk(cursor->owner, cursor->node,
				       cursor->key, WALK_FLAG_CLOSEST);
	return actual;
}

bool radix_cursor_has_entry(const radix_cursor_t *cursor)
{
	struct radix_node *n = cursor->node;
	unsigned int i = radix_get_index(n, cursor->key);
	return node_is_leaf(n) && n->children[i].val;
}

const void *radix_cursor_read(radix_cursor_t *cursor)
{
	struct radix_node *n = cursor->node;
	if (!n)
		return NULL;

	/*
	 * if the cursor is sitting on a non-leaf node, the key it indexes
	 * may still exist (we just have to walk to get to the leaf)...
	 * This can only happen if the tree was modified since the last time
	 * the cursor was moved.
	 */
	if (!node_is_leaf(n)) {
		n = radix_tree_walk(cursor->owner, n, cursor->key,
				    WALK_FLAG_CLOSEST);
		cursor->node = n;
		if (!node_is_leaf(n))
			return NULL;
	}

	unsigned int i = radix_get_index(n, cursor->key);
	return n->children[i].val;
}

bool radix_cursor_write(radix_cursor_t *restrict cursor,
			const void *value, const void **restrict old)
{
	struct radix_node *node = cursor->node;
	if (!node_is_leaf(node)) {
		node = radix_tree_walk(cursor->owner, node, cursor->key, 
				       WALK_FLAG_ALLOC);
		if (!node)
			return false;
		cursor->node = node;
	}

	unsigned int index = radix_get_index(node, cursor->key);
	if (old)
		*old = node->children[index].val;
	node->children[index].val = value;
	return true;
}

unsigned long radix_cursor_read_block(const radix_cursor_t *restrict cursor,
				      const void **restrict result,
				      unsigned long size)
{
	struct radix_node *node = cursor->node;
	unsigned long key = cursor->key;

	/* if the cursor isn't at a leaf, try to walk to one */
	if (!node_is_leaf(node)) {
		node = radix_tree_walk(cursor->owner, node, key,
				       WALK_FLAG_NONE);
		if (!node)
			return 0;
	}

	assert(node_contains_key(node, key));

	unsigned long res_idx;
	unsigned int node_idx = radix_get_index(node, key);
	for (res_idx = 0; res_idx < size; res_idx++,
             node_idx++, key += RADIX_KEY_DIFF) {

		/* if we're at the end of a node, go to the next one */
		if (node_idx == RADIX_TREE_CHILDREN) {
			node = radix_tree_walk(cursor->owner, node, key,
					       WALK_FLAG_NONE);
			if (!node)
				break;
			node_idx = 0;
		}

		const void *val = node->children[node_idx].val;
		if (!val)
			break;
		
		result[res_idx] = val;

		/* if we were at the last key, we have to be done */
		if (key == RADIX_KEY_MAX)
			break;
	}
	return res_idx;
}

unsigned long radix_cursor_write_block(const radix_cursor_t *restrict cursor,
				       const void **src, const void **dst,
				       unsigned long size)
{
	struct radix_node *node = cursor->node;
	unsigned long key = cursor->key;

	/*
	 * if the cursor isn't at a leaf, we need to allocate nodes to get to a
	 * leaf
	 */
	if (!node_is_leaf(node)) {
		node = radix_tree_walk(cursor->owner, node, key,
				       WALK_FLAG_ALLOC);
		if (!node)
			return 0;
	}

	assert(node_contains_key(node, key));

	unsigned long src_idx;
	unsigned int node_idx = radix_get_index(node, key);
	for (src_idx = 0; src_idx < size; src_idx++,
             node_idx++, key += RADIX_KEY_DIFF) {
		
		/* if we're at the end of a node, go to the next one */
		if (node_idx == RADIX_TREE_CHILDREN) {
			node = radix_tree_walk(cursor->owner, node, key,
					       WALK_FLAG_ALLOC);
			if (!node)
				break;
			node_idx = 0;
		}

		assert(node_is_leaf(node) && node_contains_key(node, key));

		const void *old_val = node->children[node_idx].val;
		node->children[node_idx].val = src[src_idx];
		if (dst)
			dst[src_idx] = old_val;

		/* if we didn't kick anything out, update counters */
		if (!old_val) {
			cursor->owner->nentries++;
			node->entries++;
		}

		/* if we were at the last key, we have to be done */
		if (key == RADIX_KEY_MAX)
			break;
	}
	return src_idx;
}
