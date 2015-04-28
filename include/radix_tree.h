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
 * \file radix_tree.h
 *
 * \author Eric Mueller
 * 
 * \brief Header file for an a radix_tree.
 *
 * \detail **TODO**
 */

#ifndef STRUCT_RADIX_TREE_H
#define STRUCT_RADIX_TREE_H 1

struct radix_tree_head {
	/* root of the tree */
	struct radix_node *root;
	
	/* number of nodes in the tree*/
	unsigned long nnodes;

	/* number of entries in the tree */
	unsigned long nentries;
};

/* destroy */

/* insert */

/* delete */

/* lookup */

/* */
	

#endif /* STRUCT_RADIX_TREE_H */
