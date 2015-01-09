/**
 * \file avl_tree_types.h
 *
 * \author Eric Mueller
 * 
 * \brief Types for avl_tree.h
 */

#ifndef STRUCT_AVL_TREE_TYPES_H
#define STRUCT_AVL_TREE_TYPES_H 1

#include <stddef.h>

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
	size_t offset;                  /* offset of the avl node in the
					    * enclosing struct */
};
typedef struct avl_head avl_head_t;


#endif /* STRUCT_AVL_TREE_TYPES_H */
