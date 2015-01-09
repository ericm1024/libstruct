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
 * dllist.h
 *
 * This is a header file for a doubly linked list implementation.
 */

#ifndef DLLIST_H
#define DLLIST_H 1

#include <stdbool.h>
#include <stddef.h>

typedef struct elem {
    struct elem* prev;
    struct elem* next;
    void* data;
} dllist_t;

typedef struct head {
    dllist_t* first;
    dllist_t* last;
    size_t length;
} dllist_head_t;

/* constructors and destructors */

/* Initializes a linked list.
 * list shall not be NULL
 */ 
void dllist_init(dllist_head_t* head);

/* Destroys (frees) all the elements in a list. Does NOT free the actual
 * list or the data in the list. This is the caller's job.
 * head shall not be null. free_data may be null.
 */
void dllist_destroy(dllist_head_t* head);

/* front & back element access */

/* Inserts an element at the beginning of the list. Returns 0 on
 * success, 1 on failure. (i.e. no memory). head shall not be null.
 */
int dllist_push_front(dllist_head_t* head, void* elem);

/* Inserts an element at the end of the list. Returns 0 on sucess,
 * 1 on failure (i.e. no memory). head shall not be null.
 */ 
int dllist_push_back(dllist_head_t* head, void* elem);

/* Removes and returns the first element.
 * Head shall not be null. 
 */ 
void* dllist_pop_front(dllist_head_t* head);

/* Removes and returns the last element.
 * Head shall not be null. 
 */
void* dllist_pop_back(dllist_head_t* head);

#endif /* DLLIST_H */
