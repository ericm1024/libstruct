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
 * \file binomial_heap_test.c
 *
 * \author Eric Mueller
 *
 * \brief Tests for a binomial heap.
 *
 * bug:
 *   peaked element was out of order
 *   expcted 1995149855911362, got 0, i is 1
 */

#include "binomial_heap.h"
#include "pcg_variants.h"
#include "test.h"
#include "util.h"

#include <stdbool.h>

static unsigned long test_size = 1000000;

/* data structure used for test heaps */
struct foo {
        unsigned long val;
        struct binomial_tree_node node;
};

int foo_cmp(const struct binomial_tree_node *lhs,
            const struct binomial_tree_node *rhs)
{
        const struct foo *foo_lhs = container_of(lhs, struct foo, node);
        const struct foo *foo_rhs = container_of(rhs, struct foo, node);
        
        if (foo_lhs->val < foo_rhs->val)
                return -1;
        else if (foo_lhs->val > foo_rhs->val)
                return 1;
        else
                return 0;
}

/* count the number of entries in a tree */
unsigned long count_tree_entries(struct binomial_tree_node *tree)
{
        if (tree->btn_children.length == 0)
                return 1;

        unsigned long entries = 0;
        
        list_for_each(&tree->btn_children, struct binomial_tree_node, n) 
                entries += count_tree_entries(n);

        return entries + 1;
}

/* count the number of entries in a heap */
unsigned long count_heap_entries(struct binomial_heap *heap)
{
        unsigned long entries = 0;
        for (unsigned long i = 0; i < BINOMIAL_HEAP_MAX_TREES; i++)
                entries += count_tree_entries(heap->bh_trees[i]);
        
        return entries;
}

/* validate that a heap is empty */
void assert_heap_empty(const struct binomial_heap *heap)
{
        ASSERT_TRUE(heap->bh_elems == 0, "heap->bh_elms was non-zero\n");
        for (unsigned long i = 0; i < BINOMIAL_HEAP_MAX_TREES; i++)
                ASSERT_TRUE(!heap->bh_trees[i],
                            "empty heap had non-null subtree\n");
}

/* validate a binomial tree */
void assert_tree_valid(const struct binomial_tree_node *tree, unsigned order)
{
        ASSERT_TRUE(tree->btn_children.length == order,
                    "length of tree's child list was wrong\n");
        
        if (order == 0) {
                ASSERT_TRUE(!tree->btn_children.first
                            && !tree->btn_children.last,
                            "order 0 node did not have empty child list\n");
                return;
        }
        
        unsigned *children = calloc(order, sizeof *children);
        ASSERT_TRUE(children, "calloc barfed in assert_tree_valid\n");
        
        list_for_each(&tree->btn_children, struct binomial_tree_node, n) {
                unsigned child_order = n->btn_children.length;
                ASSERT_FALSE(children[child_order],
                            "found child of duplicate order\n");
                children[child_order] = true;
                ASSERT_TRUE(n->btn_parent == tree,
                            "child tree node did not point to parent\n");
                ASSERT_TRUE(foo_cmp(tree, n) <= 0,
                            "child node was greater than parent\n");
                assert_tree_valid(n, child_order);
        }
        free(children);
}

/* validate a binomial heap */
void assert_heap_valid(const struct binomial_heap *heap)
{
        unsigned long entries = 0;
        for (unsigned long i = 0; i < BINOMIAL_HEAP_MAX_TREES; i++) {
                if (heap->bh_trees[i]) {
                        entries += 1ULL << i;
                        assert_tree_valid(heap->bh_trees[i], i);
                        ASSERT_TRUE(!heap->bh_trees[i]->btn_parent,
                                    "root node had non-null parent\n");
                }
        }
        ASSERT_TRUE(heap->bh_elems == entries,
                    "heap->bh_entries was wrong\n");
}

int ulong_cmp(const void *lhs_ptr, const void *rhs_ptr)
{
        unsigned long lhs = *(unsigned long *)lhs_ptr;
        unsigned long rhs = *(unsigned long *)rhs_ptr;
        
        if (lhs < rhs)
                return -1;
        else if (lhs > rhs)
                return 1;
        else
                return 0;
}

/* initialize a heap with random values */
void init_heap(struct binomial_heap *heap, unsigned long size,
               unsigned long **values)
{
        *values = malloc(sizeof **values * size);
        ASSERT_TRUE(*values, "malloc barfed\n");
        unsigned long values_idx = 0;
        
        /* insert the rest */
        for (unsigned long i = 0; i < size; i++) {
                struct foo *elem = malloc(sizeof *elem);
                ASSERT_TRUE(elem, "malloc barfed\n");
                elem->val = pcg64_random() % (size/2); /* mod size/2 guarentees repeats */
                binomial_heap_insert(heap, &elem->node);

                (*values)[values_idx++] = elem->val;
        }
        ASSERT_TRUE(values_idx == size, "logic error\n");

        qsort(*values, size, sizeof **values, ulong_cmp);
}

void destroy_heap(struct binomial_heap *heap)
{
        struct binomial_tree_node *node = NULL;
        struct foo *fp;
        for (;;) {
                node = binomial_heap_pop(heap);
                if (!node)
                        break;
                fp = container_of(node, struct foo, node);
                free(fp);
        }
        assert_heap_empty(heap);
}

/* test the declaration macro */
void test_init()
{
        BINOMIAL_HEAP(test, foo_cmp);

        ASSERT_TRUE(test.bh_elems == 0, "bad elems count\n");
        ASSERT_TRUE(test.bh_cmp == foo_cmp, "bad comparator\n");
        for (unsigned long i = 0; i < BINOMIAL_HEAP_MAX_TREES; i++)
                ASSERT_TRUE(!test.bh_trees[i], "non-null tree\n");
}


