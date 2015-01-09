/**
 * \file avl_test.c
 *
 * \author Eric Mueller
 *
 * \brief Test suite for functions defined in avl_tree.h
 */

#include "avl_tree.h"
#include "test.h"
#include <stdbool.h>
#include <stddef.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>

size_t count_nodes(avl_node_t *n)
{
	if (!n)
		return 0;
	else
		return 1 + count_nodes(n->children[0])
			+ count_nodes(n->children[1]);
}

size_t height(avl_node_t *n)
{
	if (!n)
		return 0;
	else {
		size_t lheight = height(n->children[0]);
		size_t rheight = height(n->children[1]);
		return lheight > rheight ? lheight + 1 : rheight + 1;
	}
}

bool valid_node(avl_head_t *hd, avl_node_t *n)
{
	if (!n)
		return true;
	
	short bf = height(n->children[1]) - height(n->children[0]);
	bool ret = (bf == n->balance);
	
	if (n->parent) {
		if (n->parent->children[0] == n)
			ret = n->cradle == 0 ? ret : false;
		else if (n->parent->children[1] == n)
			ret = n->cradle == 1 ? ret : false;
		else
			ret = false;
	}

	if (n->children[0])
		ret = hd->cmp((void *)((uintptr_t)n->children[0] - hd->offset),
			      (void*)((uintptr_t)n - hd->offset)) == -1 ? ret : false;
	if (n->children[1])
		ret = hd->cmp((void*)((uintptr_t)n->children[1] - hd->offset),
			      (void*)((uintptr_t)n - hd->offset)) == 1 ? ret : false;
	if (n->children[0] && n->children[1])
		ret = hd->cmp((void*)((uintptr_t)n->children[0] - hd->offset),
			      (void*)((uintptr_t)n->children[1] - hd->offset))
			== -1 ? ret : false;
	
	ret = valid_node(hd, n->children[0]) ? ret : false;
	ret = valid_node(hd, n->children[1]) ? ret : false;
	return ret;
}

void assert_is_valid_tree(avl_head_t *hd)
{
	ASSERT_TRUE(hd->n_nodes == count_nodes(hd->root),
		"is_valid_avl_tree: hd->n_nodes is wrong.\n");
	ASSERT_TRUE(valid_node(hd, hd->root),
		    "is_valid_avl_tree: hd->root is not a valid node.\n");
}

typedef struct {
	int x;
	avl_node_t avl;
} test_t;

void print_node(avl_node_t *n, size_t offset)
{
	if (!n) {
		printf("-");
		return;
	}

	test_t *data = (test_t *)((uintptr_t)n - offset);
	printf("(");
	print_node(n->children[0], offset);
	printf(", %i, ", data->x);
	print_node(n->children[1], offset);
	printf(")");
}

void print_tree(avl_head_t *t)
{
	print_node(t->root, t->offset);
	printf("\n");
}


/* 0 on sucess, 1 on failure. takes test and control */
/*
static int point_equal(test_t *t, test_t *c,
		       const char *msg)
{
	if (t->x == c->x) {
		return 0;
	} else {
		fprintf(OUT_FILE, "%s", msg);
		return 1;
	}
}

// generate a random point
static void rand_point(test_t *p)
{
	*p = (test_t) {rand(),(avl_node_t){NULL, {NULL, NULL}, 0, 0}};
}

// copy the provided point into a new point (allocates memory)
static test_t *copy_point(test_t *src)
{
	test_t *copy = (test_t *)malloc(sizeof(test_t));
	if (!copy) {
		fprintf(stderr, "copy_point: failed to allocate new point\n");
		return NULL;
	}
	*copy = *src;
	return copy;
}

static void mutate_point(test_t *p)
{
	p->x /= 5;
}
*/

int point_cmp(void *lhs, void *rhs)
{
	int rx = ((test_t*)rhs)->x;
	int lx = ((test_t*)lhs)->x;

	if (lx < rx)
		return -1;
	else if (lx > rx)
		return 1;
	else
		return 0;
}


/**** tests ****/

static const size_t n = 50;

void test_insert()
{
	AVL_TREE(t, &point_cmp, test_t, avl);
	test_t data[n*2];
	
	for (size_t i = 0; i < n; i++) {
		data[i].x = rand();
		avl_insert(&t, (void*)&data[i]);
		assert_is_valid_tree(&t);
	}

	for (size_t i = 0; i < n; i++) {
		void *e = avl_find(&t, (void*)&data[i]);
		ASSERT_TRUE( e == &data[i],
			"test_basic: error. could not find inserted element.\n");
	}

	for (size_t i = n; i < 2*n; i++) {
		void *e = avl_find(&t, (void*)&data[i]);
		ASSERT_TRUE(e == NULL,
			    "test_basic: error. found element in tree"
			    " that was not inserted.");
	}

	for (size_t i = 0; i < n; i++) {
		avl_delete(&t, (void*)&data[i]);
		assert_is_valid_tree(&t);
		ASSERT_TRUE(avl_find(&t, (void*)&data[i]) == NULL,
			    "test_basic: error. found element after deleting it.");
	}
}

void test_delete()
{
	AVL_TREE(t, &point_cmp, test_t, avl);
	test_t data[n*2];
	
	for (size_t i = 0; i < n; i++) {
		avl_delete(&t, (void*)&data[i]);
		assert_is_valid_tree(&t);
		ASSERT_TRUE(avl_find(&t, (void*)&data[i]) == NULL,
			    "test_basic: error. found element after deleting it.");
	}
}

/**** main ****/

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	srand(time(NULL));
	REGISTER_TEST(test_insert);
	REGISTER_TEST(test_delete);
	return run_all_tests();
}
