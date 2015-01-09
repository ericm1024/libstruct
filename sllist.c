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

#include <assert.h>
#include <stdlib.h> /* malloc */
#include "sllist.h"

void sllist_init(sllist_head_t* head)
{
#ifdef DEBUG
    assert(head);
#endif
    head->first = NULL;
    head->length = 0;
}

void sllist_destroy(sllist_head_t* head)
{
#ifdef DEBUG
    assert(head);
#endif
    sllist_t* next = head->first;
    while(next) {
	sllize_t* old_next = next;
	next = next->next;
	free(old_next);
    }
}

bool sllist_push_front(sllist_head_t* head, void* elem)
{
#ifdef DEBUG
    assert(head);
#endif    
    sllist_t* new_front = malloc(sizeof(sllist_t));
    if (!new_front) {
	return false;       
    }
    new_front->next = head->first;
    head->length++;
    return true;
}

void* sllist_pop_front(sllist_head_t* head)
{
#ifdef DEBUG
    assert(head);
#endif
    if(!head->first) {
	return NULL;
    }
    void* data = head->first->data;
    sllist_t* new_first = head->first->next;
    free(head->first);
    head->first = new_first;
    head->length--;
    return data;
}
