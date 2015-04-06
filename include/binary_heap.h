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
 * \file binheap.h 
 *
 * \author Eric Mueller
 * 
 * \brief Header file for a binary heap.
 *
 * \detail TODO 
 */

#ifndef STRUCT_BINARY_HEAP_H
#define STRUCT_BINARY_HEAP_H 1

#include "kv_pair.h"
#include <stdbool.h>

/**
 * \brief Simple binary heap structure. Data is stored in key-value pairs of
 * unsigned longs and void * pointers.
 */
struct binary_heap {
	/** number of elements that the heap can currently hold */
	unsigned long capacity;
	/** one-past-the-end index, also number of used elements */
	unsigned long end;
	/** heap allocated heap array (lol) */
	struct kv_pair *heap;
};

/**
 * \brief Declare and define a binary heap. 
 * \param name   (token) the name of the binary heap to declare and define.
 * \note Before using this heap, you must call binary_heap_init to allocate
 * memory.
 */
#define BINARY_HEAP(name)						\
	struct binary_heap name = {.size = 0;				\
	                           .end = 0;                            \
	                           .heap = NULL; };



/* ============================================================== */
/*                         heap operators                         */
/* ============================================================== */

/**
 * \brief Initialize a binary heap by allocating memory.
 * \param heap       The heap to initialize.
 * \param capacity   The number of elements to allocate space for.
 * \return True if sucessfull, false if the allocation fails.
 */
bool binary_heap_init(struct binary_heap *heap, unsigned long capacity);

/**
 * \brief Destroy a binary heap by freeing memory associated with it.
 * \param heap    The heap to destroy.
 */
void binary_heap_destroy(struct binary_heap *heap);

/**
 * \brief Grow a heap. It is generally not necessary to call this function.
 * \param heap       The heap to grow.
 * \param new_cap    The new capacity of the heap in number of elements.
 *                   Needs to be larger.
 * \return true if the resize suceeded, false if memory could not be
 * allocated.
 * \note O(n) complexity, where n is the number of elements in the heap
 */
bool binary_heap_grow(struct binary_heap *heap, unsigned long new_cap);

/**
 * \brief Clear a heap. This does not deallocate any memory, it just
 * marks the heap as having no elements.
 * \parma heap    The heap to clear.
 */
static inline void binary_heap_clear(struct binary_heap *heap)
{
	heap->end = 0;
}

/**
 * \brief Shrink a heap. It is generally not necessary to call this function
 * because binary_heap_pop will shrink itself.
 * \param heap       The heap to shrink.
 * \param new_cap    The new size of the heap.
 * \note O(n) complexity, where n is the number of elements in the heap.
 */
void binary_heap_shrink(struct binary_heap *heap, unsigned long new_cap);



/* ============================================================== */
/*                         heap accessors                         */
/* ============================================================== */

/**
 * \brief Remove the minimum element from the heap.
 * \param heap   The heap to remove the minimum of.
 * \param key    Where the popped key is put.
 * \param val    Where the popped value is put.
 * \note O(log(n)) complexity, where n is the number of elements in the heap.
 */
void binary_heap_pop(struct binary_heap *heap, unsigned long *key,
		     const void **val);

/**
 * \brief Peek at the first kv-pair in the heap
 * \param hp    The heap to peak at.
 * \return Evaluates to minimum struct kv_pair in the heap.
 */
#define BINARY_HEAP_PEEK(hp) ((hp)->heap[0])

/**
 * \brief Add a new element to the heap.
 * \param heap   The heap to insert into.
 * \param key    The key to insert.
 * \param val    The value to insert.
 * \return True if the insertion succeeded, false if memory could not
 * be allocated.
 * \note O(log(n)) complexity
 */
bool binary_heap_insert(struct binary_heap *heap, unsigned long key,
			const void *val);

/**
 * \brief Merge two binary heaps.
 * \param heap     The heap to merge into.
 * \param victim   The heap to merge from.
 * \return true if the merge succeeded, false if memory allocation failed.
 * \note O(m*log(n)) complexity, where m is the number of elements in the
 * smaller heap and n is the number of elements in the larger heap.
 */
bool binary_heap_merge(struct binary_heap *heap, struct binary_heap *victim);

#endif /* STRUCT_BINARY_HEAP_H */
