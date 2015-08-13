/*
 * Copyright 2014 Eric Mueller
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
 * \file avl_test.c
 *
 * \author Eric Mueller
 *
 * \brief Test suite for functions defined in avl_tree.h
 */

#include "avl_tree.h"
#include "test.h"
#include "util.h"
#include <stdbool.h>
#include <stddef.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct {
	uint64_t x;
	struct avl_node avl;
} test_t;

size_t count_nodes(struct avl_node *n)
{
	if (!n)
		return 0;
	else
		return 1 + count_nodes(n->children[0])
			+ count_nodes(n->children[1]);
}

size_t height(struct avl_node *n)
{
	if (!n)
		return 0;
	else {
		size_t lheight = height(n->children[0]);
		size_t rheight = height(n->children[1]);
		return lheight > rheight ? lheight + 1 : rheight + 1;
	}
}

void valid_node(struct avl_head *hd, struct avl_node *n)
{
	if (!n)
		return;
	
	short bf = height(n->children[1]) - height(n->children[0]);
	ASSERT_TRUE(bf == n->balance, "valid_node: bad balance factor.\n");

	if (n->children[0])
		ASSERT_TRUE(hd->cmp(n->children[0], n) <= 0,
			    "valid_node: left child was not less than root.\n");
	if (n->children[1])
		ASSERT_TRUE(hd->cmp(n->children[1], n) >= 0,
			    "valid_node: right child was not greater than root.\n");
	if (n->children[0] && n->children[1])
		ASSERT_TRUE(hd->cmp(n->children[0], n->children[1]) <= 0,
                            "valid_node: left child > right child.\n");
	
	valid_node(hd, n->children[0]);
	valid_node(hd, n->children[1]);
}

void assert_is_valid_tree(struct avl_head *hd)
{
	ASSERT_TRUE(hd->n_nodes == count_nodes(hd->root),
		"is_valid_avl_tree: hd->n_nodes is wrong.\n");
	valid_node(hd, hd->root);
}

void print_node(struct avl_node *n)
{
	if (!n) {
		printf("-");
		return;
	}

	test_t *data = container_of(n, test_t, avl);
	printf("(");
	print_node(n->children[0]);
	printf(", %"PRIu64", ", data->x);
	print_node(n->children[1]);
	printf(")");
}

void print_tree(struct avl_head *t)
{
	print_node(t->root);
	printf("\n");
}

int point_cmp(struct avl_node *lhs, struct avl_node *rhs)
{
        uint64_t lx = container_of(lhs, test_t, avl)->x;
	uint64_t rx = container_of(rhs, test_t, avl)->x;

	if (lx < rx)
		return -1;
	else if (lx > rx)
		return 1;
	else
		return 0;
}


/**** tests ****/

#define n 10000

void test_insert()
{
	AVL_TREE(t, &point_cmp, test_t);
	test_t data[n*2];
	
	for (size_t i = 0; i < n; i++) {
		data[i].x = pcg64_random() % (n/2);
		avl_insert(&t, &data[i].avl);
		assert_is_valid_tree(&t);
		ASSERT_TRUE(t.n_nodes == (i + 1),
			    "test_insert: error. n_nodes is wrong.\n");
	}

	for (size_t i = 0; i < n; i++) {
		struct avl_node *e = avl_find(&t, &data[i].avl);
		ASSERT_TRUE(container_of(e, test_t, avl)->x == data[i].x,
			"test_basic: error. could not find inserted element.\n");
	}

	for (size_t i = n; i < 2*n; i++) {
		data[i].x = pcg64_random() + n; /* initialize to shut up valgrind */
		struct avl_node *e = avl_find(&t, &data[i].avl);
		ASSERT_TRUE(e == NULL,
			    "test_basic: error. found element in tree"
			    " that was not inserted.\n");
	}
}

void test_delete()
{
	AVL_TREE(t, &point_cmp, test_t);
	test_t data[n];

	for (size_t i = 0; i < n; i++) {
		data[i].x = pcg64_random() % (n/2);
		avl_insert(&t, &data[i].avl);
	}
	
	for (size_t i = 0; i < n; i++) {
		avl_delete(&t, &data[i].avl);
		assert_is_valid_tree(&t);
		ASSERT_TRUE(t.n_nodes == n - (i + 1),
			    "test_basic: error. n_nodes is wrong.\n");
	}
        ASSERT_TRUE(t.root == NULL, "root was not null after emptying tree\n");
}

