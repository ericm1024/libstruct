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
#include "llist.h"

/* helpers */

void link(llist_t* prev, llist_t* next)
{
    prev->next = next;
    next->prev = prev;
}

void make_first(llist_head_t* head, llist_t* new_first)
{
    head->length += 1;
    if (head->first) {
	link(new_first, head->first);
    }
    head->first = new_first;
    head->first->prev = NULL;
    if (!head->last) {
	head->last = new_first;
	head->last->next = NULL;
    }
}

void make_last(llist_head_t* head, llist_t* new_last)
{
    head->length += 1;
    if (head->last) {
	link(head->last, new_last);
    }
    head->last = new_last;
    head->last->next = NULL;
    if (!head->first) {
	head->first = new_last;
	head->first->prev = NULL;
    }
}

void remove_first(llist_head_t* head) 
{
    head->length -= 1;
    llist_t* old_first = head->first;

    if (old_first == head->last) { /* if the list only had one element */
	head->first = NULL;
	head->last = NULL;
    } else {
	head->first = old_first->next;
	head->first->prev = NULL;
    }

    free(old_first);
}

void remove_last(llist_head_t* head)
{
    head->length -= 1;
    llist_t* old_last = head->last;

    if (old_last == head->first) { /* if the list only had one element */
	head->first = NULL;
	head->last = NULL;
    } else {
	head->last = old_last->prev;
	head->last->next = NULL;
    }

    free(old_last);
}

llist_t* make_new_node(void* elem)
{
    llist_t* node = malloc(sizeof(llist_t));
    if (node) {
	node->prev = NULL;
	node->next = NULL;
	node->data = elem;
    }
    return node;
}

/* initialization and destruction methods */

llist_head_t* llist_init(void (*free_data)(void* data))
{
    llist_head_t* head;
    head = malloc(sizeof(llist_head_t));
    
    if (head) {
	head->length = 0;
	head->first = NULL;
	head->last = NULL;
	head->free_data = free_data;
    }
    return head;
}

void llist_destroy(llist_head_t* head)
{
#ifdef DEBUG
    assert(head);
#endif
    
    llist_t* node = head->first;
    
    while (node) {
#ifdef DEBUG
	assert(head->length > 0);
#endif	
	/* if we were given a freeing function, use it*/
	if (head->free_data) {
	    head->free_data(node->data);
	}
	llist_t* old = node;
	node = node->next;
	free(old);
    }
    free(head);
    return;
}

/* front & back element access */

int llist_push_front(llist_head_t* head, void* elem)
{
#ifdef DEBUG
    assert(head);
#endif
    llist_t* new_first = make_new_node(elem);

    /* if the allocation failed, bail */
    if (!new_first) {
	return 1;
    }
    
    /* add it to the list */
    make_first(head, new_first);
    return 0;
}

int llist_push_back(llist_head_t* head, void* elem)
{
#ifdef DEBUG
    assert(head);
#endif
    
    llist_t* new_last = make_new_node(elem);

    /* if the allocation failed, bail */
    if (!new_last) {
	return 1;
    }
    
    /* add it to the list */
    make_last(head, new_last);
    return 0;
}

void* llist_pop_front(llist_head_t* head)
{
#ifdef DEBUG
    assert(head);
#endif
    if (!head->first) {
	return NULL;
    }

    /* grab the data from the first node and the next node before we free it */
    void* data = head->first->data;

    /* update the list to reflect the removed node */
    remove_first(head);
    
    return data;
}

void* llist_pop_back(llist_head_t* head)
{
#ifdef DEBUG
    assert(head);
#endif
    if (!head->last) {
	return NULL;
    }

    /* grab the return element and the second to last node */
    void* data = head->last->data;

    /* update the list to reflect the removed node */
    remove_last(head);
    
    return data;
}
