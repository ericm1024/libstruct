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
 * \file rbtree_test.c
 *
 * \author Eric Mueller
 *
 * \brief Test suite for functions defined in rbtree.h
 */

#include "rbtree.h"
#include "test.h"
#include <stdbool.h>
#include <stddef.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>

#define KRED  "\x1B[31m"
#define RESET "\033[0m"

struct test_struct {
	int x;
	struct rb_node rb;
};

size_t count_nodes(struct rb_node *n)
{
	if (!n)
		return 0;
	else
		return 1 + count_nodes(n->chld[0])
			+ count_nodes(n->chld[1]);
}

unsigned long valid_node(struct rb_head *hd, struct rb_node *n)
{
	if (!n)
		return 1;

	if (n->chld[0])
		ASSERT_TRUE(hd->cmp((void *)((uintptr_t)n->chld[0] - hd->offset),
				    (void*)((uintptr_t)n - hd->offset)) == -1,
			    "valid_node: left child was not less than root.\n");
	if (n->chld[1])
		ASSERT_TRUE(hd->cmp((void*)((uintptr_t)n->chld[1] - hd->offset),
				    (void*)((uintptr_t)n - hd->offset)) == 1,
			    "valid_node: right child was not greater than root.\n");
	if (n->chld[0] && n->chld[1])
		ASSERT_TRUE(hd->cmp((void*)((uintptr_t)n->chld[0] - hd->offset),
			      (void*)((uintptr_t)n->chld[1] - hd->offset))
			    == -1, "valid_node: left child was not less than "
			           "right child.\n");

	/* check for red violations */
	unsigned long black = 0;
	if (((uintptr_t)n->parent & 1UL) == 1UL) {
		if ((struct rb_node*)((uintptr_t)n->parent & ~1))
			ASSERT_TRUE(((uintptr_t)((struct rb_node*)((uintptr_t)n->parent
	               & ~1))->parent & 1UL) == 0, "valid_node: red violation 1\n");
		if (n->chld[0])
			ASSERT_TRUE(((uintptr_t)n->chld[0]->parent & 1UL) == 0,
				    "valid_node: red violation 2\n");
		if (n->chld[1])
			ASSERT_TRUE(((uintptr_t)n->chld[1]->parent & 1UL) == 0,
				    "valid_node: red violation 2\n");
	} else {
		black = 1;
	}

	/* check for black violations */
	unsigned long right_black_height = valid_node(hd, n->chld[0]);
	unsigned long left_black_height = valid_node(hd, n->chld[1]);
	ASSERT_TRUE(right_black_height == left_black_height,
		    "valid_node: black violation.\n");
	
	return black + right_black_height;
}

void print_node(struct rb_node *n, size_t offset)
{
	if (!n) {
		printf("-");
		return;
	}

	struct test_struct *data = (struct test_struct *)((uintptr_t)n - offset);
	printf("(");
	print_node(n->chld[0], offset);
	if (((uintptr_t)n->parent & 1UL) == 1)
		printf(","  KRED "%i" RESET ", ", data->x);
	else
		printf(", %i, ", data->x);
	print_node(n->chld[1], offset);
	printf(")");
}

void print_tree(struct rb_head *t)
{
	print_node(t->root, t->offset);
	printf("\n");
}

void assert_is_valid_tree(struct rb_head *hd)
{
	ASSERT_TRUE(hd->nnodes == count_nodes(hd->root),
		"is_valid_tree: hd->nnodes is wrong.\n");
	valid_node(hd, hd->root);
}

long point_cmp(void *lhs, void *rhs)
{
	int rx = ((struct test_struct*)rhs)->x;
	int lx = ((struct test_struct*)lhs)->x;

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
	RB_TREE(t, &point_cmp, struct test_struct, rb);
	struct test_struct data[n*2];
	
	for (size_t i = 0; i < n; i++) {
		data[i].x = rand();
		rb_insert(&t, (void*)&data[i]);
		assert_is_valid_tree(&t);
		ASSERT_TRUE(t.nnodes == (i + 1),
			    "test_insert: error. n_nodes is wrong.\n");
	}

	for (size_t i = 0; i < n; i++) {
		void *e = rb_find(&t, (void*)&data[i]);
		ASSERT_TRUE( e == &data[i],
			"test_basic: error. could not find inserted element.\n");
	}

	for (size_t i = n; i < 2*n; i++) {
		data[i].x = rand(); /* initialize to shut up valgrind */
		void *e = rb_find(&t, (void*)&data[i]);
		ASSERT_TRUE(e == NULL,
			    "test_basic: error. found element in tree"
			    " that was not inserted.\n");
	}
}