/* avl next */
void test_itterators()
{
	AVL_TREE(t, &point_cmp, test_t);
	test_t data[n];

	for (size_t i = 0; i < n; i++) {
		data[i].x = i;
		avl_insert(&t, &data[i].avl);
	}

	ASSERT_TRUE(avl_first(&t) == &data[0].avl, "test_itterators: avl_first did"
		    " not return first element.\n");
	ASSERT_TRUE(avl_last(&t) == &data[n-1].avl, "test_itterators: avl_last did"
		    " not return last element.\n");

	struct avl_node *node = avl_first(&t);
	for (size_t i = 0; i < n; i++) {
		ASSERT_FALSE(node == NULL, "test_itterators: got null node"
			     " when more nodes were expected.\n");
		ASSERT_TRUE(node == &data[i].avl, "test_itterators: traversed out"
			    " of order.\n");
		if (i > 0) {
			ASSERT_TRUE(avl_prev(node) == &data[i-1].avl,
				    "test_itterators: avl_prev does not give"
				    " previous element.\n");
			ASSERT_TRUE(avl_next(avl_prev(node)) == &data[i].avl,
				    "test_itterators: next of prev does not give"
				    " current node.\n");
		} else {
			ASSERT_TRUE(node == avl_first(&t), "test_itterators:"
				    " first node not equal to avl_first");
			ASSERT_TRUE(avl_prev(node) == NULL,
				    "test_itterators: avl_prev of first element"
				    " does not give NULL.\n");
		}
		if (i < n-1)
			ASSERT_TRUE(avl_prev(avl_next(node)) == &data[i].avl,
				    "test_itterators: prev of next does not give"
				    " current node.\n");
		else {
			ASSERT_TRUE(avl_next(node) == NULL, "test_itterators:"
				    " next of last nodes does not give null.\n");
			ASSERT_TRUE(node == avl_last(&t), "test_itterators:"
				    " last node not equal to avl_last");
		}
		node = avl_next(node);
	}			    
}

/* avl splice */
void test_splice()
{
	AVL_TREE(t, &point_cmp, test_t);
	AVL_TREE(s, &point_cmp, test_t);
	test_t data[n*2];
	
	for (size_t i = 0; i < n; i++) {
		data[i].x = pcg64_random() % (n/2);
		data[i + n].x = pcg64_random() % (n/2);
		avl_insert(&t, &data[i].avl);
		avl_insert(&s, &data[i+n].avl);
	}
	avl_splice(&t, &s);
	
	assert_is_valid_tree(&t);
	ASSERT_TRUE(s.n_nodes == 0, "test_splice: splicee.n_nodes was not zero"
		    " after splicing.\n");
	ASSERT_TRUE(s.root == NULL, "test_splice: splicee.root was not null"
		    " after splicing.\n");

	for (size_t i = 0; i < n*2; i++) {
                struct avl_node *node = avl_find(&t, &data[i].avl);
		ASSERT_TRUE(container_of(node, test_t, avl)->x == data[i].x,
			    "test_splice: could not find element in target tree"
			    " after splicing.\n");
	}
}

/* avl for each */
void test_for_each()
{
	AVL_TREE(t, &point_cmp, test_t);
	test_t data[n];

	for (size_t i = 0; i < n; i++) {
		data[i].x = i;
		avl_insert(&t, &data[i].avl);
	}

	avl_for_each(&t, i) {
                container_of(i, test_t, avl)->x++;
	}
	
	for (size_t i = 0; i < n; i++)
		ASSERT_TRUE(data[i].x == i+1, "test_for_each: data was not"
			    " modified.\n");
}

/* avl for each range */
void test_for_each_range()
{
	AVL_TREE(t, &point_cmp, test_t);
	test_t data[n];

	for (size_t i = 0; i < n; i++) {
		data[i].x = i;
		avl_insert(&t, &data[i].avl);
	}

	avl_for_each_range(&t, i, &data[n/4].avl, &data[n - n/4].avl) {
                container_of(i, test_t, avl)->x++;
	}

	for (size_t i = 0; i < n/4; i++)
		ASSERT_TRUE(data[i].x == i, "test_for_each_range: data out"
			    " of range (before start) was modified.\n");
	for (size_t i = n/4; i < n - n/4; i++)
		ASSERT_TRUE(data[i].x == i+1, "test_for_each_range: data in"
			    " range was not modified.\n");
	for (size_t i = n - n/4; i < n; i++)
		ASSERT_TRUE(data[i].x == i, "test_for_each_range: data out"
			    " of range (after end) was modified.\n");
}

/**** main ****/

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	seed_rng();

	REGISTER_TEST(test_insert);
	REGISTER_TEST(test_delete);
	REGISTER_TEST(test_itterators);
	REGISTER_TEST(test_splice);
	REGISTER_TEST(test_for_each);
	REGISTER_TEST(test_for_each_range);
	return run_all_tests();
}
