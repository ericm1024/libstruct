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
 * \file btree.c
 *
 * \author Eric Mueller
 *
 * \brief Implementation of a B-tree. 
 */  

#include "btree.h"

/*! Order of the B-tree. */
#define BT_ORDER ((unsigned)32)
/*! Number of keys stored in each btree node */
#define BT_NUM_KEYS (BT_ORDER * 2)
/*! Number of child pointers in each btree node */
#define BT_NUM_CHILDREN (BT_ORDER * 2 + 1)

/*! internal btree node. */
struct bt_node {
        int is_leaf; /*! 1 if the node is a leaf, 0 if it is a body node */
        uint64_t keys[BT_NUM_KEYS]; /*! keys seperating children !*/
        union {  
                const void *leaves[BT_NUM_KEYS]; 
                             /*! data, if the node is a leaf */
                struct bt_node *children[BT_NUM_CHILDREN];
                             /*! child nodes, if the node is a body node */
        };
        struct bt_node *parent;
};
typedef struct bt_node node_t;

static node_t *bt_alloc_node()
{
        return NULL;
}

int btree_init(btree_t *bt)
{
        return 1;
}

void btree_destroy(btree_t *bt)
{

}

int btree_insert(btree_t *bt, uint64_t key, const void *value)
{
        return 1;
}

int btree_contains(btree_t *bt, uint64_t key)
{
        return 1;
}

int btree_getval(btree_t *bt, uint64_t key, const void **val_ptr)
{
        return 1;
}

void btree_remove(btree_t *bt, uint64_t key)
{

}