void test_delete()
{
	RB_TREE(t, &point_cmp, struct test_struct, rb);
	struct test_struct data[n];

	for (size_t i = 0; i < n; i++) {
		data[i].x = rand();
		rb_insert(&t, (void*)&data[i]);
	}

	for (size_t i = 0; i < n; i++) {
		rb_erase(&t, (void*)&data[i]);
		assert_is_valid_tree(&t);
		ASSERT_TRUE(rb_find(&t, (void*)&data[i]) == NULL,
			    "test_basic: error. found element after deleting it.\n");
		ASSERT_TRUE(t.nnodes == n - (i + 1),
			    "test_basic: error. n_nodes is wrong.\n");
	}
}


void test_itterators()
{
	RB_TREE(t, &point_cmp, struct test_struct, rb);
	struct test_struct data[n];

	for (size_t i = 0; i < n; i++) {
		data[i].x = i;
		rb_insert(&t, (void*)&data[i]);
	}

	ASSERT_TRUE(rb_first(&t) == &data[0], "test_itterators: rb_first did"
		    " not return first element.\n");
	ASSERT_TRUE(rb_last(&t) == &data[n-1], "test_itterators: rb_last did"
		    " not return last element.\n");

	void *node = rb_first(&t);
	for (size_t i = 0; i < n; i++) {
		ASSERT_FALSE(node == NULL, "test_itterators: got null node"
			     " when more nodes were expected.\n");
		ASSERT_TRUE(node == &data[i], "test_itterators: traversed out"
			    " of order.\n");
		if (i > 0) {
			ASSERT_TRUE(rb_inorder_prev(&t, node) == &data[i-1],
				    "test_itterators: avl_prev does not give"
				    " previous element.\n");
			ASSERT_TRUE(rb_inorder_next(&t, rb_inorder_prev(&t, node)) == &data[i],
				    "test_itterators: next of prev does not give"
				    " current node.\n");
		} else {
			ASSERT_TRUE(node == rb_first(&t), "test_itterators:"
				    " first node not equal to rb_first.\n");
			ASSERT_TRUE(rb_inorder_prev(&t, node) == NULL,
				    "test_itterators: rb_inorder_prev of first element"
				    " does not give NULL.\n");
		}
		if (i < n-1)
			ASSERT_TRUE(rb_inorder_prev(&t, rb_inorder_next(&t, node)) == &data[i],
				    "test_itterators: prev of next does not give"
				    " current node.\n");
		else {
			ASSERT_TRUE(rb_inorder_next(&t, node) == NULL, "test_itterators:"
				    " next of last nodes does not give null.\n");
			ASSERT_TRUE(node == rb_last(&t), "test_itterators:"
				    " last node not equal to rb_last.\n");
		}
		node = rb_inorder_next(&t, node);
	}			    
}


void test_for_each()
{
	RB_TREE(t, &point_cmp, struct test_struct, rb);
	struct test_struct data[n];

	for (size_t i = 0; i < n; i++) {
		data[i].x = i;
		rb_insert(&t, (void*)&data[i]);
	}

	rb_for_each_inorder(&t, struct test_struct, i)
		i->x++;
	
	for (size_t i = 0; i < n; i++)
		ASSERT_TRUE(data[i].x == (int)i+1, "test_for_each: data was not"
			    " modified.\n");
}


void test_for_each_range()
{
	RB_TREE(t, &point_cmp, struct test_struct, rb);
	struct test_struct data[n];

	for (size_t i = 0; i < n; i++) {
		data[i].x = i;
		rb_insert(&t, (void*)&data[i]);
	}

	rb_for_each_range(&t, struct test_struct, i, &data[n/4], &data[n - n/4])
		i->x++;

	for (size_t i = 0; i < n/4; i++)
		ASSERT_TRUE(data[i].x == (int)i, "test_for_each_range: data out"
			    " of range (before start) was modified.\n");
	for (size_t i = n/4; i < n - n/4; i++)
		ASSERT_TRUE(data[i].x == (int)i+1, "test_for_each_range: data in"
			    " range was not modified.\n");
	for (size_t i = n - n/4; i < n; i++)
		ASSERT_TRUE(data[i].x == (int)i, "test_for_each_range: data out"
			    " of range (after end) was modified.\n");
}

void test_postorder_iterate()
{
	RB_TREE(t, &point_cmp, struct test_struct, rb);
	struct test_struct *array[n];

	for (size_t i = 0; i < n; i++) {
		array[i] = malloc(sizeof(struct test_struct));
		ASSERT_TRUE(array[i], "malloc failed!\n");
		array[i]->x = rand();
		rb_insert(&t, (void*)array[i]);
	}

	rb_postorder_iterate(&t, free);
	/* valgrind will catch errors */
}

/**** main ****/

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	srand(time(NULL));
	REGISTER_TEST(test_insert);
	REGISTER_TEST(test_delete);
	REGISTER_TEST(test_itterators);
	REGISTER_TEST(test_for_each);
	REGISTER_TEST(test_for_each_range);
	REGISTER_TEST(test_postorder_iterate);
	return run_all_tests();
}
