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
 * \file chunky_str.h
 *
 * \author Eric Mueller
 *
 * \note I must credit to the Harvey Mudd College Computer Science Department
 * as they came up with this data structure and its lovely name (though they
 * do it in C++).
 *
 * \brief Header file for a string implementation.
 *
 * \detail A versitile string implementation designed to handle fast insetion,
 * deletion, and traversal with minimal space usage. The string can store any
 * byte, including multiple null bytes.
 *
 * Access to the string is performed via the opaque cs_cursor_t type opject.
 * any number of cursors can exists concurrently, but once one cursor writes,
 * all others are invalidated.
 *
 * TODO WRITE MORE
 */

#ifndef STRUCT_CHUNKY_STRING_H
#define STRUCT_CHUNKY_STRING_H 1

#include "list.h"
#include <stddef.h>
#include <stdbool.h>

/*
 * TODO: make this a truly opaque type,
 * i.e. typedef struct {struct cs_cursor*} cs_cursor_t;
 */

/*! opaque iterator */
typedef struct cs_cursor* cs_cursor_t;

/*! chunky string structure */
struct chunky_str {
	/*! doubly linked list of chunks */
	struct list_head str;
	/*! number of characters in the string */
	unsigned long nchars;
};


/**
 * \brief Definition of a default chunky string.
 */ 
#define CHUNKY_STRING_DEFAULT				\
	(struct chunky_str){.str = {			\
			        .first = NULL,		\
				.last = NULL,		\
				.length = 0,		\
				.offset = 0},		\
			.nchars = 0}

/**
 * \brief Declare and define a chunky string.
 * \param name   (token) name of the string to declare.
 */
#define CHUNKY_STRING(name)			\
	struct chunky_str name = CHUNKY_STRING_DEFAULT



/**********************************************************
 *                       cursor ops                       *
 **********************************************************/

/**
 * \brief Allocate and return a cursor to the beginning of the string.
 * \param cs  The chunky_str to get a cursor to.
 * \return A cursor referencing the beginning of the list. NULL if allocation
 * failed.
 */
extern cs_cursor_t cs_cursor_get(struct chunky_str *cs);

/**
 * \brief Destroy (deallocate) a cursor. The cursor is (hopefully obviously)
 * no longer valid after this operation, i.e. if you try to use it, you'll
 * probably get segfaults.
 * \param cursor   The cursor to destroy.
 */
extern void cs_cursor_destroy(cs_cursor_t cursor);

/**
 * \brief Clone a cursor.
 * \param Jango   The template for cloning
 * \return Boba (an unaltered clone for himself)
 * \detail This function allocates memory. Both cursors need to be
 * freed with calls to cs_cursor_destroy.
 */
extern cs_cursor_t cs_cursor_clone(cs_cursor_t jango);

/**
 * \brief Determine if two cursors refer to exactly the same location
 * in the same string.
 * \param lhs   The first cursor
 * \param rhs   The second cursor
 * \return true if the cursors are both valid and refer to the same
 * location.
 */
extern bool cs_cursor_equal(cs_cursor_t lhs, cs_cursor_t rhs);

/**
 * \brief Move a cursor to the beginning of its string.
 * \param cursor   The cursor to move.
 */
extern void cs_cursor_begin(cs_cursor_t cursor);

/**
 * \brief Get a cursor to one-past the end of the string.
 * \param cursor   The cursor to move.
 */
extern void cs_cursor_end(cs_cursor_t cursor);

/**
 * \brief Determine if a cursor is within the range of the string.
 * \param cursor  The cursor
 * \return true if the cursor is valid and refers to a character within the string
 */
extern bool cs_cursor_in_range(cs_cursor_t cursor);

/**
 * \brief Move a cursor to the next character in the string.
 * \param cursor   The cursor to move.
 * \return The next character.
 */
extern char cs_cursor_next(cs_cursor_t cursor);

/**
 * \brief Move a cursor to the previous character in the string.
 * \param cursor   The cursor to move.
 * \return The previous character.
 */
extern char cs_cursor_prev(cs_cursor_t cursor);

