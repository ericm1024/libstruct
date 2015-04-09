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
 * \file binary_heap.c
 *
 * \author Eric Mueller
 * 
 * \brief Implementation of a simple binary min heap.
 */

#include "binary_heap.h"
#include <assert.h>
#include <stdlib.h>

/* get the ith key from the heap */
#define HEAP_KEY(hp, i) ((hp)->heap[i].key)

/* get the ith value from the heap */
#define HEAP_VAL(hp, i) ((hp)->heap[i].value)

/* swap the ith and jth k-v pair in the heap */
#define HEAP_SWAP(hp, i, j)			              \
	do {						      \
		struct kv_pair __swap_tmp = (hp)->heap[i];    \
		(hp)->heap[i] = (hp)->heap[j];		      \
		(hp)->heap[j] = __swap_tmp;		      \
	} while (0);

bool binary_heap_init(struct binary_heap *heap, unsigned long capacity)
{
	assert(heap);
	heap->heap = malloc(capacity * sizeof(struct kv_pair));
	if (!heap->heap)
		return false;
	heap->capacity = capacity;
	return true;
}

void binary_heap_destroy(struct binary_heap *heap)
{
	assert(heap);
	heap->end = 0;
	heap->capacity = 0;
	free(heap->heap);
	heap->heap = NULL;
}

bool binary_heap_grow(struct binary_heap *heap, unsigned long new_cap)
{
	assert(new_cap >= heap->capacity);
	assert(heap);

	if (new_cap == heap->capacity)
		return true;

	struct kv_pair *new_heap =
		realloc(heap->heap, new_cap * sizeof (struct kv_pair));
	
	if (!new_heap)
		return false;

	heap->heap = new_heap;
	heap->capacity = new_cap;
	return true;
}

bool binary_heap_shrink(struct binary_heap *heap, unsigned long new_cap)
{
	assert(new_cap < heap->capacity);
	assert(heap);
	
	struct kv_pair *new_heap =
		realloc(heap->heap, new_cap * sizeof (struct kv_pair));
	
	if (new_cap != 0 && !new_heap)
		return false;

	heap->heap = new_heap;
	heap->capacity = new_cap;
	
	return true;
}

void binary_heap_pop(struct binary_heap *heap, unsigned long *key,
		     const void **val)
{
	assert(heap);
	assert(heap->end > 0);
	assert(key);
	assert(val);
	unsigned long i = --heap->end;

	/* remove the top element and swap with last */
	*key = HEAP_KEY(heap, 0);
	*val = HEAP_VAL(heap, 0);
	heap->heap[0] = heap->heap[i];
	
	/*
	 * restore the heap property: walk down the heap and swap the current
	 * key with its minimum child until the heap property is restored.
	 * this is a little tricky because we need to make sure we stay in
	 * bounds
	 */
	i = 0;
	while (i < heap->end) {
		unsigned long left = (i << 1) + 1;
		unsigned long right = (i << 1) + 2;
		/* find index of minimum child */
		unsigned long min = right < heap->end
			&& HEAP_KEY(heap, right) < HEAP_KEY(heap, left)
			? right : left;
		
		if (min < heap->end && HEAP_KEY(heap, min) < HEAP_KEY(heap, i))
			HEAP_SWAP(heap, i, min);
		
		i = min;
	}

	/* shrink if we have sufficient space */
	if (2*heap->end <= heap->capacity) 
		binary_heap_shrink(heap, heap->end);
}

bool binary_heap_insert(struct binary_heap *heap, unsigned long key,
			const void *val)
{
	assert(heap);
	assert(heap->end <= heap->capacity);

	/*
	 * resize if necessary. We resize by a factor of 1.5, which means we
	 * need to check if multiplying by 1.5 actually yields a larger number
	 * (because 1*1.5 == 1)
	 */ 
	if (heap->end == heap->capacity) {
		unsigned long new_cap = heap->capacity + (heap->capacity >> 1);
		new_cap = new_cap == heap->capacity ? new_cap + 1 : new_cap;
		if (!binary_heap_grow(heap, new_cap))
			return false;	
	}

	/* put the key-value pair at the end of the heap */
	unsigned long i = heap->end++;
	HEAP_VAL(heap, i) = val;
	HEAP_KEY(heap, i) = key;

	/*
	 * restore the heap property: walk back up the heap and swap the current
	 * kv-pair with its parent until the parent key is <= the current key.
	 */ 
	while (i > 0) {
		unsigned long parent = (i - 1) >> 1;
		if (HEAP_KEY(heap, parent) <= HEAP_KEY(heap, i))
			break;

		HEAP_SWAP(heap, i, parent);
		i = parent;
	}
	return true;
}

bool binary_heap_merge(struct binary_heap *heap, struct binary_heap *victim)
{	
	/* put the bigger heap into heap */
	if (heap->capacity < victim->capacity) {
		struct binary_heap tmp = *victim;
		*victim = *heap;
		*heap = tmp;
	}
	
	if (!binary_heap_grow(heap, heap->capacity + victim->capacity))
		return false;

	/* merge all the elements of victim into heap */
	for (unsigned long i = 0; i < victim->end; i++)
		/*
		 * we don't check the return value because we already made
		 * sure to allocate enough memory. We assert it for debugging
		 * purposes though.
		 */
		assert(binary_heap_insert(heap, HEAP_KEY(victim,i),
					  HEAP_VAL(victim,i)));
	
	binary_heap_destroy(victim);
	return true;
}
