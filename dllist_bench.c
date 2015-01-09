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
#define MULTI 1000000UL

long get_time_ns()
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_nsec;
}

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

void bench_dtor(llist_head_t*(*test_ctor)(), void*(*get_elem)())
{
    llist_head_t* head = test_ctor();
    
    for (size_t i = 0; i < MULTI; i++) {
	void* elem = get_elem();
	llist_push_front(head, elem);
    }
    
    long time = get_time_ns();
    llist_destroy(head);
    time = get_time_ns() - time;
    fprintf(stdout, "bench_dtor: average %ld ns per element.\n",
	    time/MULTI);
}

void bench_push_front(llist_head_t*(*test_ctor)(), void*(*get_elem)())
{
    llist_head_t* head = test_ctor();

    long time = get_time_ns();
    for (size_t i = 0; i < MULTI; i++) {
	llist_push_front(head, get_elem());
    }
    time = get_time_ns() - time;
    fprintf(stdout, "bench_push_front: average %ld ns per element.\n",
	    time/MULTI);
    llist_destroy(head);
}

void bench_push_back(llist_head_t*(*test_ctor)(), void*(*get_elem)())
{
    llist_head_t* head = test_ctor();

    long time = get_time_ns();
    for (size_t i = 0; i < MULTI; i++) {
	llist_push_back(head, get_elem());
    }
    time = get_time_ns() - time;
    fprintf(stdout, "bench_push_back: average %ld ns per element.\n",
	    time/MULTI);
    llist_destroy(head);
}

int main (int argc, char** argv)
{
    fprintf(stdout, "llist-bench: running benchmark with characters."
	" (no mallocs involved for data).\n");
    bench_dtor(&make_char_list, &get_char);
    bench_push_front(&make_char_list, &get_char);
    bench_push_back(&make_char_list, &get_char);

    fprintf(stdout, "llist-bench: running benchmark with points."
	    " (points use dynamicly allocated memory).\n");
    bench_dtor(&make_point_list, &get_point);
    bench_push_front(&make_point_list, &get_point);
    bench_push_back(&make_point_list, &get_point);
}

