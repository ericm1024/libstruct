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

#include "llist.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

/* an arbitrary number of elements to run some tests with */
#define MULTI 100

/* helper functions */

llist_head_t* make_char_list()
{
    return llist_init(NULL);
}

/* we use characters as pesudo data instead of pointed-to things because
 * they will (hopefully) make it easier to detect memory leaks in the actual
 * code
 */
void* get_char()
{
    /* using % means we're not getting a truly random distribution,
     * but it's close enough for what we're doing */
    unsigned char random =
	(unsigned char) (rand() % 256);
    
    return (void*)random;
}

struct point {
    int x;
    int y;
    int z;
};

void* get_point()
{
    struct point* pt = malloc(sizeof(struct point));
    if (!pt) {
	fprintf(stdout, "llist-test: get_point: malloc failed. bailing.\n");
	exit(1);
    }
    pt->x = rand();
    pt->y = rand();
    pt->z = rand();
    return pt;
}

void dealloc_point(void* pnt)
{
    free(pnt);
}
    
llist_head_t* make_point_list()
{
    return llist_init(dealloc_point);
}

/*****                   *
 ****                   **
 ***       tests       ***
 **                   ****
 *                   *****/

/**************** c'tor and d'tor test *******************/

/* test creating and destroying an empty list */
int test_ctor_dtor_empty(llist_head_t*(*test_ctor)())
{
    int errc = 0;
    
    llist_head_t* head = test_ctor();
    if (!head) {
	fprintf(stdout, "test_ctor_dtor_empty: failed. got null pointer.\n");
	errc++;
	/* returning here leaks memory but whatever, its a test, and 
	 * if you get here you failed the test anyway
	 */
	return errc; 
    }
    if (head->first != NULL) {
	fprintf(stdout, "test_ctor_dtor_empty: failed. first is not null.\n");
	errc++;
    }
    if (head->last != NULL) {
	fprintf(stdout, "test_ctor_dtor_empty: failed. last is not null.\n");
	errc++;
    }
    if (head->length != 0) {
	fprintf(stdout, "test_ctor_dtor_empty: failed. length is not 0.\n");
	errc++;
    }
    
    llist_destroy(head);
    /* we don't really have a way to check if that failed.. use valgrind */
    return errc;
}

/******************** push front tests ********************/

/* call push_front once */
int test_push_front_1(llist_head_t*(*test_ctor)(), void*(*get_elem)())
{
    llist_head_t* head = test_ctor();
    void* elem = get_elem();
    llist_push_front(head, elem);

    int errc = 0 ;
    if (head->first->data != elem) {
	fprintf(stdout,
		"test_push_front_1: failed. head->first->data != elem.\n");
	errc++;
    }
    if (head->last->data != elem) {
	fprintf(stdout,
		"test_push_front_1: failed. head->last->data != elem.\n");
	errc++;
    }
    if (head->length != 1) {
	fprintf(stdout, "test_push_front_1: failed. head->length != 1.\n");
	errc++;
    }
    if (head->first->prev != NULL) {
	fprintf(stdout,
		"test_push_front_1: failed. head->first->prev != NULL.\n");
	errc++;
    }
    if (head->last->next != NULL){
	fprintf(stdout,
		"test_push_front_1: failed. head->first->prev != NULL.\n");
	errc++;
    }
    if (head->first != head->last) {
	fprintf(stdout,
		"test_push_front_1: failed. head->first != head->last.\n");
	errc++;
    }
    llist_destroy(head);
    return errc;
}

/* call push_front MULTI times */
int test_push_front_multi(llist_head_t*(*test_ctor)(), void*(*get_elem)())
{
    llist_head_t* head = test_ctor();
    int errc = 0 ;
    llist_t** node = &head->first;
    for (size_t i = 0; i < MULTI; i++) {
	
	void* elem = get_elem();
	llist_push_front(head, elem);
	
	if ((*node)->data != elem) {
	    fprintf(stdout,
		    "test_push_front_multi: failed. (*node)->data != elem.\n");
	    errc++;
	}
	if (head->length != i+1) {
	    fprintf(stdout, "test_push_front_multi: failed. bad length.\n");
	    errc++;
	}
	if (head->first->prev != NULL) {
	    fprintf(stdout,
	      "test_push_front_multi: failed. head->first->prev != NULL.\n");
	    errc++;
	}
	if (head->last->next != NULL){
	    fprintf(stdout,
	      "test_push_front_multi: failed. head->first->prev != NULL.\n");
	    errc++;
	}
    }
    
    llist_destroy(head);
    return errc;
}

