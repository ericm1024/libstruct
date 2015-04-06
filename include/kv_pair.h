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
 * \file kv_pair.h
 *
 * \author Eric Mueller
 *
 * \brief Simple key-value pair structure. 
 */

#ifndef STRUCT_KV_PAIR_H
#define STRUCT_KV_PAIR_H 1

struct kv_pair {
	unsigned long key;
	const void *value;
};

#define KV_PAIR(k, v) struct kv_pair {.key = k; .value = v; };

#endif /* STRUCT_KV_PAIR_H */
