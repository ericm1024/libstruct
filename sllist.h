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
 * sllist.h
 *
 * Header file for a linked list implementation.
 */

#ifndef SLLIST_H
#define SLLIST_H 1

typedef struct elem {
    void* data;
    struct elem* next;
} sllist_t;

typedef struct {
    sllist_t* first;
    size_t length;
} sllist_head_t;

/* c'tor and d'tor */
void sllist_init(sllist_head_t* head);
void sllist_destroy(sllist_head_t* head);

/* element access */
bool sllist_push_front(sllist_head_t* head, void* elem);
void* sllist_pop_front(sllist_head_t* head);

#endif /* SLLIST_H */
