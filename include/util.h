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

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "pcg_variants.h"

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
