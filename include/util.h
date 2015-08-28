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
 * \file util.h
 *
 * \author Eric Mueller
 * 
 * \brief Small set of generic utilities (random numbers, etc)
 */

#ifndef STRUCT_UTIL_H
#define STRUCT_UTIL_H

#include "pcg_variants.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#define __RANDOM_SRC "/dev/urandom"

/* never look at this function its terrible */
static inline bool fallback_seed_rng()
{
	pcg128_t seed = (pcg128_t)(uintptr_t)&seed; /* ASLR. not my idea */
	pcg64_srandom(seed, seed); /* ugh */
	return true;
}

static inline bool seed_rng()
{
	int fd = open(__RANDOM_SRC, O_RDONLY);
	pcg128_t seeds[2];

	if (fd < 0) {
		fprintf(stderr, "coult not open file %s, error: %s\n",
			__RANDOM_SRC, strerror(errno));
		return fallback_seed_rng();
	}

	if (read(fd, &seeds, sizeof(seeds)) < (int)sizeof(seeds))
		return fallback_seed_rng();

	pcg64_srandom(seeds[0], seeds[1]);
	pcg32_srandom(pcg64_random(), pcg64_random());
	
	return true;
}

static inline unsigned long div_round_up_ul(unsigned long x, unsigned long d)
{
        if (x > ULONG_MAX - d)
                return ULONG_MAX/d;
        else
                return (x + d - 1)/d;
}

#define container_of(__ptr, __type, __member)   \
        ((__type *)((char *)(__ptr) - offsetof(__type, __member)))

#define swap_t(__type, __a, __b)                        \
        do {                                            \
                __type __swap_tmp = (__a);              \
                (__a) = (__b);                          \
                (__b) = __swap_tmp;                     \
        } while (0);

#endif /* STRUCT_UTIL_H */
