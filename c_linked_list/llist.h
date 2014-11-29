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

/* 
 * Eric Mueller
 * November 2014
 * llist.h
 *
 * This is a header file for a doubly linked list implementation.
 */

#ifndef LLIST_H
#define LLIST_H 1

#include <stdbool.h>
#include <stddef.h>

typedef struct elem {
    struct elem* prev;
    struct elem* next;
    void* data;
} llist_t;

typedef struct head {
    llist_t* first;
    llist_t* last;
    size_t length;
    /* deallocator */
    void (*free_data)(void* data);
} llist_head_t;

/* constructors and destructors */

/* Initializes a linked list.
 * Initializes the memory in 'where' if where is not NULL
 * Returns a null pointer if the allocation fails.
 * free_data and where may be a null pointer.
 */ 
llist_head_t* llist_init(void (*free_data)(void* data), llist_head_t* where);

/* Destroys (frees) an entire list. head shall not be null.
 */
void llist_destroy(llist_head_t* head);

/* front & back element access */

/* Inserts an element at the beginning of the list. Returns 0 on
 * success, 1 on failure. (i.e. no memory). head shall not be null.
 */
int llist_push_front(llist_head_t* head, void* elem);

/* Inserts an element at the end of the list. Returns 0 on sucess,
 * 1 on failure (i.e. no memory). head shall not be null.
 */ 
int llist_push_back(llist_head_t* head, void* elem);

/* Removes and returns the first element.
 * Head shall not be null. 
 */ 
void* llist_pop_front(llist_head_t* head);

/* Removes and returns the last element.
 * Head shall not be null. 
 */
void* llist_pop_back(llist_head_t* head);

#endif /* LLIST_H */
