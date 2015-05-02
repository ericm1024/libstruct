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
 * when it needs to. If the set of indices stored by the tree is distributed
 * in such a way that a naive radix tree would have branches that go several
 * nodes without branching, this radix tree will not actually create those
 * nodes unless the need arrises later (i.e. branches need to be created).
 */

#include "radix_tree.h"
#include <limits.h>
#include <stdint.h>


#define BITS_PER_LONG (sizeof(unsigned long)*CHAR_BIT)

/**
 * \brief macro for generating a "match mask". Used to help determine
 * membership of an index within a node.
 *
 * \param len   (unsigned integer less than BITS_PER_LONG) The length of
 *              the mask to declare. len bits starting at the highest bit
 *              are set high, the rest are low. For example, giving a length
 *              of 4 would produce the result 0xFF00_0000_0000_0000 if longs
 *              are 64 bits.
 */ 
#define RADIX_LEN_TO_MASK(len)			\
	(~((1UL << (BITS_PER_LONG - len)) - 1UL))

/* minimum number of bits each level of the tree will use */
#define RADIX_TREE_SHIFT (6UL)

/* number of children per node */
#define RADIX_TREE_SIZE (1UL << RADIX_TREE_SHIFT)

/* tag to mark a node a a leaf. Stored in the node's parent pointer */
#define RADIX_TAG_LEAF ((uintptr_t)0x1)

struct radix_node {
	/*
	 * parent node to this node and tag to denote if current node is
	 * a leaf
	 */
	union parent_union {
		struct radix_node *parent;
		uintptr_t tag;
	} parent;
	
	/*
	 * array of children -- either nodes or values, depending on
	 * whether the tag field of parent has RADIX_TAG_LEAF
	 */
	union child_union {
		struct radix_node *node;
		const void *val;
	} children[RADIX_TREE_SIZE];

	/*
	 * all elements in the subtree rooted at a node match this prefix
	 * up through a certain length from the highest bit. For example,
	 * if the prefix was 0xFA15_0000_0000_0000 and the prefix length
	 * was 16, then all elements in the subtree rooted at the node
	 * have indicies starting with 0xFA15. See RADIX_LEN_TO_MASK.
	 */
	unsigned long prefix;
	unsigned int prefix_len:BITS_PER_LONG;

	/* index in parent */
	unsigned int parent_index:RADIX_TREE_SHIFT;

	/* nr valid children */
	unsigned int entries:RADIX_TREE_SHIFT
};
