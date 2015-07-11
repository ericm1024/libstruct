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
 * \file list_test.c
 *
 * \author Eric Mueller
 *
 * \brief Tests for list.
 */

#include <stdint.h>

#include "list.h"
#include "test.h"
#include "util.h"

/* size of test lists */
#define data_length 10000

struct foo {
        uint64_t val;
	struct list link;
};

enum {
        INSERT_FRONT,
        INSERT_BACK,
        INSERT_MIDDLE,
};

void init_test_list(struct list *head, struct foo ***values, size_t length,
                    int how)
{
        unsigned long i, j;
        struct foo *foo;
        
        *values = malloc(length * sizeof **values);
        ASSERT_TRUE(*values, "malloc barfed in init_test_list\n");

        for (i = 0; i < length; i++) {
                foo = malloc(sizeof *foo);
                ASSERT_TRUE(foo, "malloc barfed in init_test_list\n");
                values[i] = 
                values[i]->val = pcg64_random();
        
        if (how == INSERT_FRONT)
                for (i = 0; i < length; i++)
                        list_insert_before(head, &values[i]->link);
        else if (how == INSERT_BACK)
                for (i = length; i-- > 0;)
                        list_insert_after(head, &values[i]->link);
        else {
                struct list *middle = head;
                for (i = 0, j = length - 1; i < j; middle = middle->next) {
                        list_insert_before(middle, &values[i]->link);
                        i++;
                        
                        middle = middle->prev;
                        
                        if (!(i < j))
                                break;
                        
                        list_insert_after(middle, &values[j]->link);
                        j--;
                }
        }
}

void verify_test_list(struct list *head, struct foo ***values, size_t length)
{
        struct foo *elem;
        unsigned long i = 0;
        
        list_for_each_entry(head, elem, struct foo, link) {
                ASSERT_TRUE(elem == values[i], "list was out of order\n");
                i++;
        }

        ASSERT_TRUE(i == length, "list length was wrong\n");
}

void destroy_list(struct list *head, struct foo ***values, size_t length)
{
        struct foo *elem;
        
        list_for_each_entry_safe(head, elem, struct foo, link)
}

void test_insert()
{
        struct list test = LIST_INIT(test);
        struct foo *values;

        init_test_list(&test, &values, data_length, INSERT_FRONT);
}


/* main */
int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	return run_all_tests();
}
