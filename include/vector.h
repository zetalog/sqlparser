/*
 * Copyright (c) 2004
 *    Author: Lv "Zetalog" Zheng
 *    Internet: zhengl@sh.necas.nec.com.cn
 *
 * Redistribution and use in source and binary forms in company, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by the NEC-CAS Shanghai
 *    Development Center.
 * 3. Neither the name of the companay nor the names of its developer may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 4. Any out company redistribution are not permitted unless companay
 *    copyright no longer declaimed, but must not remove developer(s) above.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NEC-CAS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE NEC-CAS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @(#)vector.c: vector data structure header
 * $Id: vector.h,v 1.2 2007/03/10 06:55:55 cvsroot Exp $
 */

#ifndef __VECTOR_H_INCLUDE__
#define __VECTOR_H_INCLUDE__ 1

#ifdef __cplusplus
extern "C" {
#endif

#define DS_ESUCCESS     0
#define DS_EARGUMENT    1
#define DS_EMEMORY      2
#define DS_ENOELEMENT   3

typedef void (*DESTROYER)(void *);
typedef int (*COMPARER)(void *, void *);
typedef int (*TRAVERSER)(void * /*handle*/, int /*index*/, void * /*element*/);

typedef struct _vector_t {
	void **elements;
	int type;
	int count;
	int size;
	DESTROYER destroyer;
	COMPARER comparer;
} vector_t, *VECTOR;

/* vector's element must be unique */
#define VECTOR_VALUE_UNIQUE     1

/* vector act like a dynamic array */
#define VECTOR_INDEX_UNIQUE     2

/* traverer action */
#define VECTOR_TRAVERSE_BREAK   0

/**
 * Create vector
 */
vector_t *create_vector(DESTROYER destroyer, COMPARER comparer);

/**
 * Destroy vector
 */
void destroy_vector(vector_t *vector);

/*
 * Compare vector
 */
int compare_vector(vector_t *vector1, vector_t *vector2);

/**
 * Get element at specified index
 */
void *get_element(vector_t *vector, int index);

/**
 * Count elements
 */
int element_count(vector_t *vector);

/**
 * Get vector size
 */
int vector_size(vector_t *vector);

/**
 * Append element at the end
 */
void append_element(vector_t *vector, void *element, int singleton);

/**
 * Remove an element by its index
 */
void remove_element(vector_t *vector, int index);

/**
 * Sort all elements
 */
void sort_elements(vector_t *vector, COMPARER comparer);

/**
 * Find specified element, return found index
 */
int find_element(vector_t *vector, void *element);

/**
 * Set element at specified index
 */
void set_element(vector_t *vector, int index, void *element);

/**
 * Traverse vector, executing TRAVERSER
 */
void traverse_vector(vector_t *vector, TRAVERSER traverser, void *handle);

void push_element(vector_t *vector, void *element);
void *pop_element(vector_t *vector);

#ifdef __cplusplus
}
#endif

#endif
