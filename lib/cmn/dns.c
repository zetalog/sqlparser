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
 * @(#)dns.c: domain name service (DNS) routines
 * $Id: dns.c,v 1.2 2007/03/10 06:55:55 cvsroot Exp $
 */

#include <proto.h>
#include <dns.h>

static int dns_encode_header(dns_header_t *header,
                             uint8_t *buffer, int length);
static int dns_decode_header(dns_header_t *header,
                             uint8_t *buffer);
static int dns_encode_dotted(const char *dotted,
                             uint8_t *buffer, int length);
static int dns_decode_dotted(const uint8_t *buffer, int offset, 
                             char *dotted, int length);
static int dns_dotted_length(const uint8_t *buffer, int offset);
static int dns_encode_question(dns_question_t *question,
                               uint8_t *buffer, int length);
static int dns_decode_question(dns_question_t *question,
                               uint8_t *buffer, int offset);
static int dns_question_length(uint8_t *buffer, int offset);
static int dns_encode_resource(dns_resource_t *resource,
                               uint8_t *buffer, int length);
static int dns_decode_resource(dns_resource_t *resource,
                               uint8_t *buffer, int offset);
static int dns_append_resource(dns_message_t *message,
                               int section, dns_resource_t *resource);
static int dns_append_question(dns_message_t *message,
                               dns_question_t *question);

/*=========================================================================
 * FUNCTION:      dns_encode_header()
 * TYPE:          internal DNS operation
 * OVERVIEW:      Encoding DNS query header into message buffer.
 * INTERFACE:
 *   parameters:  IN message header
 *                INOUT message buffer
 *                IN buffer length
 *   returns:     encoded length
 * NOTE:          Return -1 on failure.
 *=======================================================================*/
int dns_encode_header(dns_header_t *header, uint8_t *buffer, int length)
{
	if (length < 12)
		return -1;
	
	buffer[0]  = (header->id & 0xff00) >> 8;
	buffer[1]  = (header->id & 0x00ff) >> 0;
	buffer[2]  = (header->qr ? 0x80 : 0) |
		((header->opcode & 0x0f) << 3) |
		(header->aa ? 0x04 : 0) |
		(header->tc ? 0x02 : 0) |
		(header->rd ? 0x01 : 0);
	buffer[3]  = (header->ra ? 0x80 : 0) |
		(header->rcode & 0x0f);
	buffer[4]  = (header->qdcount & 0xff00) >> 8;
	buffer[5]  = (header->qdcount & 0x00ff) >> 0;
	buffer[6]  = (header->ancount & 0xff00) >> 8;
	buffer[7]  = (header->ancount & 0x00ff) >> 0;
	buffer[8]  = (header->nscount & 0xff00) >> 8;
	buffer[9]  = (header->nscount & 0x00ff) >> 0;
	buffer[10] = (header->arcount & 0xff00) >> 8;
	buffer[11] = (header->arcount & 0x00ff) >> 0;
	
	return 12;
}

/*=========================================================================
 * FUNCTION:      dns_decode_header()
 * TYPE:          internal DNS operation
 * OVERVIEW:      Decoding DNS query header into message buffer.
 * INTERFACE:
 *   parameters:  OUT message header
 *                IN message buffer
 *   returns:     decoded length
 * NOTE:          Return -1 on failure.
 *=======================================================================*/
int dns_decode_header(dns_header_t *header, uint8_t *buffer)
{
	header->id      = (buffer[0]  << 8) | buffer[1];
	header->qr      = (buffer[2]  & 0x80) ? 1 : 0;
	header->opcode  = (buffer[2]  >> 3) & 0x0f;
	header->aa      = (buffer[2]  & 0x04) ? 1 : 0;
	header->tc      = (buffer[2]  & 0x02) ? 1 : 0;
	header->rd      = (buffer[2]  & 0x01) ? 1 : 0;
	header->ra      = (buffer[3]  & 0x80) ? 1 : 0;
	header->rcode   = (buffer[3]  & 0x0f);
	header->qdcount = (buffer[4]  << 8) | buffer[5];
	header->ancount = (buffer[6]  << 8) | buffer[7];
	header->nscount = (buffer[8]  << 8) | buffer[9];
	header->arcount = (buffer[10] << 8) | buffer[11];
	
	return 12;
}

/*=========================================================================
 * FUNCTION:      dns_encode_dotted()
 * TYPE:          internal DNS operation
 * OVERVIEW:      Encode a dotted string into nameserver transport-level
 *                encoding.
 * INTERFACE:
 *   parameters:  dotted string
 *                encoding buffer
 *                buffer length
 *   returns:     encoded length
 * NOTE:          Return -1 on failure. This routine is fairly dumb, and
 *                doesn't attempt to compress the data.
 *=======================================================================*/
int dns_encode_dotted(const char *dotted, uint8_t *buffer, int length)
{
	int used = 0;
	
	while (dotted && *dotted) {
		char * c = strchr(dotted, '.');
		int l = c ? c-dotted : strlen(dotted);
		
		if (l >= (length-used-1))
			return -1;
		
		buffer[used++] = l;
		memcpy(buffer+used, dotted, l);
		used += l;
		
		if (c)
			dotted = c+1;
		else
			break;
	}
	
	if (length < 1)
		return -1;
	
	buffer[used++] = 0;
	
	return used;
}

/*=========================================================================
 * FUNCTION:      dns_decode_dotted()
 * TYPE:          internal DNS operation
 * OVERVIEW:      Decode a dotted string from nameserver transport-level
 *                encoding.
 * INTERFACE:
 *   parameters:  encoding buffer
 *                buffer offset
 *                dotted string
 *                string max length
 *   returns:     decoded length
 * NOTE:          Return -1 on failure.
 *=======================================================================*/
int dns_decode_dotted(const uint8_t *buffer, int offset, 
                      char *dotted, int length)
{
	int l;
	int measure = 1;
	int total = 0;
	int used = 0;
	
	if (!buffer)
		return -1;
	
	while ((measure && total++), (l=buffer[offset++]) & 0x00ff) {
		if ((l & 0xc0) == (0xc0)) {
			if (measure)
				total++;
			/* compressed item, redirect */
			offset = ((l & 0x3f) << 8) | buffer[offset];
			measure = 0;
			continue;
		}
		
		if ((used+l+1) >= length)
			return -1;
		
		memcpy(dotted+used, buffer+offset, l);
		offset += l;
		used += l;
		if (measure)
			total += l;
		
		if (buffer[offset] != 0)
			dotted[used++] = '.';
		else
			dotted[used++] = '\0';
	}
	
	return total;
}

/*=========================================================================
 * FUNCTION:      dns_dotted_length()
 * TYPE:          internal DNS operation
 * OVERVIEW:      Get dotted string length.
 * INTERFACE:
 *   parameters:  dotted buffer
 *                buffer offset
 *   returns:     dotted length
 * NOTE:          Return -1 on failure.
 *=======================================================================*/
int dns_dotted_length(const uint8_t *dotted, int offset)
{
	int orig_offset = offset;
	int l;
	
	if (!dotted)
		return -1;
	
	while ((l = dotted[offset++])) {
		if ((l & 0xc0) == (0xc0)) {
			offset++;
			break;
		}
		offset += l;
	}
	
	return offset-orig_offset;
}

/*=========================================================================
 * FUNCTION:      dns_encode_question()
 * TYPE:          internal DNS operation
 * OVERVIEW:      Encoding DNS question section into message buffer.
 * INTERFACE:
 *   parameters:  question section
 *                message buffer
 *                buffer length
 *   returns:     encoded length
 * NOTE:          Return -1 on failure.
 *=======================================================================*/
int dns_encode_question(dns_question_t *question,
                        uint8_t *buffer, int length)
{
	int i;
	
	i = dns_encode_dotted(question->qname, buffer, length);
	if (i < 0)
		return i;
	
	buffer += i;
	length -= i;
	
	if (length < 4)
		return -1;
	
	buffer[0] = (question->qtype & 0xff00) >> 8;
	buffer[1] = (question->qtype & 0x00ff) >> 0;
	buffer[2] = (question->qclass & 0xff00) >> 8;
	buffer[3] = (question->qclass & 0x00ff) >> 0;
	
	return i+4;
}

/*=========================================================================
 * FUNCTION:      dns_decode_question()
 * TYPE:          internal DNS operation
 * OVERVIEW:      Decoding message buffer into DNS question section.
 * INTERFACE:
 *   parameters:  question section
 *                message buffer
 *                buffer offset
 *   returns:     decoded length
 * NOTE:          Return -1 on failure.
 *=======================================================================*/
int dns_decode_question(dns_question_t *question,
                        uint8_t *buffer, int offset)
{
	char temp[256];
	int i;
	
	i = dns_decode_dotted(buffer, offset, temp, 256);
	if (i < 0)
		return i;
	
	offset += i;
	
	question->qname  = (unsigned char *)strdup(temp);
	question->qtype  = (buffer[offset+0] << 8) | buffer[offset+1];
	question->qclass = (buffer[offset+2] << 8) | buffer[offset+3];
	
	return i+4;
}

/*=========================================================================
 * FUNCTION:      dns_question_length()
 * TYPE:          internal DNS operation
 * OVERVIEW:      Get length of question section.
 * INTERFACE:
 *   parameters:  message buffer
 *                buffer offset
 *   returns:     question length
 * NOTE:          Return -1 on failure.
 *=======================================================================*/
int dns_question_length(uint8_t *buffer, int offset)
{
	int i;
	
	i = dns_dotted_length(buffer, offset);
	if (i < 0)
		return i;
	
	return i+4;
}

/*=========================================================================
 * FUNCTION:      dns_encode_resource()
 * TYPE:          internal DNS operation
 * OVERVIEW:      Encoding DNS resource record into message buffer.
 * INTERFACE:
 *   parameters:  resource record
 *                message buffer
 *                buffer length
 *   returns:     encoded length
 * NOTE:          Return -1 on failure.
 *=======================================================================*/
int dns_encode_resource(dns_resource_t *resource,
                        uint8_t *buffer, int length)
{
	int i;
	
	i = dns_encode_dotted(resource->name, buffer, length);
	if (i < 0)
		return i;
	
	buffer += i;
	length -= i;
	
	if (length < (10+resource->rdlength))
		return -1;
	
	*buffer++ = (resource->type & 0xff00) >> 8;
	*buffer++ = (resource->type & 0x00ff) >> 0;
	*buffer++ = (resource->klass & 0xff00) >> 8;
	*buffer++ = (resource->klass & 0x00ff) >> 0;
	*buffer++ = (resource->ttl & 0xff000000) >> 24;
	*buffer++ = (resource->ttl & 0x00ff0000) >> 16;
	*buffer++ = (resource->ttl & 0x0000ff00) >> 8;
	*buffer++ = (resource->ttl & 0x000000ff) >> 0;
	*buffer++ = (resource->rdlength & 0xff00) >> 8;
	*buffer++ = (resource->rdlength & 0x00ff) >> 0;
	memcpy(buffer, resource->rdata, resource->rdlength);
	
	return i+10+resource->rdlength;
}

/*=========================================================================
 * FUNCTION:      dns_decode_resource()
 * TYPE:          internal DNS operation
 * OVERVIEW:      Decoding message buffer into DNS resource record.
 * INTERFACE:
 *   parameters:  resource record
 *                buffer offset
 *                message buffer
 *   returns:     decoded length
 * NOTE:          Return -1 on failure.
 *=======================================================================*/
int dns_decode_resource(dns_resource_t *resource,
                        uint8_t *buffer, int offset)
{
	char temp[256];
	int i;
	
	i = dns_decode_dotted(buffer, offset, temp, 256);
	if (i < 0)
		return i;
	
	buffer += offset+i;
	
	resource->name  = (unsigned char *)strdup(temp);
	resource->type  = (buffer[0] << 8)  | buffer[1]; buffer += 2;
	resource->klass = (buffer[0] << 8)  | buffer[1]; buffer += 2;
	resource->ttl   = (buffer[0] << 24) | (buffer[1] << 16) |
		(buffer[2] << 8)  | (buffer[3] << 0);
	buffer += 4;
	resource->rdlength = (buffer[0] << 8) | buffer[1]; buffer += 2;
	resource->rdata = malloc(resource->rdlength);
	if (resource->rdata)
		memcpy(resource->rdata, buffer, resource->rdlength);
	resource->rdoffset = offset+i+10;
	
	return i+10+resource->rdlength;
}

/*=========================================================================
 * FUNCTION:      dns_extend_resource()
 * TYPE:          internal DNS operation
 * OVERVIEW:      Extend resource array.
 * INTERFACE:
 *   parameters:  resource record array
 *                array size
 *                resource record to be appended
 *   returns:     reallocated array
 * NOTE:          If succeeded, value pointed by index will be
 *                increased.
 *=======================================================================*/
dns_resource_t *dns_extend_resource(dns_resource_t *resource,
                                    unsigned short size,
                                    dns_resource_t *record)
{
	dns_resource_t *extended = NULL;
	
	if (resource)
		extended = realloc(resource, (size+1)*sizeof (dns_resource_t));
	else
		extended = malloc((size+1)*sizeof (dns_resource_t));
	
	if (extended) {
		if (record->name)
			extended[size].name = strdup(record->name);
		extended[size].type     = record->type;
		extended[size].klass    = record->klass;
		extended[size].rdlength = record->rdlength;
		extended[size].rdata    = malloc(record->rdlength);
		if (extended[size].rdata)
			memcpy(extended[size].rdata, record->rdata, record->rdlength);
		extended[size].rdoffset = record->rdoffset;
	}
	return extended;
}

/*=========================================================================
 * FUNCTION:      dns_append_resource()
 * TYPE:          internal DNS operation
 * OVERVIEW:      Append resource record into message handle.
 * INTERFACE:
 *   parameters:  DNS message
 *                append section
 *                resource record
 *   returns:     error indication
 * NOTE:          This function returns -1 on failure.
 *
 *                Parameter "section" should be following values:
 *                DNS_SECTION_AN: resource record as answer
 *                DNS_SECTION_NS: resource record as authority
 *                DNS_SECTION_AR: resource record as additional
 *
 *                Resource will be duplicated into message handle,
 *                caller should destroy input resource parameter by
 *                himself.
 *=======================================================================*/
int dns_append_resource(dns_message_t *message, int section,
                        dns_resource_t *resource)
{
	dns_resource_t *position = NULL;
	
	switch (section) {
	case DNS_SECTION_AN:
		/* extend answer section */
		position = dns_extend_resource(message->answer,
					       message->header.ancount,
					       resource);
		if (position) {
			message->answer = position;
			message->header.ancount++;
		}
		break;
	case DNS_SECTION_NS:
		/* extend authority section */
		position = dns_extend_resource(message->authority,
					       message->header.nscount,
			resource);
		if (position) {
			message->authority = position;
			message->header.nscount++;
		}
		break;
	case DNS_SECTION_AR:
		/* extend additional section */
		position = dns_extend_resource(message->additional,
					       message->header.arcount,
					       resource);
		if (position) {
			message->additional = position;
			message->header.arcount++;
		}
		break;
	default:
		break;
	}
	
	return position?0:-1;
}

int dns_append_question(dns_message_t *message, dns_question_t *question)
{
	dns_question_t *extended = NULL;
	
	if (message->question)
		extended = realloc(message->question,
		(message->header.qdcount+1)*sizeof (dns_question_t));
	else
		extended = malloc((message->header.qdcount+1)*sizeof (dns_question_t));
	if (extended) {
		extended[message->header.qdcount].qtype = question->qtype;
		extended[message->header.qdcount].qclass = question->qclass;
		if (question->qname)
			extended[message->header.qdcount].qname = strdup(question->qname);
		message->question = extended;
		message->header.qdcount++;
	}
	return extended?0:-1;
}

int dns_encode_message(dns_message_t *message,
                       uint8_t *buffer, int length)
{
	int i, total = 0;
	int j;
	
	/* build header section */
	i = dns_encode_header(&(message->header), buffer, length);
	if (i < 0)
		return i;
	buffer += i;
	length -= i;
	total += i;
	
	/* build question section */
	for (j = 0; j < message->header.qdcount; j++) {
		i = dns_encode_question(&(message->question[j]), buffer, length);
		if (i < 0)
			return i;
		buffer += i;
		length -= i;
		total += i;
	}
	
	/* build answer section */
	for (j = 0; j < message->header.ancount; j++) {
		i = dns_encode_resource(&(message->answer[j]), buffer, length);
		if (i < 0)
			return i;
		buffer += i;
		length -= i;
		total += i;
	}
	/* build authority section */
	for (j = 0; j < message->header.nscount; j++) {
		i = dns_encode_resource(&(message->authority[j]), buffer, length);
		if (i < 0)
			return i;
		buffer += i;
		length -= i;
		total += i;
	}
	/* build additional section */
	for (j = 0; j < message->header.arcount; j++) {
		i = dns_encode_resource(&(message->additional[j]), buffer, length);
		if (i < 0)
			return i;
		buffer += i;
		length -= i;
		total += i;
	}
	
	return total;
}

int dns_decode_message(dns_message_t *message,
                       uint8_t *buffer, int length)
{
	int i, total = 0;
	int j;
	
	/* parse header section */
	i = dns_decode_header(&(message->header), buffer);
	if (i < 0)
		return i;
	buffer += i;
	length -= i;
	total += i;
	
	/* parse question section */
	if (message->header.qdcount) {
		message->question = (dns_question_t *)calloc(message->header.qdcount,
			sizeof (dns_question_t));
		if (message->question) {
			for (j = 0; j < message->header.qdcount; j++) {
				i = dns_decode_question(&(message->question[j]), buffer, 0);
				if (i < 0)
					goto done;
				buffer += i;
				length -= i;
				total += i;
			}
		}
	}
	
	/* parse answer section */
	if (message->header.ancount) {
		message->answer = (dns_resource_t *)calloc(message->header.ancount,
			sizeof (dns_resource_t));
		if (message->answer) {
			for (j = 0; j < message->header.ancount; j++) {
				i = dns_decode_resource(&(message->answer[j]), buffer, 0);
				if (i < 0)
					goto done;
				buffer += i;
				length -= i;
				total += i;
			}
		}
	}
	/* parse authority section */
	if (message->header.nscount) {
		message->authority = (dns_resource_t *)calloc(message->header.nscount,
			sizeof (dns_resource_t));
		if (message->authority) {
			for (j = 0; j < message->header.nscount; j++) {
				i = dns_decode_resource(&(message->authority[j]), buffer, 0);
				if (i < 0)
					goto done;
				buffer += i;
				length -= i;
				total += i;
			}
		}
	}
	/* parse additional section */
	if (message->header.arcount) {
		message->additional = (dns_resource_t *)calloc(message->header.arcount,
			sizeof (dns_resource_t));
		if (message->additional) {
			for (j = 0; j < message->header.arcount; j++) {
				i = dns_decode_resource(&(message->additional[j]), buffer, 0);
				if (i < 0)
					goto done;
				buffer += i;
				length -= i;
				total += i;
			}
		}
	}
	
done:
	if (i < 0) {
		dns_release_message(message);
		return i;
	}
	return total;
}

void dns_release_message(dns_message_t *message)
{
	int i;
	
	if (message) {
		/* release question section */
		if (message->question) {
			for (i = 0; i < message->header.qdcount; i++) {
				if (message->question[i].qname)
					free(message->question[i].qname);
			}
			free(message->question);
		}
		/* release answer section */
		if (message->answer) {
			for (i = 0; i < message->header.ancount; i++) {
				if (message->answer[i].name)
					free(message->answer[i].name);
				if (message->answer[i].rdata)
					free(message->answer[i].rdata);
			}
			free(message->answer);
		}
		/* release authority section */
		if (message->authority) {
			for (i = 0; i < message->header.nscount; i++) {
				if (message->authority[i].name)
					free(message->authority[i].name);
				if (message->authority[i].rdata)
					free(message->authority[i].rdata);
			}
			free(message->authority);
		}
		/* release additional section */
		if (message->additional) {
			for (i = 0; i < message->header.arcount; i++) {
				if (message->additional[i].name)
					free(message->additional[i].name);
				if (message->additional[i].rdata)
					free(message->additional[i].rdata);
			}
			free(message->additional);
		}
	}
}

int dns_append_section(dns_message_t *message, int section,
                       dns_section_t *record)
{
	if (section == DNS_SECTION_QD)
		return dns_append_question(message, (dns_question_t *)record);
	else
		return dns_append_resource(message, section, (dns_resource_t *)record);
}

#ifdef TEST
int main(int argc, char **argv)
{
	dns_message_t encoded, decoded;
	dns_question_t question;
	dns_resource_t resource;
	char hostname[20];
	char buffer[512];
	int res;
	
	memset(&encoded, 0, sizeof (dns_message_t));
	memset(&decoded, 0, sizeof (dns_message_t));
	
	encoded.header.id = 0x1234;
	encoded.header.qr = DNS_QR_RESPONSE;
	encoded.header.opcode = DNS_OP_QUERY;
	encoded.header.rcode = DNS_RCODE_NO_ERROR;
	
	question.qtype = DNS_TYPE_PTR;
	question.qclass = DNS_CLASS_IN;
	question.qname = strdup("1.100.168.192.in-addr.arpa");
	
	resource.klass = DNS_CLASS_IN;
	/* keep one hour */
	resource.ttl = 3600;
	resource.type = DNS_TYPE_PTR;
	resource.name = strdup("1.100.168.192.in-addr.arpa");
	sprintf(hostname, "foobar%u", 0xffffffff);
	resource.rdata = hostname;
	resource.rdlength = strlen(resource.rdata) + 1;
	
	dns_append_section(&encoded, DNS_SECTION_QD, (dns_section_t *)&question);
	dns_append_section(&encoded, DNS_SECTION_AN, (dns_section_t *)&resource);
	res = dns_encode_message(&encoded, buffer, 512);
	
	dns_decode_message(&decoded, buffer, res);
	
	dns_release_message(&encoded);
	dns_release_message(&decoded);
	
	free(question.qname);
	free(resource.name);
	return 0;
}
#endif
