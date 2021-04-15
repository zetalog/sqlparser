/*
 * ZETALOG's Personal COPYRIGHT
 *
 * Copyright (c) 2003
 *    ZETALOG - "Lv ZHENG".  All rights reserved.
 *    Author: Lv "Zetalog" Zheng
 *    Internet: zetalog@hzcnc.com
 *
 * This COPYRIGHT used to protect Personal Intelligence Rights.
 * Redistribution and use in source and binary forms with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by the Lv "Zetalog" ZHENG.
 * 3. Neither the name of this software nor the names of its developers may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 4. Permission of redistribution and/or reuse of souce code partially only
 *    granted to the developer(s) in the companies ZETALOG worked.
 * 5. Any modification of this software should be published to ZETALOG unless
 *    the above copyright notice is no longer declaimed.
 *
 * THIS SOFTWARE IS PROVIDED BY THE ZETALOG AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE ZETALOG OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @(#)list.h: simple doubly linked list implementation
 * $Id: list.h,v 1.2 2007/03/10 06:55:55 cvsroot Exp $
 */

#ifndef __LIST_H_INCLUDE__
#define __LIST_H_INCLUDE__

/*
 * These are non-NULL pointers that will result in page faults
 * under normal circumstances, used to verify that nobody uses
 * non-initialized list entries.
 */
#define LIST_POISON1  ((void *)0x00100100)
#define LIST_POISON2  ((void *)0x00200200)

/*
 * Copied from LINUX source codes.
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole lists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */

typedef struct _list_t {
	struct _list_t *next, *prev;
} list_t;

#define list_head_init(name) { &(name), &(name) }

#define list_head(name)	\
	struct _list_t name = list_head_init(name)

#define list_init(node)						\
	do {							\
		(node)->next = node; (node)->prev = node;	\
	} while (0)

/**
 * list_insert_head - add a new entry
 * @entry: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
#define list_insert_head(entry, head)				\
	do {							\
		(head)->next->prev = entry;			\
		(entry)->next = (head)->next;			\
		(entry)->prev = head; (head)->next = entry;	\
	} while (0)

/**
 * list_insert_tail - add a new entry
 * @entry: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
#define list_insert_tail(entry, head)				\
	do {							\
		(head)->prev->next = entry;			\
		(entry)->prev = (head)->prev;			\
		(head)->prev = entry; (entry)->next = head;	\
	} while (0)

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
#define __list_delete(node, entry)				\
	do {							\
		(entry)->prev = node; (node)->next = entry;	\
	} while (0)

/**
 * list_delete - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: list_empty on entry does not return true after this,
 * the entry is in an undefined state.
 */
#define list_delete(entry)					\
	do {							\
		(entry)->next->prev = (entry)->prev;		\
		(entry)->prev->next = (entry)->next;		\
		(entry)->next = LIST_POISON1;			\
		(entry)->prev = LIST_POISON2;			\
	} while (0)

/**
 * list_delete_init - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 */
#define list_delete_init(entry)					\
	do {							\
		list_t * tmp = entry;				\
		(entry)->next->prev = (entry)->prev;		\
		(entry)->prev->next = (entry)->next;		\
		list_init(tmp);					\
	} while (0)

/**
 * list_move - delete from one list and add as another's head
 * @list: the entry to move
 * @head: the head that will precede our entry
 */
#define list_move_head(list, head)				\
	do {							\
		list_t * tmp = list;				\
		__list_delete(list->prev, list->next);		\
		list_insert_head(tmp, head);			\
	} while (0)

/**
 * list_move_tail - delete from one list and add as another's tail
 * @list: the entry to move
 * @head: the head that will follow our entry
 */
#define list_move_tail(list, head)				\
	do {							\
		list_t * tmp = list;				\
		__list_delete(list->prev, list->next);		\
		list_insert_tail(tmp, head);			\
	} while (0)

/**
 * list_empty - tests whether a list is empty
 * @head: the list to test.
 */
#define list_empty(head)					\
	((head)->next == (head))

#define __list_splice(list, head)				\
	do {							\
		list_t *first = (list)->next;			\
		list_t *last = (list)->prev;			\
		list_t *at = (head)->next;			\
		first->prev = (head); (head)->next = first;	\
		last->next = at; at->prev = last;		\
	} while (0)

/**
 * list_splice - join two lists
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
#define list_splice(list, head)					\
	do {							\
		if (!list_empty(list)) {			\
			list_t *first = (list)->next;		\
			list_t *last = (list)->prev;		\
			list_t *at = (head)->next;		\
			first->prev = head;			\
			(head)->next = first;			\
			last->next = at; at->prev = last;	\
		}						\
	} while (0)

/**
 * list_splice_init - join two lists and reinitialise the emptied list.
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * The list at @list is reinitialised
 */
#define list_splice_init(list, head)				\
	do {							\
		if (!list_empty(list)) {			\
			list_t *first = (list)->next;		\
			list_t *last = (list)->prev;		\
			list_t *at = (head)->next;		\
			first->prev = head;			\
			(head)->next = first;			\
			last->next = at; at->prev = last;	\
			list_init(list);			\
		}						\
	} while (0)

/**
 * list_entry - get the struct for this entry
 * @node: the &struct list_head pointer.
 * @type: the type of the struct this is embedded in.
 * @member: the name of the list_struct within the struct.
 */
#define list_entry(node, type, member)				\
	((type *)((char *)(node) - (int)&((type *)0)->member))

/**
 * list_iterate_forward	- iterate over a list safe against removal of list entry
 * @pos: the &struct list_head to use as a loop counter.
 * @n: another &struct list_head to use as temporary storage
 * @head: the head for your list.
 */
#define list_iterate_forward(pos, n, head)			\
	for (pos = (head)->next, n = pos->next;			\
		pos != (head); pos = n, n = pos->next)

/**
 * list_iterate_backward - iterate over a list safe against removal of list entry
 * @pos: the &struct list_head to use as a loop counter.
 * @n: another &struct list_head to use as temporary storage
 * @head: the head for your list.
 */
#define list_iterate_backward(pos, n, head)			\
	for (pos = (head)->prev, n = pos->prev;			\
		pos != (head); pos = n, n = pos->prev)

#endif /* __LIST_H_INCLUDE__ */
