/*
 * ZETALOG's Personal COPYRIGHT
 *
 * Copyright (c) 2007
 *    ZETALOG - "Lv ZHENG".  All rights reserved.
 *    Author: Lv "Zetalog" Zheng
 *    Internet: zetalog@gmail.com
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
 * @(#)sql_node.h: SQL parser storage node interface
 * $Id: sql_node.h,v 1.2 2007/04/13 13:41:42 zhenglv Exp $
 */

#ifndef __SQL_NODE_H_INCLUDE__
#define __SQL_NODE_H_INCLUDE__

#include <list.h>
#include <vector.h>

#define SQL_COLUMN_NAME		1
#define SQL_COLUMN_TYPE		2
#define SQL_COLUMN_OPTS		3

typedef enum _sql_tag_t {
	T_table,
	T_column,
	T_type,
	T_symbol,
	T_expr,
	T_atom,
	T_option,
	T_order,
	T_assign,
	T_refer,
	T_func,
	T_constr,
	T_normal_stmt,
	T_insert_stmt,
	T_delete_stmt,
	T_update_stmt,
	T_select_stmt,
	T_create_stmt,
	T_drop_stmt
} sql_tag_t;

typedef struct _sql_node_t {
	sql_tag_t type;
	list_t list;
} sql_node_t;

extern sql_node_t *sql_new_node;
extern vector_t *sql_new_vector;
extern list_t sql_parse_tree;

sql_node_t *sql_node_new(int size, int tag);

#define sql_node_make(_type_)						\
	((sql_##_type_##_t *)sql_node_new(sizeof(sql_##_type_##_t), T_##_type_))

#define sql_node_get_type(nodeptr)		(((sql_node_t*)(nodeptr))->type)
#define sql_node_set_type(nodeptr,t)		(((sql_node_t*)(nodeptr))->type = (t))
#define sql_node_is_type(nodeptr, _type_)	(sql_node_get_type(nodeptr) == T_##_type_)
#define sql_node_insert(nodeptr)					\
	do {								\
		list_insert_head(&nodeptr->list, &sql_parse_tree);	\
	} while (0)

#define sql_vector_new(free, cmp, ele)					\
	( sql_new_vector = create_vector(free, cmp),			\
	  set_element(sql_new_vector, 0, ele), sql_new_vector)
#define sql_vector_empty(free, cmp)					\
	( sql_new_vector = create_vector(free, cmp), sql_new_vector)
#define sql_vector_add(vector, element)					\
	(append_element(vector, element, 0), vector)

#endif /* __SQL_NODE_H_INCLUDE__ */
