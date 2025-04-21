/**
 * @file	httpUtil.h
 * @brief	Header File for HTTP Server Utilities
 * @version 1.0
 * @date	2014/07/15
 * @par Revision
 *			2014/07/15 - 1.0 Release
 * @author	
 * \n\n @par Copyright (C) 1998 - 2014 WIZnet. All rights reserved.
 */

#ifndef	__HTTPUTIL_H__
#define	__HTTPUTIL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "../../../Wiznet/Internet/httpServer/httpServer.h"
#include "../../../Wiznet/Internet/httpServer/httpParser.h"
#include <stdio.h>

typedef struct _post_request_t{
	char* content_length_header;
	uint32_t content_length;
	char* content_disposition;
	char* content_type;
	char* content_start;
	char* content_end;
}
post_request_t;

uint8_t http_get_cgi_handler(uint8_t * uri_name, uint8_t * buf, uint32_t * file_len);
uint8_t http_post_cgi_handler(uint8_t * uri_name, st_http_request * p_http_request, uint8_t * buf, uint32_t * file_len);

uint8_t predefined_get_cgi_processor(uint8_t * uri_name, uint8_t * buf, uint16_t * len);
uint8_t predefined_set_cgi_processor(uint8_t * uri_name, uint8_t * uri, uint8_t * buf, uint16_t * len);

uint8_t disassemble_post_request(st_http_request * p_http_request, post_request_t * request);

int get_query_param(const char* uri, const char* key, char* out, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif
