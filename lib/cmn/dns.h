/*
 * ZETALOG's Personal COPYRIGHT
 *
 * Copyright (c) 2003
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
 * @(#)dns.h: domain name service (DNS) interface
 * $Id: dns.h,v 1.2 2007/03/10 06:55:55 cvsroot Exp $
 */

#ifndef __DNS_H_INCLUDE__
#define __DNS_H_INCLUDE__

#define DNS_PORT	53
#define DNS_UDP_PORT	DNS_PORT	/* UDP transport */
#define DNS_TCP_PORT	DNS_PORT	/* TCP transport */

/* Header.QR values */
#define DNS_QR_QUERY			0
#define DNS_QR_RESPONSE			1
/* Header.Opcode values */
#define DNS_OP_QUERY			0
#define DNS_OP_IQUERY			1
#define DNS_OP_STATUS			2
/* Header.RCODE values */
#define DNS_RCODE_NO_ERROR		0	/* no error condition */
#define DNS_RCODE_FORMAT_ERROR		1	/* format error */
#define DNS_RCODE_SERVER_FAILURE	2	/* server failure */
#define DNS_RCODE_NAME_ERROR		3	/* name error */
#define DNS_RCODE_NOT_IMPLEMENTED	4	/* not implemented */
#define DNS_RCODE_REFUSED		5	/* refused */
/* RR.TYPE (also Question.QTYPE) values */
#define DNS_TYPE_A			1	/* host address */
#define DNS_TYPE_NS			2	/* authoritative name server */
#define DNS_TYPE_MD			3	/* mail destination (Obsolete - use MX) */
#define DNS_TYPE_MF			4	/* mail forward (Obsolete - use MX) */
#define DNS_TYPE_CNAME			5	/* canonical name for an alias */
#define DNS_TYPE_SOA			6	/* start of a zone of authority */
#define DNS_TYPE_MB			7	/* mailbox domain name (EXPERIMENTAL) */
#define DNS_TYPE_MG			8	/* mail group member (EXPERIMENTAL) */
#define DNS_TYPE_MR			9	/* mail rename domain name (EXPERIMENTAL) */
#define DNS_TYPE_NULL			10	/* null RR (EXPERIMENTAL) */
#define DNS_TYPE_WKS			11	/* well known service description */
#define DNS_TYPE_PTR			12	/* domain name pointer */
#define DNS_TYPE_HINFO			13	/* host information */
#define DNS_TYPE_MINFO			14	/* mailbox or mail list information */
#define DNS_TYPE_MX			15	/* mail exchange */
#define DNS_TYPE_TXT			16	/* text strings */
/* additional Question.QTYPE values */
#define DNS_QTYPE_AXFR			252	/* request for a transfer of an entire zone */
#define DNS_QTYPE_MAILB			253	/* request for mailbox-related records (MB, MG or MR) */
#define DNS_QTYPE_MAILA			254	/* request for mail agent RRs (Obsolete - see MX) */
#define DNS_QTYPE_ALL			255	/* (*) request for all records */
/* RR.CLASS (also Question.QCLASS) values */
#define DNS_CLASS_IN			1	/* internet */
#define DNS_CLASS_CS			2	/* CSNET class (Obsolete) */
#define DNS_CLASS_CH			3	/* CHAOS class */
#define DNS_CLASS_HS			4	/* Hesiod [Dyer 87] */
/* additional Question.QCLASS values */
#define DNS_QCLASS_ANY			255	/* (*) for any class */
/* RR.TTL values */
#define DNS_TTL_UNCACHED		0	/* for the transaction in progress, should not be cached */
/* section types for append/remove */
#define DNS_SECTION_QD			0	/* question section */
#define DNS_SECTION_AN			1	/* answer section */
#define DNS_SECTION_NS			2	/* authority section */
#define DNS_SECTION_AR			3	/* additional section */

/**
 * Header Section Format
 *
 *    0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |                      ID                       |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |                    QDCOUNT                    |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |                    ANCOUNT                    |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |                    NSCOUNT                    |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |                    ARCOUNT                    |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 */
struct _dns_header_t {
    uint16_t id;		/* identifier */
    uint16_t qr:1;		/* query or response */
    uint16_t opcode:4;		/* option code */
    uint16_t aa:1;		/* authoritative answer */
    uint16_t tc:1;		/* truncation */
    uint16_t rd:1;		/* recursion desired */
    uint16_t ra:1;		/* recursion available */
    uint16_t z:3;		/* reserved for future use */
    uint16_t rcode:4;		/* response code */
    uint16_t qdcount;		/* number of entries in query section */
    uint16_t ancount;		/* number of RRs in answer section */
    uint16_t nscount;		/* number of NS RRs in authority section */
    uint16_t arcount;		/* number of RRs in addition section */
};
typedef struct _dns_header_t dns_header_t;

