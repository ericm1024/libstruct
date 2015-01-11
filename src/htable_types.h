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
 * December 2014
 * htable_types.h
 *
 * Hash table structures.
 */

#ifndef STRUCT_HTABLE_TYPES_H
#define STRUCT_HTABLE_TYPES_H 1

#include <stddef.h>

struct htable {
	size_t entries;
	void *buckets;
	void *meta;
};

#endif /* STRUCT_HTABLE_TYPES_H */