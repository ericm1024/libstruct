/**
 * \file bitops.h
 *
 * \author Eric Mueller
 *
 * \brief A collection of bit-level operations 
 */

#ifndef INCLUDE_BITOPS_H
#define INCLUDE_BITOPS_H 1

#include <limits.h> /* CHAR_BIT */

#define SIGN_BIT(x) ((x) >> (sizeof(typeof(x)) * CHAR_BIT - 1) & 1)

#endif /* INCLUDE_BITOPS_H */
