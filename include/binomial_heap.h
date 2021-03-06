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
 * \file binomial_heap.h
 *
 * \author Eric Mueller
 * 
 * \brief Header file for a binomial heap.
 *
 * \detail Binomial heaps implement a priority queue and support the following
 * operations. (Let n be the number of nodes in the heap).
 *
 *     - insert, in worst case log(n) time and average case constant time
 *     - pop, in log(n) time.
 *     - peak, in constant time.
 *     - merge (two heaps), in log(n) time. (here n is the number of elements
 *       in the larger heap)
 *     - rekey (change the value of a node) in log(n)^2 time.
 *
 * Heaps should be declared with the BINOMIAL_HEAP macro.
 */

#ifndef STRUCT_BINOMIAL_HEAP_H
#define STRUCT_BINOMIAL_HEAP_H 1

#include "list.h"
#include <stddef.h>

/**
 * intrusive binomial tree node. one of these should be in the enclosing
 * data structure (i.e. the structure that you want to be in a heap).
 *
 * All these members are private and you should NEVER TOUCH THEM.
 */ 
struct binom_node {
        /** The parent node of this node. */
        struct binom_node *btn_parent;

        /** The list head for the child list starting with this node */
        struct list_head btn_children;

        /** linkage between sibling nodes */
        struct list btn_link;
};

/**
 * each tree with index i can hold 2^i elems. Thus 0 \leq i < 48 gives
 * us a ceiling of 2^48 - 1 elements in the tree. considering this is
 * purely an in-memory data structure and modern machines only have 46
 * bits or so of physical address space, 48 should be sufficient for now
 */
#define BINOMIAL_HEAP_MAX_TREES 48

/**
 * \brief binomial heap handle -- the primary data structure on which
 * functions in this file operate
 */
struct binomial_heap {
        /** number of elements in the heap */
        unsigned long bh_elems;

        /**
         * comparator function for heap elements. should return < 0 for
         * lhs < rhs, 0 for lhs == rhs, and > 0 for lhs > rhs if the heap
         * should behave as a min heap. Reverse this behavior if you want a
         * max heap.
         */
        int (*bh_cmp)(const struct binom_node *lhs,
                      const struct binom_node *rhs);

        /** pointer to min element of the tree. private data */
        struct binom_node *bh_min;
        
        /**
         * array of binomial trees that make up the heap. tree at index
         * i has order i (i.e. has 2^i nodes)
         */
        struct binom_node *bh_trees[BINOMIAL_HEAP_MAX_TREES];
};

/**
 * \brief declare and define a binomial heap
 * \param name    (token) the name of the heap to declare
 * \param type    (type) the type of the structure this heap will contain
 * \param member  (token) the name of the struct binom_node within
 *                type.
 * \param cmp     comparator function as described in struct binomial_heap
 */
#define BINOMIAL_HEAP(name, cmp)                                        \
        struct binomial_heap name = {                                   \
                .bh_elems = 0,                                          \
                .bh_trees = {0},                                        \
                .bh_cmp = cmp,                                          \
                .bh_min = NULL }

/**
 * \brief remove the minimum element from the heap
 * \param heap   The heap.
 * 
 * \return the minimum element in the heap, or NULL if the heap is empty.
 * \note completes in \Theta(log(n)) time
 */
struct binom_node *
binomial_heap_pop(struct binomial_heap *restrict heap);

/**
 * \brief get the minimum element of a heap but do not remove it from the heap
 * \param heap   The heap.
 * 
 * \return the minimum element in the heap, or NULL if the heap is empty
 *
 * \note Completes in \Theta(1) time.
 *
 * \note Be aware that the node returned by peak need not be the same node
 * returned by pop. For example, if multiple nodes have the same value, pop
 * will remove whichever one happens to be at the root of a tree, while peak
 * will return whichever one was inserted first. It would be nice to avoid this
 * issue, but it would entail a minor increas in the complexity of insert/pop.
 * What this all means is that the following WILL CAUSE MEMORY CORRUPTION:
 *
 *     node = binomial_heap_peak(heap);
 *     // do stuff with node
 *     binomial_heap_pop(heap);
 *     free(node);
 *
 * Consider yourself warned.
 */
static inline struct binom_node *
binomial_heap_peak(const struct binomial_heap *restrict heap)
{
        return heap->bh_min;
}

/**
 * \brief Put a new element into a heap
 * \param heap       The heap.
 * \param insertee   The element to insert.
 * \note it is okay to insert repeated elements.
 * \note runtime is worst case O(log(n)) and average case \Theta(1)
 */
void binomial_heap_insert(struct binomial_heap *restrict heap,
                          struct binom_node *restrict insertee);

/**
 * \brief Merge two heaps.
 * \param heap     The heap to merge into.
 * \param victim   The heap to destroy.
 * \note victim will be empty after this function returns.
 * \note runtime is \Theta(log(n))
 */
void binomial_heap_merge(struct binomial_heap *restrict heap,
                         struct binomial_heap *restrict victim);

/**
 * \brief Tell the heap that the key of a node has changed.
 * \param heap   The heap.
 * \param node   The node whose key has changed.
 * \note Runtime is \Theta(log(n)^2)
 */
void binomial_heap_rekey(struct binomial_heap *restrict heap,
                         struct binom_node *restrict node);

#endif /* STRUCT_BINOMIAL_HEAP_H */
