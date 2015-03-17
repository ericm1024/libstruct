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

/*! opaque iterator */
typedef struct cs_cursor* cs_cursor_t;

/*! chunky_str */
struct chunky_str {
	/*! doubly linked list containing chunks */
	struct list_head str;
	/*! number of characters in the string */
	unsigned long nchars;
	/*! list of outstanding cursors */
	struct list_head cursors;
};

/**
 * \brief Declare a chunky string.
 * \param name   (token) name of the string to declare.
 */
#define CHUNKY_STR(name)			\
	struct chunky_str name = {		\
		.str = {			\
			.first = NULL,		\
			.last = NULL,		\
			.length = 0,		\
			.offset = 0},		\
		.nchars = 0,			\
		.list_head = {			\
			.first = NULL,		\
			.last = NULL,		\
			.length = 0,		\
			.offset = 0}}



/**********************************************************
 *                       cursor ops                       *
 **********************************************************/

/**
 * \brief Allocate and return a cursor to the beginning of the string.
 * \param cs  The chunky_str to get a cursor to.
 * \return A cursor referencing the beginning of the list. NULL if allocation
 * failed.
 */
extern cs_cursor_t cs_get_cursor(struct chunky_str *cs);

/**
 * \brief Clone a cursor.
 * \param Jango   The template for cloning
 * \return Boba (an unaltered clone for himself)
 * \detail This function allocates memory. Both cursors need to be
 * freed with calls to cs_cursor_destroy.
 */
extern cs_cursor_t cs_clone_cursor(cs_cursor_t jango);

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
 * \return true if the cursor refers to a valid character (i.e. if the string
 * is not empty)
 */
extern bool cs_cursor_begin(cs_cursor_t cursor);

/**
 * \brief Move a cursor to the end of its string.
 * \param cursor   The cursor to move.
 * \return true if the cursor refers to a valid character (i.e. if the string
 * is not empty)
 */
extern bool cs_cursor_end(cs_cursor_t cursor, int *status);

/**
 * \brief Determine if a cursor is within the range of the string.
 * \param cursor  The cursor
 * \return true if the cursor is valid and refers to a character within the string
 */
extern bool cs_cursor_in_range(cs_cursor_t cursor);

/**
 * \brief Destroy (deallocate) a cursor. The cursor is (hopefully obviously)
 * no longer valid after this operation, i.e. if you try to use it, you'll
 * probably get segfaults.
 * \param cursor   The cursor to destroy.
 */
extern void cs_cursor_destroy(cs_cursor_t cursor);

/**
 * \brief Move a cursor to the next character in the string.
 * \param cursor   The cursor to move.
 * \return The next character.
 * \detail Calling this on an invalid cursor will produce undefined behavior.
 */
extern char cs_cursor_next(cs_cursor_t cursor, int *status);

/**
 * \brief Move a cursor to the previous character in the string.
 * \param cursor   The cursor to move.
 * \return The previous character.
 * \detail Calling this on an invalid cursor will produce undefined behavior.
 */
extern char cs_cursor_prev(cs_cursor_t cursor, int *status);

/**
 * \brief Get the character at the cursor's current location.
 * \param cursor   The cursor.
 * \return The character at the cursor's current location.
 * \detail Calling this on an invalid cursor will produce undefined behavior.
 */
extern char cs_cursor_get(cs_cursor_t cursor);

/**
 * \brief Insert a character before a cursor.
 * \param cursor   The cursor
 * \param c        The character to insert
 * \return true if the character was inserted, false if it could not be
 * inserted (for example, if memory needed to be allocated and the
 * allocation failed, or if the cursor was invalid)
 * \note This will invalidate all other cursors.
 */
extern bool cs_insert_before(cs_cursor_t cursor, char c);

/**
 * \brief Insert a character after a cursor.
 * \param cursor   The cursor
 * \param c        The charater to insert
 * \return true if the character was inserted, false if it could not be
 * inserted (for example, if memory needed to be allocated and the
 * allocation failed, or if the cursor was invalid)
 * \note This will invalidate all other cursors.
 */
extern bool cs_insert_after(cs_cursor_t cursor, char c);

/**
 * \brief Clobber the character at the current cursor with a new one.
 * \param cursor  The cursor
 * \param c       The new character.
 * \return true if the insertion was sucessful, false otherwise. Since this
 * function will not allocate memory, the only way for it to fail is if
 * the cursor is not valid.
 */
extern bool cs_insert_clobber(cs_cursor_t cursor, char c);

/**
 * \brief Erase the charater at the cursor's location.
 * \param cursor   The cursor.
 * \param c        Somewhere to put the character that was removed. NULL is ok.
 * \return True if the erase was valid, false otherwise.
 */
extern bool cs_erase(cs_cursor_t cursor, char *c);

/**
 * \brief Determine if a cursor is valid.
 * \param cursor   The cursor
 * \return true if the cursor is valid, false if not. A cursor is invalidated
 * any time cs_insert_before, cs_insert_after, or cs_erase is called on another
 * cursor to the same string.
 */
extern bool cs_cursor_is_valid(cs_cursor_t cursor);

/**
 * \brief Invalidate a cursor. This is a quick way to mark a cursor as unused
 * without actually freeing it.
 * \param cursor   The cursor to invalidate.
 * \detail There is no going back on this function call. Once a cursor is
 * invalidated it is no longer usable in any way.
 */
extern void cs_invalidate_cursor(cs_cursor_t cursor);



/**********************************************************
 *                    chunky string ops                   *
 **********************************************************/

/**
 * \brief Garbage collect cursors, i.e. free all invalid cursors.
 * \param cs   The chunky string to garbage collect from.
 */
extern void cs_do_cursor_gc(struct chunky_str *cs);

/**
 * \brief Destroy a chunky string and free all memory associated with it
 * (this includes the string itself, as well as any and all outstanding cursors
 * to the string.
 * \param cs   The string to destroy.
 */
extern void cs_destroy(struct chunky_str *cs);

/**
 * \brief Create a copy of a string (does not copy cursors).
 * \param cs   The string to copy.
 * \return A deep copy of @cs.
 * \detail Memory is allocated, so the new string will need to be freed
 * with a call to cs_destroy.
 */
extern struct chunky_str *cs_clone(struct chunky_str *cs);

/**
 * \brief Create a c string representation of @cs.
 * \param cs       The struct chunky_str to turn into a c string.
 * \param length   Pointer to long where the length of the string is written
 * to.
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
 * \detail This function will copy all characters in @cs, not just up to the
 * first null byte.
 */
extern unsigned long cs_print(struct chunky_str *cs, char *buf,
			      unsigned long size);

#endif /* STRUCT_CHUNKY_STRING_H */