/**
 * Question Section Format:
 *
 *    0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |                                               |
 *  /                     QNAME                     /
 *  /                                               /
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |                     QTYPE                     |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |                     QCLASS                    |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 */
struct _dns_question_t {
	char *qname;		/* dotted name */
	uint16_t qtype;		/* question type */
	uint16_t qclass;	/* question class */
};
typedef struct _dns_question_t dns_question_t;

/**
 * Resource Record Format:
 *
 * Answer/Authority/Additional Sections all shares this format.
 *
 *    0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |                                               |
 *  /                                               /
 *  /                      NAME                     /
 *  |                                               |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |                      TYPE                     |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |                     CLASS                     |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |                      TTL                      |
 *  |                                               |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |                   RDLENGTH                    |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--|
 *  /                     RDATA                     /
 *  /                                               /
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *
 * For IN class, RDATA is 4 octet ARPA internet address.
 */
struct _dns_resource_t {
	char *name;                 /* domain name */
	uint16_t type;              /* resource data type */
	uint16_t klass;             /* resource data class */
	uint16_t ttl;               /* time interval (in seconds) cached */
	uint16_t rdlength;          /* resource data length */
	unsigned char *rdata;       /* resource data */
	int rdoffset;
};
typedef struct _dns_resource_t dns_resource_t;

/**
 * Message Format
 *
 *  +---------------------+
 *  |        Header       |
 *  +---------------------+
 *  |       Question      | the question for the name server
 *  +---------------------+
 *  |        Answer       | RRs answering the question
 *  +---------------------+
 *  |      Authority      | RRs pointing toward an authority
 *  +---------------------+
 *  |      Additional     | RRs holding additional information
 *  +---------------------+
 *
 * Header section is always present.
 */
struct _dns_message_t {
	dns_header_t header;		/* header section */
	dns_question_t *question;	/* question section */
	dns_resource_t *answer;		/* answer section */
	dns_resource_t *authority;	/* authority section */
	dns_resource_t *additional;	/* additional section */
};
typedef struct _dns_message_t dns_message_t;

struct _dns_section_t {
	union {
		dns_question_t question;
		dns_resource_t resource;
	} record;
};
typedef struct _dns_section_t dns_section_t;

/*=========================================================================
 * FUNCTION:      dns_encode_message()
 * TYPE:          external DNS operation
 * OVERVIEW:      Encode DNS message buffer.
 * INTERFACE:
 *   parameters:  DNS message
 *                message buffer
 *                buffer length
 *   returns:     encoded length
 * NOTE:          Return -1 on failure.
 *=======================================================================*/
int dns_encode_message(dns_message_t *message, uint8_t *buffer, int length);

/*=========================================================================
 * FUNCTION:      dns_decode_message()
 * TYPE:          external DNS operation
 * OVERVIEW:      Decode DNS message buffer.
 * INTERFACE:
 *   parameters:  DNS message
 *                message buffer
 *                buffer length
 *   returns:     decoded length
 * NOTE:          Return -1 on failure.
 *=======================================================================*/
int dns_decode_message(dns_message_t *message, uint8_t *buffer, int length);

/*=========================================================================
 * FUNCTION:      dns_append_section()
 * TYPE:          external DNS operation
 * OVERVIEW:      Append section record into message handle.
 * INTERFACE:
 *   parameters:  DNS message
 *                append section
 *                one section record
 *   returns:     error indication
 * NOTE:          This function returns -1 on failure.
 *
 *                Parameter "section" should be following values:
 *                DNS_SECTION_QD: question record 
 *                DNS_SECTION_AN: resource record as answer
 *                DNS_SECTION_NS: resource record as authority
 *                DNS_SECTION_AR: resource record as additional
 *
 *                Record will be duplicated into message handle, caller
 *                should destroy input record parameter by himself.
 *=======================================================================*/
int dns_append_section(dns_message_t *message, int section, dns_section_t *record);

/*=========================================================================
 * FUNCTION:      dns_release_message()
 * TYPE:          external DNS operation
 * OVERVIEW:      Release message content.
 * INTERFACE:
 *   parameters:  DNS message
 *   returns:     <none>
 * NOTE:          This function will not release message handle itself.
 *=======================================================================*/
void dns_release_message(dns_message_t *message);

#endif /* __DNS_H_INCLUDE__ */