/**
 * \brief Get the character at the cursor's current location.
 * \param cursor   The cursor.
 * \return The character at the cursor's current location.
 */
extern char cs_cursor_getchar(cs_cursor_t cursor);

/**
 * \brief Insert a character before a cursor.
 * \param cursor   The cursor. Points to the same character after this
 *                 function is called.
 * \param c        The character to insert. Inserted before the character the
 *                 cursor currently refers to.
 * \return true if the character was inserted, false if it could not be
 * inserted (for example, if memory needed to be allocated and the
 * allocation failed, or if the cursor was invalid)
 * \note This operation renders all other cursors invalid.
 */
extern bool cs_cursor_insert(cs_cursor_t cursor, char c);

/**
 * \brief Clobber the character at the current cursor with a new one.
 * \param cursor  The cursor. Does not move.
 * \param c       The new character.
 * \return true
 */
extern bool cs_cursor_insert_clobber(cs_cursor_t cursor, char c);

/**
 * \brief Erase the charater at the cursor's location.
 * \param cursor   The cursor. Gets moved to the character after the erased
 *                 character, possibly moving it off the end.
 */
extern void cs_cursor_erase(cs_cursor_t cursor);

/**
 * \brief Erase the charater at the cursor's location and return it.
 * \param cursor   The cursor.
 * \return The character that was erased.
 */
extern char cs_cursor_erase_get(cs_cursor_t cursor);



/**********************************************************
 *                    chunky string ops                   *
 **********************************************************/

/**
 * \brief Append a character to a string.
 * \param cs  The chunky string to append to.
 * \param c   The character to append.
 * \return true if the operation suceeded, false if memory allocation failed.
 */
extern bool cs_push_back(struct chunky_str *cs, char c);

/**
 * \brief Prepend a character to a string.
 * \param cs  The chunky string to prepend to.
 * \param c   The character to prepend.
 * \return true if the operation suceeded, false if memory allocation failed.
 */
extern bool cs_push_front(struct chunky_str *cs, char c);

/**
 * \brief Destroy a chunky string and free all memory associated with it
 * (this includes the string itself, as well as any and all outstanding cursors
 * to the string.
 * \param cs   The string to destroy.
 */
extern void cs_destroy(struct chunky_str *cs);

/**
 * \brief Create a copy of a string (does not copy cursors).
 * \param cs     The string to copy.
 * \param clone  Where to put the new string.
 * \return A deep copy of @cs.
 * \detail Memory is allocated, so the new string will need to be freed
 * with a call to cs_destroy.
 */
extern bool cs_clone(struct chunky_str *cs, struct chunky_str *clone);

/**
 * \brief Create a c string representation of @cs.
 * \param cs       The struct chunky_str to turn into a c string.
 * \param length   Pointer to long where the length of the string is written
 * to. Length includes the terminating null.
 * \return Pointer to a heap allocated string. Must be freed by the caller.
 * \detail **NOTE** Because of how c strings are expected to behave, this
 * function will only copy the elements in @cs up to the first null byte.
 * If you want to create a c string from a chunky string with multiple null
 * bytes in it, be my guest, but I refuse to enable such shenanigans.
 */
extern char *cs_to_cstring(struct chunky_str *cs, unsigned long *length);

/**
 * \brief Write the contents of a string to a buffer.
 * \param cs    The chunky string to write out.
 * \param buf   The buffer to write to.
 * \param size  The size of buf in bytes.
 * \return The number of bytes written to buf.
 * \detail This function will copy all characters in @cs, not just up to the
 * first null byte.
 */
extern unsigned long cs_write(struct chunky_str *cs, char *buf,
			      unsigned long size);

/**
 * \brief Iterate over every character in a chunky string.
 * \param char_name   The name of the iterating char variable to declare.
 * \param cursor      A cursor to iterate with.
 * \detail Within the loop one should only access the cursor with the
 * cs_cursor_getchar function.
 */ 
#define cs_for_each(cursor)						\
	for (cs_cursor_begin(cursor);					\
	     cs_cursor_in_range(cursor);				\
	     cs_cursor_next(cursor))

#endif /* STRUCT_CHUNKY_STRING_H */
