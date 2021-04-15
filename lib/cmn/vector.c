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
 * @(#)vector.c: vector data structure implementation
 * $Id: vector.c,v 1.2 2007/03/10 06:55:55 cvsroot Exp $
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "vector.h"

#define INCREMENT   10

int extend_vector(vector_t *vector, int required)
{
	void **elements = NULL;
	int size;
	
	if (!vector)
		return -(DS_EARGUMENT);
	
	if (required == 0)
		required = vector->size;
	
	size = required+INCREMENT;
	
	/* FIXME: should use realloc, but will crash with Microsoft C library */
	elements = (void **)malloc((sizeof (void *))*size);
	
	if (!elements) {
		fprintf(stderr, "Cannot extends vector.\n");
		return -(DS_EMEMORY);
	} else {
		memset(elements, 0, (sizeof (void *))*size);
		/* copy old vector */
		if (vector->elements)
			memcpy(elements, vector->elements, (sizeof (void *))*vector->size);
		
		vector->size = size;
		
		/* release old vector */
		if (vector->elements)   /* need not this? */
			free(vector->elements);
		vector->elements = elements;
	}
	return DS_EMEMORY;
}

vector_t *create_vector(DESTROYER destroyer, COMPARER comparer)
{
	VECTOR vector = malloc(sizeof (vector_t));
	
	if (vector) {
		/* initialize vector */
		memset(vector, 0, sizeof (vector_t));
		vector->destroyer = destroyer;
		vector->comparer = comparer;
		
		/* create default vector */
		vector->count = 0;
		vector->elements = (void **)malloc((sizeof(void *))*INCREMENT);
		
		if (vector->elements) {
			vector->size = INCREMENT;
			memset(vector->elements, 0, (sizeof(void*))*INCREMENT);
		}
	}
	return vector;
}

void *get_element(vector_t *vector, int index)
{
	if (vector && index < vector->count)
		return vector->elements[index];
	return NULL;
}

int element_count(vector_t *vector)
{
	if (vector)
		return vector->count;
	return 0;
}

int vector_size(vector_t *vector)
{
	if (vector)
		return vector->size;
	return 0;
}

int find_element(vector_t *vector, void *element)
{
	int index;
	int found = 0;
	
	if (!vector)
		return -(DS_EARGUMENT);
	
	for (index = 0; index < vector->count; index++) {
		if ((vector->elements[index] == element) || 
			(vector->comparer && element && vector->elements[index] &&
			!vector->comparer(vector->elements[index], element))) {
			found = 1;
			break;
		}
	}
	return (found ? index : -(DS_ENOELEMENT));
}

void *pop_element(vector_t *vector)
{
	if (vector) {
		if (vector->count > 0) {
			void *element = vector->elements[vector->count-1];
			vector->elements[vector->count-1] = NULL;
			vector->count--;
			return element;
		}
	}
	return NULL;
}

void push_element(vector_t *vector, void *element)
{
	append_element(vector, element, 0);
}

void set_element(vector_t *vector, int index, void *element)
{
	if (vector) {
		if (index < vector->size) {
			/* release the old element */
			if (index < vector->count && vector->destroyer)
				vector->destroyer(vector->elements[index]);
			
			/* set element and count */
			vector->elements[index] = element;
			vector->count = vector->count > index+1 ? vector->count : index+1;
		} else if (extend_vector(vector, index+1)) {
			/* readd element */
			set_element(vector, index, element);
		}
	}
}

int compare_vector(vector_t *vector1, vector_t *vector2)
{
	if (vector1 && vector2) {
		if (vector1->count == vector2->count) {
			int index;
			int count = vector1->count;
			
			for (index = 0; index < count; index++) {
				void *element1 = get_element(vector1, index);
				void *element2 = get_element(vector2, index);
				
				if (element1 != element2) {
					if (vector1->comparer)
						return vector1->comparer(element1, element2);
					if (vector2->comparer)
						return vector2->comparer(element1, element2);
				} else
					return 0;
			}
		}
		else
			return vector2->count - vector1->count;
	}
	return vector2-vector1;
}

void traverse_vector(vector_t *vector, TRAVERSER traverser, void *handle)
{
	int index;
	int count;
	
	if (!traverser || !vector)
		return;
	
	count = vector->count;
	
	for (index = 0; index < count; index++) {
		void *element = get_element(vector, index);
		
		if (traverser(handle, index, element) == VECTOR_TRAVERSE_BREAK)
			break;
	}
}

/*
 * TODO: singleton should not be a parameter of this function.
 *       it should be defined in vector->type during creation.
 */
void append_element(vector_t *vector, void *element, int singleton)
{
	if (vector) {
		if (singleton) {
			int index = find_element(vector, element);
			
			if (index > -1) {
				if (vector->destroyer)
					vector->destroyer(element);
				return;
			}
		}
		
		if (vector->count < vector->size) {
			/* unused block exists */
			vector->elements[vector->count] = element;
			vector->count++;
		} else if (extend_vector(vector, 0)) {
			/* readd element */
			append_element(vector, element, singleton);
		}
	}
}

void remove_element(vector_t *vector, int index)
{
        int i = index;
        if (vector) {
                if (index < 0 || index >= vector->count)
                        return;
                if (vector->destroyer)
                        vector->destroyer(vector->elements[i]);
                else
                        free(vector->elements[i]);
                for (; i < vector->count - 1; i++)
                        vector->elements[i] = vector->elements[i+1];
                vector->count--;
        }
}

void sort_elements(vector_t *vector, COMPARER comparer)
{
        if (vector) {
                int i = 0, k = 0;
                int j = vector->count - 1;
                void *temp;

                for (; i < vector->count - 1; i++,j--) {
                        for (k = 0; k < j; k++) {
                                if (comparer(vector->elements[k], vector->elements[k+1]) > 0) {
                                        temp = vector->elements[k];
                                        vector->elements[k] = vector->elements[k+1];
                                        vector->elements[k+1] = temp;
                                }
			}
		}
        }
}

void destroy_vector(vector_t *vector)
{
	if (vector) {
		if (vector->elements) {
			/* destroy maintained elements */
			int index;
			
			for (index = 0; index < vector->count; index++) {
				if (vector->destroyer/* && vector->elements[index]*/)
					vector->destroyer(vector->elements[index]);
			}
			free(vector->elements);
		}
		free(vector);
	}
}