/******************** push back tests *********************/

/* call push_back once */
bool test_push_back_1(llist_head_t*(*test_ctor)(), void*(*get_elem)())
{
    llist_head_t* head = test_ctor();
    void* elem = get_elem();
    llist_push_back(head, elem);

    int errc = 0 ;
    if (head->first->data != elem) {
	fprintf(stdout,
		"test_push_back_1: failed. head->first->data != elem.\n");
	errc++;
    }
    if (head->last->data != elem) {
	fprintf(stdout,
		"test_push_back_1: failed. head->last->data != elem.\n");
	errc++;
    }
    if (head->length != 1) {
	fprintf(stdout, "test_push_back_1: failed. head->length != 1.\n");
	errc++;
    }
    if (head->first->prev != NULL) {
	fprintf(stdout,
		"test_push_back_1: failed. head->first->prev != NULL.\n");
	errc++;
    }
    if (head->last->next != NULL){
	fprintf(stdout,
		"test_push_back_1: failed. head->first->prev != NULL.\n");
	errc++;
    }
    if (head->first != head->last) {
	fprintf(stdout,
		"test_push_back_1: failed. head->first != head->last.\n");
	errc++;
    }
    
    llist_destroy(head);
    return errc;
}

/* call push_back MULTI times */
bool test_push_back_multi(llist_head_t*(*test_ctor)(), void*(*get_elem)())
{
    llist_head_t* head = test_ctor();
    int errc = 0 ;
    llist_t** node = &head->last;
    for (size_t i = 0; i < MULTI; i++) {
	
	void* elem = get_elem();
	llist_push_back(head, elem);
	
	if ((*node)->data != elem) {
	    fprintf(stdout,
		    "test_push_back_multi: failed. (*node)->data != elem.\n");
	    errc++;
	}
	if (head->length != i+1) {
	    fprintf(stdout, "test_push_back_multi: failed. bad length.\n");
	    errc++;
	}
	if (head->first->prev != NULL) {
	    fprintf(stdout,
	      "test_push_back_multi: failed. head->first->prev != NULL.\n");
	    errc++;
	}
	if (head->last->next != NULL){
	    fprintf(stdout,
	      "test_push_back_multi: failed. head->first->prev != NULL.\n");
	    errc++;
	}
    }

    llist_destroy(head);
    return errc;
}

/******************* pop front tests **********************/

/* call pop_front once */
int test_pop_front_1(llist_head_t*(*test_ctor)(), void*(*get_elem)())
{
    llist_head_t* head = test_ctor();
    int errc = 0;

    void* elem = get_elem();
    llist_push_front(head, elem);

    /* we assume push_front works, so we don't test anything after
     * calling push_front
     */
    
    /* pop the element */
    void* popped = llist_pop_front(head);

    if (head->first != NULL) {
	fprintf(stdout,
		"test_pop_front_1: failed. head->first != NULL.\n");
	errc++;
    }
    if (head->last != NULL) {
	fprintf(stdout,
		"test_pop_front_1: failed. head->last != NULL.\n");
	errc++;
    }
    if (head->length != 0) {
	fprintf(stdout, "test_pop_front_1: failed. head->length != 0.\n");
	errc++;
    }
    if (popped != elem) {
	fprintf(stdout, "test_pop_front_1: failed. popped != elem.\n");
	errc++;
    }

    /* free the test element */
    if (head->free_data) {
	head->free_data(elem);
    }
    
    llist_destroy(head);
    return errc;
}

/* call pop_front MULTI times */
int test_pop_front_multi(llist_head_t*(*test_ctor)(), void*(*get_elem)())
{
    llist_head_t* head = test_ctor();
    int errc = 0;

    void* control[MULTI];
    
    /* construct a list */
    for (size_t i = 0; i < MULTI; i++) {
	void* elem = get_elem();
	llist_push_back(head, elem);
	control[i] = elem;
    }

    /* pop every element */
    for (size_t i = 0; i < MULTI; i++) {
	if (head->length != MULTI - i) {
	    fprintf(stdout,
	      "test_pop_front_multi: failed. bad length.\n");
	    errc++;
	}
	void* popped = llist_pop_front(head);
	if (popped != control[i]) {
	    fprintf(stdout,
	      "test_pop_front_multi: failed. popped != control[i].\n");
	    errc++;
	}
	if (head->first && head->first->prev != NULL) {
	    fprintf(stdout,
	      "test_pop_front_multi: failed. bad head->first.\n");
	    errc++;
	}
	if (head->last && head->last->next != NULL) {
	    fprintf(stdout,
	      "test_pop_front_multi: failed. bad head->last.\n");
	    errc++;
	}
    }

    /* free the test data */
    if (head->free_data) {
	for (size_t i = 0; i < MULTI; i++) {
	    head->free_data(control[i]);
	}
    }
    
    llist_destroy(head);
    return errc;
}

