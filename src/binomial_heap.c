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
 * \file binomial_heap.c
 *
 * \author Eric Mueller
 * 
 * \brief Implementation of a binomial heap.
 *
 * \detail Conceptually, a binomial heap is a set of binomial trees.
 * Binomial trees are well described in most cs textbooks and in plenty
 * of places on the internet, but they're fairly simple so I'll give the
 * definition here.
 *
 * dfn. A binomial tree of order k is a node with k children wich are
 * binomial trees with order 0...k-1. It follows that a binomial tree
 * of order k has 2^k elements. All children have value <= their parent
 * (as one would expect with a heap).
 *
 * The fundamental operation on a binomal tree is a merge. To merge 2 trees
 * of order k (they must be the same order), we just add the greater of the
 * 2 roots to the children of the lesser. This maintains the heap property
 * and yields a binomial tree of order k + 1.
 * 
 * Back to the heap itself. A binomial heap is just a collection of 
 * binomial trees each of a unique order. To this end, we implement our
 * heap at a static array of trees, with the trees indexed by their order.
 * Again, this works because the orders are unique. Since a tree of order
 * k has 2^k elements, we can hold a heap with n elements in at most
 * log(n) trees, so a small static array is sufficient.
 *
 * Insertion into heaps is fairly simple -- it's equivalent to just merging
 * a tree of order 0 into the array of trees.
 *
 * Deletion is fairly simple as well -- take the minimum element of the heap
 * (which is guarenteed to be the root of some tree), remove it from that
 * tree, and merge each of its subtrees back into the heap.
 *
 * The only tricky part is keeping track of the minimum element of the heap.
 * We accomplish this by just keeping a pointer to it and updating that
 * pointer as we modify the heap. (the pointer allows for O(1) access to the
 * minimum element).
 */

#include "binomial_heap.h"
#include "util.h"
#include <assert.h>
#include <stdbool.h>

/**
 * less than predicate. wraps the heaps comparator function because
 * I can never remember which return value signifies less than and which
 * return value signifies greater than.
 */
static inline bool node_lt(const struct binomial_heap *restrict heap,
                           const struct binomial_tree_node *restrict lhs,
                           const struct binomial_tree_node *restrict rhs)
{
        assert(lhs && rhs);
        return heap->bh_cmp(lhs, rhs) < 0;
}

static inline void node_init(struct binomial_tree_node *node)
{
        node->btn_parent = NULL;
        node->btn_children = (struct list_head) {
                               .first = NULL,
                               .last = NULL,
                               .length = 0,
                               .offset = offsetof(struct binomial_tree_node,
                                                  btn_link) };

        node->btn_link = (struct list) {
                           .next = NULL,
                           .prev = NULL };
}

static inline unsigned long
node_order(const struct binomial_tree_node *node)
{
        return node->btn_children.length;
}

static inline struct binomial_tree_node *
tree_merge(const struct binomial_heap *heap,
           struct binomial_tree_node *tree,
           struct binomial_tree_node *other)
{
        struct binomial_tree_node *parent, *child;
        assert(tree != other);
        assert(node_order(tree) == node_order(other));

        /* lower valued node becomes the parent */
        if (node_lt(heap, tree, other)) {
                parent = tree;
                child = other;
        } else {
                parent = other;
                child = tree;
        }

        child->btn_parent = parent;
        list_push_back(&parent->btn_children, child);
        parent->btn_parent = NULL;
        return parent;
}

/**
 * \brief merge a tree into the heap and
 *
 * \param heap   the heap to merge into
 * \param tree   the tree to merge into the heap
 *
 * \detail This is the meat of binomial heaps -- merging a tree into
 * the set of trees that make up the heap.
 */
static inline void heap_coalesce(struct binomial_heap *restrict heap,
                                 struct binomial_tree_node *restrict tree)
{        
        for (unsigned i = node_order(tree); ; i++) {
                assert(node_order(tree) == i);
                if (!heap->bh_trees[i]) {
                        heap->bh_trees[i] = tree;
                        break;
                }

                tree = tree_merge(heap, heap->bh_trees[i], tree);
                heap->bh_trees[i] = NULL;
        }
}

