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
 * \file types.h
 *
 * \author Eric Mueller
 * 
 * \brief Types for libstruct.
 */

#ifndef STRUCT_TYPES_H
#define STRUCT_TYPES_H 1

#include <stddef.h>
#include <stdint.h>

/* avl tree types */
struct avl_node {
	struct avl_node *parent;        /* parent node */
	struct avl_node *children[2];   /* 0 is left child, 1 is right child */
	short balance;                  /* balance factor. -1, 0, or 1 */
	unsigned short cradle;          /* where am I in my parent? (0 or 1) */
};
typedef struct avl_node avl_node_t;

typedef int (*avl_cmp_t)(void *lhs, void *rhs);

struct avl_head {
	avl_node_t *root;                  /* pointer to the root node */
	size_t n_nodes;                    /* number of nodes in the tree */
	avl_cmp_t cmp;                     /* less than comparator */
	size_t offset;                     /* offset of the avl node in the
					    * enclosing struct */
};
typedef struct avl_head avl_head_t;

/* forward (singly-linked) list types */
struct flist {
	struct flist *next;
};

struct flist_head {
	struct flist *first;
	unsigned long length;
};

/* list (doubly-linked) list types */
struct list {
	struct list *next;
	struct list *prev;
};

struct list_head {
	struct list *first;
	struct list *last;
	size_t length;
};

/* hash table types */
struct bucket;
struct htable {
	size_t size;
	size_t entries;
	uint32_t seed1;
	uint32_t seed2;
	struct bucket *table1;
	struct bucket *table2;
	struct bucket *stash;
};
typedef struct htable htable_t;

/* btree types */
struct btree_node;
struct btree {
        size_t entries;
        struct btree_node *root;
};
typedef struct btree btree_t;

/* bloom filter */
struct bloom {
	long *bits; /* bits array for filter */
	uint64_t *seeds; /* seeds for */
	size_t n; /* target number of elements */
	size_t bsize; /* size of bits array */
	size_t nhash; /* number of hash functions, also size of seeds array */
	double p; /* target false probability */
	size_t nbits; /* actual number of bits in the array */
};
typedef struct bloom bloom_t;

struct rb_node {
	struct rb_node *parent;
	struct rb_node *chld[2];
};
typedef struct rb_node rb_node_t;

typedef long (*rb_cmp_t)(void *lhs, void *rhs);
struct rb_head {
	rb_node_t *root;
	size_t offset; /* offset of rb_nodes in enclosing structs */
	rb_cmp_t cmp;
	size_t nnodes; /* number of nodes in the tree */
};
typedef struct rb_head rb_head_t;
	
#endif /* STRUCT_TYPES_H */
