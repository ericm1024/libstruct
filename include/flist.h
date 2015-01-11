/* Eric Mueller
 * flist.h 
 * December 2014
 *
 * Header file for a forward list (singly linked).
 */

#ifndef STRUCT_FLIST_H
#define STRUCT_FLIST_H 1

#include "types.h"

/**
 * Declares a new flist head.
 *
 * @name  The name of the new list.
 */
#define FLIST_HEAD(name) \
	struct flist_head name = {NULL, 0};

/**
 * This macro is taken out of the Linux kernel and is not my own.
 * (This comment however, is)
 *
 * @ptr     Pointer to a member of a struct.
 * @type    The type of the enclosing struct.
 * @member  The name of the member in the struct declaration.
 */
#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

/**
 * Insert a list element after another element.
 *
 * @hd        Pointer to the head of the list.
 * @after     Pointer to the list node to insert after. If null, insertee is
 *            inserted at the front of the list.
 * @insertee  Pointer the the list node to insert.
 */
extern void flist_insert_after(struct flist_head *hd, struct flist *after,
			       struct flist *insertee);

/**
 * Insert an element at the front of a list.
 *
 * @hd        Pointer to the head of the list.
 * @insertee  Pointer to the list node to insert.
 */
extern void flist_push_front(struct flist_head *hd, struct flist *insertee);

/**
 * Remove the first element of the list.
 *
 * @hd  Pointer to the head of the list. 
 */
extern struct flist *flist_pop_front(struct flist_head *hd);

/**
 * Insert an entire list into the list hd after the element after. The head
 * of the inserted list is invalidated, i.e. its first pointer is set to null
 * and length is set to zero.
 *
 * @hd       Pointer to the list to insert into. 
 * @after    Splicee is inserted after this element
 * @splicee  Pointer to the list to insert. Invalidated by calling this 
 *           function.
 */
extern void flist_splice(struct flist_head *hd, struct flist *after,
			 struct flist_head *splicee);

/**
 * Execute a function on each element in the list. The function is applied to
 * the container, not the list node itself.
 *
 * @hd      Pointer to the head of the list.
 * @f       Pointer to the function to apply to each element.
 * @offset  The offset of the list member in the enclosing data structure.
 */
extern void flist_for_each(struct flist_head *hd, void (*f)(void *data),
			   ptrdiff_t offset);

/**
 * Execute a function on each element in the list in the range [first, last).
 * The function is applied to the container, not the list node itself.
 *
 * @hd      Pointer to the head of the list.
 * @f       Pointer to the functiuon to apply to each element.
 * @offset  The offset of the list member in the enclosing data structure.
 * @first   Pointer to the first element to apply the function to.
 * @last    Pointer to the element after the last element on which the
 *          function will be called.
 */
extern void flist_for_each_range(struct flist_head *hd, void (*f)(void *data),
				 ptrdiff_t offset, struct flist *first,
				 struct flist *last);

#endif /* STRUCT_FLIST_H */
