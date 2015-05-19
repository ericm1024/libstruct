/*
 * Copyright 2014 Eric Mueller
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
 * \file bitops.h
 *
 * \author Eric Mueller
 *
 * \brief A collection of bit-level operations 
 */

#ifndef INCLUDE_BITOPS_H
#define INCLUDE_BITOPS_H 1

#include <limits.h>

#define SIGN_BIT(x) ((x) >> (sizeof(x) * CHAR_BIT - 1) & 1)

static inline unsigned long ullog2(unsigned long x)
{
	unsigned long shift = 0;
	while (x > (1UL << shift++))
		;
	return shift - 1;
}

static inline bool uladd_ok(unsigned long a, unsigned long b)
{
	return ULONG_MAX - a >= b;
}

static inline bool ulsub_ok(unsigned long a, unsigned long b)
{
	return a >= b;
}

#endif /* INCLUDE_BITOPS_H */
