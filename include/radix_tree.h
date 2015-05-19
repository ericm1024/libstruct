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
 * \file radix_tree.h
 *
 * \author Eric Mueller
 * 
 * \brief Header file for a linux kernel style radix tree.
 *
 * \detail **TODO**
 */

#ifndef STRUCT_RADIX_TREE_H
#define STRUCT_RADIX_TREE_H 1

/**
 * minimum number of bits each level of the tree will use (except possibly
 * the lowest level, if this value does not evenly divide BITS_PER_LONG)
 */
#define RADIX_TREE_SHIFT (6UL)

/**
 * number of bits at the end of a key that we don't use. Currently none, but
 * this may be useful depending on how the tree is to be used.
 */
#define RADIX_KEY_UNUSED_BITS (0UL)

/* "head" of the tree structure. keeps metadata and root pointer for a tree */
struct radix_head {
	/* root of the tree */
	struct radix_node *root;
	
	/* number of nodes in the tree */
	unsigned long nnodes;

	/* number of entries in the tree */
	unsigned long nentries;
};

/*
 * itterator type for radix tree -- this is meant to be an opaque type,
 * it should only be used via the radix_cursor_* api
 */
typedef struct radix_cursor {
	/* tree that this cursor references */
	struct radix_head *owner;

	/* leaf node */
	struct radix_node *node;

	/* key that this cursor corresponds to */ 
	unsigned long key;
} radix_cursor_t;

/**
 * \brief declare and define a radix tree head.
 * \param name   (token) name of the struct radix_head to declare and define.
 */
#define RADIX_HEAD(name)						\
	struct radix_head name = {					\
		.root = NULL,						\
		.nnodes = 0,						\
		.nentries = 0};

/**
 * \brief Declare and define a radix tree cursor.
 * \note You still need to call radix_cursor_begin or radix_cursor_end before
 * you can actually use the cursor.
 */
#define RADIX_CURSOR {				\
		.owner = NULL,			\
		.node = NULL,			\
                .key = 0};

/**
 * \brief destroy a radix tree by freeing the all memory associated with its
 * nodes.
 *
 * \param head   The head of the tree to destroy.
 */
extern void radix_destroy(struct radix_head *restrict head,
			  void (*restrict dtor)(const void *node,
						void *private),
			  void *restrict private);

/**
 * \brief insert a new value at an index
 * 
 * \param head    Head of the tree to insert into.
 * \param index   Index to insert at.
 * \param value   Value to insert.
 *
 * \return true if the insertion succeeds, false if it fails, which can happen
 * if a new node needs to be allocated and the allocation fails.
 */
extern bool radix_insert(struct radix_head *head, unsigned long key,
			 const void *value);

/**
 * \brief delete a value at a given index
 *
 * \param head    Head of the tree to delete from.
 * \param index   Index to delete from.
 * \param out     Where to write the erased value. Can be NULL.
 *
 * \detail If there is no value currently associated with the given index,
 * the tree is not modified.
 */
extern void radix_delete(struct radix_head *restrict head, unsigned long key,
			 const void **restrict out);

/**
 * \brief lookup the value at a given index.
 *
 * \param head    Head of the tree to lookup from.
 * \param index   The index to lookup.
 * \param result  Where to put the looked up value, if one exists.
 *
 * \return true if the index has a value, false if not. If fales, @result is not
 * overwritten.
 */
extern bool radix_lookup(const struct radix_head *restrict head,
			 unsigned long key, const void **restrict result);

/**
 * \brief Initialize a cursor to the index of the first item in the tree.
 *
 * \param head    Pointer to the head of the tree.
 * \param cursor  Pointer to the cursor to initialize.
 *
 * \detail If the tree is empty, the cursor is not modified.
 */
extern void radix_cursor_begin(const struct radix_head *restrict head,
			       radix_cursor_t *restrict cursor);

/**
 * \brief Initialize a cursor to the index of the last item in the tree.
 *
 * \param head    Pointer to the head of the tree.
 * \param cursor  Pointer to the cursor ro initialize.
 *
 * \detail If the tree is empty, the cursor is not modified.
 */
extern void radix_cursor_end(const struct radix_head *restrict head,
			     radix_cursor_t *restrict cursor);

/**
 * \brief Get the key that a cursor indexes.
 * 
 * \param cursor   The cursor.
 * 
 * \return the key indexed by the cursor.
 */
static inline unsigned long radix_cursor_key(const radix_cursor_t *cursor)
{
	return cursor->key;
}

/**
 * \brief Move a cursor to the next slot in the tree.
 *
 * \param cursor   The cursor to move.
 *
 * \return true if the cursor was moved, false if it was already at the last
 * slot in the tree.
 * 
 * \detail Moves a cursor to the exact next slot in the tree, which may or
 * may not be occupied. 
 */
extern bool radix_cursor_next(radix_cursor_t *cursor);

/**
 * \brief Move a cursor to the next valid slot in the tree.
 * 
 * \param cursor   The cursor to move.
 *
 * \return true if the cursor was moved, false if there is no next valid slot.
 */
