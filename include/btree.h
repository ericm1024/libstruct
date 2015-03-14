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
 * \file btree.h
 *
 * \author Eric Mueller
 *
 * \brief Header file for a btree.
 */

#ifndef STRUCT_BTREE_H
#define STRUCT_BTREE_H 1

#include <stddef.h>

/* btree types */
struct btree_node;
typedef struct btree {
	size_t entries;
	struct btree_node *root;
} btree_t;

/**
 * \def B_TREE
 * \brief Declare a btree.
 * \param name  (token) name of the tree to declare
 */
#define B_TREE(name)				\
        btree_t name = {			\
		.entries = 0,			\
		.root = NULL};

/**
 * \fun btree_init
 * \param bt  btree to initialize.
 * \return 0 on success, 1 on failure
 */
extern int btree_init(btree_t *bt);

/**
 * \fun btree_destroy
 * \param bt  btee to destroy
 *
 * frees all the memory allocated to hold the nodes of the tree.
 */
extern void btree_destroy(btree_t *bt);

/**
 * \fun btree_insert
 * \brief Inserts a key-value pair into the tree.
 * \param bt  btree to insert into
 * \param key  key to insert
 * \param value  value to insert
 * \return 0 on sucess, 1 on failure.
 */
extern int btree_insert(btree_t *bt, uint64_t key, const void *value);

/**
 * \fun btree_contains
 * \brief Seach a tree for a given key.
 * \param bt  btree to search in
 * \param key  key to search for
 * \return 0 if the key exists, 1 if it does not.
 */
extern int btree_contains(btree_t *bt, uint64_t key);

/**
 * \fun btree_getval
 * \brief Get the value corresponding to a key
 * \param bt  btree to query
 * \param key  key to search for
 * \param val_ptr  the value indexed by key is written here if one is found.
 * \return 0 if a value corresponding to the given key was found, 1 otherwise.
 */
extern int btree_getval(btree_t *bt, uint64_t key, const void **val_ptr);
   
/**
 * \fun btree_remove
 * \brief Remove a key-value pair from the tree.
 * \detail If the key does not exist in the tree, the tree is unmodified.
 * \param bt  btree to remove from
 * \param key  key to remove.
 */
extern void btree_remove(btree_t *bt, uint64_t key);

#endif /* STRUCT_BTREE_H */