/* tests begin here */

/*
 * 1. should add the element to the heap
 * 2. resulting heap should be a valid binomial heap.
 * 3. should allow multiple insertions of the same value
 */ 
void test_insert()
{
        unsigned long *values;
        BINOMIAL_HEAP(test, foo_cmp);
        init_heap(&test, test_size, &values);
        assert_heap_valid(&test);
        destroy_heap(&test);
        free(values);
}

/*
 * 1. should remove and return the minimum element of the heap.
 * 2. should return NULL if there was nothing in the heap.
 * 3. resulting heap should be a valid binomial heap.
 */
void test_pop()
{
        unsigned long *values;
        struct binomial_tree_node *n;
        BINOMIAL_HEAP(test, foo_cmp);
        init_heap(&test, test_size, &values);

        for (unsigned long i = 0; i < test_size; i++) {
                n = binomial_heap_pop(&test);
                struct foo *fp = container_of(n, struct foo, node);

                ASSERT_TRUE(n, "pop returned NULL when there should have "
                            "been more elements\n");
                ASSERT_TRUE(fp->val == values[i],
                            "popped element was out of order\n");

                free(fp);
        }
        n = binomial_heap_pop(&test);
        ASSERT_TRUE(!n, "pop returned non-null for empty heap\n");
        assert_heap_empty(&test);
        free(values);
}

/*
 * 1. should return the minimum element of the heap.
 * 2. should return NULL if the heap is empty.
 * 3. should not modify the heap
 */ 
void test_peak()
{
        unsigned long *values;
        struct binomial_tree_node *n;
        BINOMIAL_HEAP(test, foo_cmp);
        init_heap(&test, test_size, &values);

        for (unsigned long i = 0; i < test_size; i++) {
                n = binomial_heap_peak(&test);
                struct foo *fp = container_of(n, struct foo, node);
                
                ASSERT_TRUE(n, "peak returned NULL when there should have "
                            "been more elements\n");
                ASSERT_TRUE(fp->val == values[i],
                            "peaked element was out of order\n");
                if (fp->val != values[i])
                        printf("expcted %lu, got %lu, i is %lu\n", values[i],
                               fp->val, i);

                ASSERT_TRUE(test.bh_elems == test_size - i,
                            "peak modified heap\n");

                /* peak need not return the same value as pop. */
                n = binomial_heap_pop(&test);
                free(container_of(n, struct foo, node));
        }
        n = binomial_heap_peak(&test);
        ASSERT_TRUE(!n, "pop returned non-null for empty heap\n");
        assert_heap_empty(&test);
        free(values);
}

/*
 * 1. all elements of second heap should be merged into first
 * 2. second heap should be emptied after call
 * 3. should work with empty first heap and/or empty second heap
 * 4. resultant heap should be a valid binomial heap
 */ 
void test_merge()
{
        unsigned long *test_values;
        unsigned long *victim_values;
        BINOMIAL_HEAP(test, foo_cmp);
        BINOMIAL_HEAP(victim, foo_cmp);
        init_heap(&test, test_size, &test_values);
        init_heap(&victim, test_size, &victim_values);

        /* merge vicitm into test */
        binomial_heap_merge(&test, &victim);
        assert_heap_valid(&test);
        assert_heap_valid(&victim);
        assert_heap_empty(&victim);

        /* ... and merge test back into victim */
        binomial_heap_merge(&victim, &test);
        assert_heap_valid(&test);
        assert_heap_valid(&victim);        
        assert_heap_empty(&test);

        /* put all the values in one big sorted array */
        unsigned long *all_values = malloc(2*test_size*sizeof *all_values);
        for (unsigned long i = 0; i < test_size; i++) {
                all_values[i] = test_values[i];
                all_values[i + test_size] = victim_values[i];
        }
        qsort(all_values, 2*test_size, sizeof *all_values, ulong_cmp);

        /* validate the resulting heap */
        for (unsigned long i = 0; i < 2*test_size; i++) {
                struct binomial_tree_node *n = binomial_heap_pop(&victim);
                struct foo *fp = container_of(n, struct foo, node);
                
                ASSERT_TRUE(n, "pop returned NULL when there should have "
                            "been more elements\n");
                ASSERT_TRUE(fp->val == all_values[i],
                            "popped element was out of order\n");
                free(fp);
        }
        assert_heap_empty(&victim);

        free(test_values);
        free(victim_values);
        free(all_values);
}

/*
 * 1. changing the value of a key and calling rekey should yield a valid
 *    heap
 */ 
void test_rekey()
{
        BINOMIAL_HEAP(test, foo_cmp);
        struct foo *foo_vals = malloc(test_size * sizeof *foo_vals);
        for (unsigned long i = 0; i < test_size; i++) {
                foo_vals[i].val = pcg64_random();
                binomial_heap_insert(&test, &foo_vals[i].node);
        }
        assert_heap_valid(&test);

        unsigned long nr_rekeys = test_size/10 > 0 ? test_size/10
                                                   : test_size;
        for (unsigned long i = 0; i < nr_rekeys; i++) {
                unsigned long idx = pcg64_random() % test_size;
                uint64_t new = pcg64_random();
                foo_vals[idx].val = new;
                binomial_heap_rekey(&test, &foo_vals[idx].node);
        }
        assert_heap_valid(&test);
        free(foo_vals);
}

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
        seed_rng();

        REGISTER_TEST(test_init);
        REGISTER_TEST(test_insert);
        REGISTER_TEST(test_pop);
        REGISTER_TEST(test_peak);
        REGISTER_TEST(test_merge);
        REGISTER_TEST(test_rekey);
	
	return run_all_tests();
}
