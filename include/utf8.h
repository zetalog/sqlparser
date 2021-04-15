/*
 * Program:	UTF-8 routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	11 June 1997
 * Last Edited:	3 June 2004
 * 
 * The IMAP toolkit provided in this Distribution is
 * Copyright 1988-2004 University of Washington.
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this Distribution.
 */

#ifndef __UTF8_H_INCLUDE__
#define __UTF8_H_INCLUDE__

#ifndef NULL
#define NULL (void *)0
#endif

#define UTF8_ERROR_SUCCESS  0
/** unknown charset */
#define UTF8_ERROR_CHARSET  1
/** encode/decode error */
#define UTF8_ERROR_CONVERT  2
/** reverse map failure */
#define UTF8_ERROR_RMAP     3

typedef struct _utf8_text_t {
    unsigned char *data;        /* text */
    unsigned long size;         /* size of text in octets */
} utf8_text_t;

/* function prototypes */
int utf8_decode_charset(utf8_text_t *text, char *charset, utf8_text_t *ret, long flags);
int utf8_encode_charset(utf8_text_t *text, char *charset, utf8_text_t *ret, unsigned short errch);
int utf8_convert_charset(utf8_text_t *text, char *sc, utf8_text_t *ret, char *dc, unsigned short errch);

#endif /* __UTF8_H_INCLUDE__ */