extern bool radix_cursor_next_valid(radix_cursor_t *cursor);

/**
 * \brief Move a cursor to the next slot in the tree, allocating a node if
 * need be.
 *
 * \param cursor   The cursor to move.
 *
 * \return true if the cursor was moved, false if the allocation failed or
 * if the cursor is already at the last slot in the tree.
 *
 * \detail If allocation fails, the cursor is not moved.
 */
extern bool radix_cursor_next_alloc(radix_cursor_t *cursor);

/**
 * \brief Move a cursor to the previous slot in the tree.
 *
 * \param cursor   The cursor to move.
 *
 * \return true if the cursor was moved, false if it was already at the first
 * slot in the tree.
 * 
 * \detail Moves a cursor to the exact previous slot in the tree, which may or
 * may not be occupied.
 */
extern bool radix_cursor_prev(radix_cursor_t *cursor);

/**
 * \brief Move a cursor to the previous valid slot in the tree.
 * 
 * \param cursor   The cursor to move.
 *
 * \return true if the cursor was moved, false if there is no pervious valid
 * slot.
 */
extern bool radix_cursor_prev_valid(radix_cursor_t *cursor);

/**
 * \brief Move a cursor to the previous slot in the tree, allocating a node if
 * need be.
 *
 * \param cursor   The cursor to move.
 *
 * \return true if the cursor was moved, false if the allocation failed or
 * if the cursor is in the first slot in the tree.
 *
 * \detail If allocation fails, the cursor is not moved.
 */
extern bool radix_cursor_prev_alloc(radix_cursor_t *cursor);


#define RADIX_SEEK_FORWARD (true)
#define RADIX_SEEK_REVERSE (false)

/**
 * \brief Move the index of a cursor by a designated ammount.
 *
 * \param cursor    The cursor to move.
 * \param seekdst   The ammount to move the cursor by.
 * \param forward   Either RADIX_SEEK_FORWARD or RADIX_SEEK_REVERSE
 * 
 * \return the ammount that the cursor was actually moved.
 *
 * \detail The return value may be less than seekdst if seekdst is not a
 * multiple of the chunk size of the tree or if seeking by seekdst would
 * overflow the index range.
 */
extern unsigned long radix_cursor_seek(radix_cursor_t *cursor,
				       unsigned long seekdst,
				       bool forward);

/**
 * \brief Determine if a cursor has a valid entry.
 *
 * \param cursor   The cursor to inspect.
 *
 * \return true if the slot indexed by the cursor has a valid entry.
 */
extern bool radix_cursor_has_entry(const radix_cursor_t *cursor);

/**
 * \brief Read the value indexed by a cursor.
 *
 * \param cursor   The cursor to read.
 *
 * \return The value indexed by a cursor. NULL if the cursor does not
 * index a valid value.
 */
extern const void *radix_cursor_read(const radix_cursor_t *cursor);

/**
 * \brief Write a value at the slot indexed by a cursor.
 *
 * \param cursor   The cursor to write.
 * \param value    The value to write into the tree. Can not be NULL.
 * \param old      If a value is overwritten, the old value is put here.
 *                 Can be NULL if you don't care about the old value.
 *
 * \return true if the write succeeded, false if it did not, i.e. if memory
 * needed to be allocated and it could not be. Note that memory may not always
 * need to be allocated, even if the cursor indexes an empty slot.
 */
extern bool radix_cursor_write(const radix_cursor_t *restrict cursor,
			       const void *value, const void **restrict old);

/**
 * \brief Read a contiguous block of values from the tree.
 *
 * \param cursor   The cursor to read from. Cursor is not moved.
 * \param result   Where to write the results.
 * \param size     Maximum number of elements to read, i.e. number of elements
 *                 in the array pointed to by result.
 *
 * \return The number of values that were read. This may be less than size.
 *
 * \detail This function will only read values in contiguous slots. It stops
 * as soon as it hits an empty slot. Hence why the result may be less than
 * size.
 */
extern unsigned long
radix_cursor_read_block(const radix_cursor_t *restrict cursor,
			const void **restrict result, unsigned long size);

/**
 * \brief Write a block of values to a cursor starting at the cursor.
 *
 * \param cursor  The cursor to write to. Cursor is not moved.
 * \param src     The array of values to write to the tree.
 * \param dst     Where to write the previous values that were overwritten.
 *                Can be NULL. NULLs may be written to this array.
 * \param size    The number of entries in the src and dst arrays. Does not
 *                apply to the dst array if it is NULL.
 *                
 *                
 * \note src and dst may point to the same array, and consequently they are not
 * restrict qualified pointers.
 * 
 * \return The number of values that were read. May be less than size if writing
 * the entire src array would overflow the tree index range or if memory needs
 * to be allocated and allocation fails.
 */
extern unsigned long
radix_cursor_write_block(const radix_cursor_t *restrict cursor,
			 const void **src, const void **dst,
			 unsigned long size);

#endif /* STRUCT_RADIX_TREE_H */