/******************** pop back tests **********************/

/* call pop_back once */
int test_pop_back_1(llist_head_t*(*test_ctor)(), void*(*get_elem)())
{
    llist_head_t* head = test_ctor();
    int errc = 0;

    void* elem = get_elem();
    llist_push_back(head, elem);

    /* we assume push_front works, so we don't test anything after
     * calling push_front
     */
    
    /* pop the element */
    void* popped = llist_pop_back(head);

    if (head->first != NULL) {
	fprintf(stdout,
		"test_pop_back_1: failed. head->first != NULL.\n");
	errc++;
    }
    if (head->last != NULL) {
	fprintf(stdout,
		"test_pop_back_1: failed. head->last != NULL.\n");
	errc++;
    }
    if (head->length != 0) {
	fprintf(stdout, "test_pop_back_1: failed. head->length != 0.\n");
	errc++;
    }
    if (popped != elem) {
	fprintf(stdout, "test_push_front_1: failed. popped != elem.\n");
	errc++;
    }

    /* free the test element */
    if (head->free_data) {
	head->free_data(elem);
    }
    
    llist_destroy(head);
    return errc;
}

/* call pop_back MULTI times */
int test_pop_back_multi(llist_head_t*(*test_ctor)(), void*(*get_elem)())
{
    llist_head_t* head = test_ctor();
    int errc = 0;

    void* control[MULTI];
    
    /* construct a list */
    for (size_t i = 0; i < MULTI; i++) {
	void* elem = get_elem();
	llist_push_back(head, elem);
	control[i] = elem;
    }

    /* pop every element */
    for (size_t i = MULTI; i != 0; i--) {
	if (head->length != i) {
	    fprintf(stdout,
	      "test_pop_back_multi: failed. bad length.\n");
	    errc++;
	}
	void* popped = llist_pop_back(head);
	if (popped != control[i-1]) {
	    fprintf(stdout, "test_pop_back_multi: failed."
		     " popped != control[i-1].\n");
	    errc++;
	}
	if (head->first && head->first->prev != NULL) {
	    fprintf(stdout,
	      "test_pop_back_multi: failed. bad head->first.\n");
	    errc++;
	}
	if (head->last && head->last->next != NULL){
	    fprintf(stdout,
	      "test_pop_back_multi: failed. bad head->last.\n");
	    errc++;
	}
    }

    /* free the test data */
    if (head->free_data) {
	for (size_t i = 0; i < MULTI; i++) {
	    head->free_data(control[i]);
	}
    }
    
    llist_destroy(head);
    return errc;
}

int run_tests(llist_head_t*(*test_ctor)(),
	      void*(*get_elem)())
{
    int errc = 0;
    
    errc += test_ctor_dtor_empty(test_ctor);
    errc += test_push_front_1(test_ctor, get_elem);
    errc += test_push_front_multi(test_ctor, get_elem);
    errc += test_push_back_1(test_ctor, get_elem);
    errc += test_push_back_multi(test_ctor, get_elem);
    errc += test_pop_front_1(test_ctor, get_elem);
    errc += test_pop_front_multi(test_ctor, get_elem);
    errc += test_pop_back_1(test_ctor, get_elem);
    errc += test_pop_back_multi(test_ctor, get_elem);

    return errc;
}

int main(int argc, char** argv)
{
    /* seed the prng */
    srand(time(NULL));

    int errc = 0;

    errc += run_tests(&make_char_list, &get_char);
    errc += run_tests(&make_point_list, &get_point);

    if (errc == 0) {
	fprintf(stdout, "llist-test: all tests passed!\n");
    } else {
	fprintf(stdout, "llist-test: failed %d tests.\n", errc);
    }
}