struct binomial_tree_node *
binomial_heap_pop(struct binomial_heap *restrict heap)
{
        struct binomial_tree_node *min = heap->bh_min;
        struct binomial_tree_node *subtree;

        if (!min)
                return NULL;

        /*
         * we can have multiple elements with the same value, we want the
         * one that is the root of a tree
         */
        while (min->btn_parent)
                min = min->btn_parent;
        
        assert(heap->bh_trees[node_order(min)] == min);
        heap->bh_trees[node_order(min)] = NULL;
        
        /* coalesce all the subtrees of min back into the heap */
        while ((subtree = list_pop_front(&min->btn_children))) {
                subtree->btn_link.prev = NULL;
                subtree->btn_link.next = NULL;
                subtree->btn_parent = NULL;
                heap_coalesce(heap, subtree);
        }

        /* find the new minimum node among all the trees */
        struct binomial_tree_node *new_min = NULL;
        for (unsigned i = 0; i < BINOMIAL_HEAP_MAX_TREES; i++) {
                struct binomial_tree_node *tree = heap->bh_trees[i];
                if (tree && (!new_min || node_lt(heap, tree, new_min)))
                        new_min = tree;
        }

        /* update the heap metadada */
        heap->bh_min = new_min;
        heap->bh_elems--;
        assert(heap->bh_min != min);

        return min;
}

void binomial_heap_insert(struct binomial_heap *restrict heap,
                          struct binomial_tree_node *restrict insertee)
{
        /* sanitize the new node */
        node_init(insertee);

        /* update the heap's metadata */
        heap->bh_elems++;
        if (!heap->bh_min || node_lt(heap, insertee, heap->bh_min))
                heap->bh_min = insertee;
        
        /* add the new node to the heap */
        heap_coalesce(heap, insertee);
}

void binomial_heap_merge(struct binomial_heap *restrict heap,
                         struct binomial_heap *restrict victim)
{
        /* merge all the trees of victim into heap */
        for (unsigned i = 0; i < BINOMIAL_HEAP_MAX_TREES; i++) {
                struct binomial_tree_node *tree = victim->bh_trees[i];
                if (tree)
                        heap_coalesce(heap, tree);
                victim->bh_trees[i] = NULL;
        }

        /* update the metadata of both heap */
        heap->bh_elems += victim->bh_elems;
        if (victim->bh_min) {
                if (!heap->bh_min)
                        heap->bh_min = victim->bh_min;
                else if (node_lt(heap, victim->bh_min, heap->bh_min))
                        heap->bh_min = victim->bh_min;
        }
        
        victim->bh_elems = 0;
        victim->bh_min = NULL;
}

static inline void
node_swap_with_child(struct binomial_heap *restrict heap,
                     struct binomial_tree_node *restrict node,
                     struct binomial_tree_node *restrict child)
{
        assert(child->btn_parent == node);

        /* delete node and child from their sibling lists */
        list_delete(&node->btn_children, child);
        if (node->btn_parent)
                list_delete(&node->btn_parent->btn_children, node);

        /* reparent node and child */
        child->btn_parent = node->btn_parent;
        node->btn_parent = child;

        /* point all children at new parents */
        list_for_each(&child->btn_children, struct binomial_tree_node, n)
                n->btn_parent = node;
        list_for_each(&node->btn_children, struct binomial_tree_node, n)
                n->btn_parent = child;

        /* swap list heads in node and child */
        list_headswap(&node->btn_children, &child->btn_children);

        /* re-add node and child to their parent lists */
        list_push_back(&child->btn_children, node);
        if (child->btn_parent)
                list_push_back(&child->btn_parent->btn_children, child);
        else
                heap->bh_trees[node_order(child)] = child;
}

void binomial_heap_rekey(struct binomial_heap *restrict heap,
                         struct binomial_tree_node *restrict node)
{
        /* move the node upwards, if we need to */
        for (;;) {
                struct binomial_tree_node *parent = node->btn_parent;

                /* no parent or node >= parent? we're done */
                if (!parent || !node_lt(heap, node, parent))
                        break;

                node_swap_with_child(heap, parent, node);
        }

        /* move the node downards, if we need to */
        for (;;) {
                struct binomial_tree_node *min_child = NULL;
                list_for_each(&node->btn_children, struct binomial_tree_node,
                              child) {
                        if (!min_child || node_lt(heap, child, min_child))
                                min_child = child;
                }

                if (!min_child || !node_lt(heap, min_child, node))
                        break;

                node_swap_with_child(heap, node, min_child);
        }

        if (node_lt(heap, node, heap->bh_min))
                heap->bh_min = node;
}
