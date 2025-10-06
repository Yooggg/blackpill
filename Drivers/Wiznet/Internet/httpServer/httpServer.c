#include "../../../Wiznet/Internet/httpServer/httpServer.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../../../Wiznet/Ethernet/socket.h"
#include "../../../Wiznet/Ethernet/wizchip_conf.h"
#include "../../../Wiznet/Internet/httpServer/httpParser.h"
#include "../../../Wiznet/Internet/httpServer/httpUtil.h"

#ifdef	_USE_SDCARD_
#include "ff.h" 	// header file for FatFs library (FAT file system)
#endif

#ifndef DATA_BUF_SIZE
	#define DATA_BUF_SIZE		2048
#endif

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/
uint8_t HTTPSock_Num[_WIZCHIP_SOCK_NUM_] = {0, };
static st_http_request * http_request;				/**< Pointer to received HTTP request */
static st_http_request * parsed_http_request;		/**< Pointer to parsed HTTP request */
static uint8_t * http_response;						/**< Pointer to HTTP response */
static uint8_t current_file_is_gzip = 0;

// Number of registered web content in code flash memory
static uint16_t total_content_cnt = 0;

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/
uint8_t * pHTTP_TX;
uint8_t * pHTTP_RX;

volatile uint32_t httpServer_tick_1s = 0;
st_http_socket HTTPSock_Status[_WIZCHIP_SOCK_NUM_] = { {STATE_HTTP_IDLE, }, };
httpServer_webContent web_content[MAX_CONTENT_CALLBACK];

#ifdef	_USE_SDCARD_
FIL fs;		// FatFs: File object
FRESULT fr;	// FatFs: File function return code
#endif

/*****************************************************************************
 * Private functions
 ****************************************************************************/
void httpServer_Sockinit(uint8_t cnt, uint8_t * socklist);
static uint8_t getHTTPSocketNum(uint8_t seqnum);
static int8_t getHTTPSequenceNum(uint8_t socket);
static int8_t http_disconnect(uint8_t sn);

static void http_process_handler(uint8_t s, st_http_request * p_http_request);
static void send_http_response_header(uint8_t s, uint8_t content_type, uint32_t body_len, uint16_t http_status);
static void send_http_response_body(uint8_t s, uint8_t * uri_name, uint8_t * buf, uint32_t start_addr, uint32_t file_len);
static void send_http_response_cgi(uint8_t s, uint8_t * buf, uint8_t * http_body, uint16_t file_len);

/*****************************************************************************
 * Public functions
 ****************************************************************************/
// Callback functions definition: MCU Reset / WDT Reset
void default_mcu_reset(void) {;}
void default_wdt_reset(void) {;}
void (*HTTPServer_ReStart)(void) = default_mcu_reset;
void (*HTTPServer_WDT_Reset)(void) = default_wdt_reset;

void httpServer_Sockinit(uint8_t cnt, uint8_t * socklist)
{
	uint8_t i;

	for(i = 0; i < cnt; i++)
	{
		// Mapping the H/W socket numbers to the sequential index numbers
		HTTPSock_Num[i] = socklist[i];
	}
}

static uint8_t getHTTPSocketNum(uint8_t seqnum)
{
	// Return the 'H/W socket number' corresponding to the index number
	return HTTPSock_Num[seqnum];
}

static int8_t getHTTPSequenceNum(uint8_t socket)
{
	uint8_t i;

	for(i = 0; i < _WIZCHIP_SOCK_NUM_; i++)
		if(HTTPSock_Num[i] == socket) return i;

	return -1;
}

void httpServer_init(uint8_t * tx_buf, uint8_t * rx_buf, uint8_t cnt, uint8_t * socklist)
{
	// User's shared buffer
	pHTTP_TX = tx_buf;
	pHTTP_RX = rx_buf;

	// H/W Socket number mapping
	httpServer_Sockinit(cnt, socklist);
}


/* Register the call back functions for HTTP Server */
void reg_httpServer_cbfunc(void(*mcu_reset)(void), void(*wdt_reset)(void))
{
	// Callback: HW Reset and WDT reset function for each MCU platforms
	if(mcu_reset) HTTPServer_ReStart = mcu_reset;
	if(wdt_reset) HTTPServer_WDT_Reset = wdt_reset;
}


void httpServer_run(uint8_t seqnum)
{
	uint8_t s;	// socket number
	uint16_t len;
	uint32_t gettime = 0;

#ifdef _HTTPSERVER_DEBUG_
	uint8_t destip[4] = {0, };
	uint16_t destport = 0;
#endif

	http_request = (st_http_request *)pHTTP_RX;		// Structure of HTTP Request
	parsed_http_request = (st_http_request *)pHTTP_TX;

	// Get the H/W socket number
	s = getHTTPSocketNum(seqnum);

	/* HTTP Service Start */
	switch(getSn_SR(s))
	{
		case SOCK_ESTABLISHED:
			// Interrupt clear
			if(getSn_IR(s) & Sn_IR_CON)
			{
				setSn_IR(s, Sn_IR_CON);
			}

			// HTTP Process states
			switch(HTTPSock_Status[seqnum].sock_status)
			{

				case STATE_HTTP_IDLE :
					if ((len = getSn_RX_RSR(s)) > 0)
					{
						if (len > DATA_BUF_SIZE) len = DATA_BUF_SIZE;
						len = recv(s, (uint8_t *)http_request, len);

						*(((uint8_t *)http_request) + len) = '\0';

						parse_http_request(parsed_http_request, (uint8_t *)http_request);


						if(strlen((char*)parsed_http_request->URI) == 1 &&
						   parsed_http_request->URI[0] == '/') {
							strcpy((char*)parsed_http_request->URI, "/index.html");
							printf("[HTTP] Root requested, serving index.html\r\n");
						}


						#ifdef _HTTPSERVER_DEBUG_
							getSn_DIPR(s, destip);
							destport = getSn_DPORT(s);
						printf("\r\n");
						printf("> HTTPSocket[%d] : HTTP Request received ", s);
						printf("from %d.%d.%d.%d : %d\r\n", destip[0], destip[1], destip[2], destip[3], destport);
#endif
#ifdef _HTTPSERVER_DEBUG_
						printf("> HTTPSocket[%d] : [State] STATE_HTTP_REQ_DONE\r\n", s);
#endif
						// HTTP 'response' handler; includes send_http_response_header / body function
						http_process_handler(s, parsed_http_request);

						// === КРИТИЧНО: НЕ ПЕРЕЗАПИСЫВАЕМ STATE_HTTP_UPLOAD! ===
						if(HTTPSock_Status[seqnum].sock_status == STATE_HTTP_UPLOAD) {
							// Потоковая загрузка началась - НЕ ТРОГАЕМ состояние!
#ifdef _HTTPSERVER_DEBUG_
							printf("> HTTPSocket[%d] : [State] Streaming upload started, keeping STATE_HTTP_UPLOAD\r\n", s);
#endif
							break;  // Остаемся в STATE_HTTP_IDLE, но sock_status = STATE_HTTP_UPLOAD
						}

						gettime = get_httpServer_timecount();
						// Check the TX socket buffer for End of HTTP response sends
						while(getSn_TX_FSR(s) != (getSn_TxMAX(s)))
						{
							if((get_httpServer_timecount() - gettime) > 3)
							{
#ifdef _HTTPSERVER_DEBUG_
								printf("> HTTPSocket[%d] : [State] STATE_HTTP_REQ_DONE: TX Buffer clear timeout\r\n", s);
#endif
								break;
							}
						}

						if(HTTPSock_Status[seqnum].file_len > 0) HTTPSock_Status[seqnum].sock_status = STATE_HTTP_RES_INPROC;
						else HTTPSock_Status[seqnum].sock_status = STATE_HTTP_RES_DONE; // Send the 'HTTP response' end
					}
					break;

				case STATE_HTTP_RES_INPROC :
					/* Repeat: Send the remain parts of HTTP responses */
#ifdef _HTTPSERVER_DEBUG_
					printf("> HTTPSocket[%d] : [State] STATE_HTTP_RES_INPROC\r\n", s);
#endif
					// Repeatedly send remaining data to client
					send_http_response_body(s, 0, http_response, 0, 0);

					if(HTTPSock_Status[seqnum].file_len == 0) HTTPSock_Status[seqnum].sock_status = STATE_HTTP_RES_DONE;
					break;

				case STATE_HTTP_UPLOAD:
#ifdef _USE_SDCARD_
				{
					// === ПОТОКОВАЯ ЗАГРУЗКА ФАЙЛА ===

					// Проверяем есть ли данные в сокете
					if ((len = getSn_RX_RSR(s)) > 0)
					{
						// Ограничиваем размер чтения буфером
						if (len > DATA_BUF_SIZE - 1) len = DATA_BUF_SIZE - 1;

						// Читаем данные из сокета
						len = recv(s, (uint8_t *)http_request, len);

						if (len > 0) {
#ifdef _HTTPSERVER_DEBUG_
							printf("> HTTPSocket[%d] : [Upload] Received %d bytes\r\n", s, len);
#endif

							// Вычисляем сколько данных нужно записать в файл
							uint32_t bytes_to_write = len;
							uint32_t total_received_after = HTTPSock_Status[seqnum].upload_bytes_received + len;

							// Проверяем - не превысили ли размер файла
							if (total_received_after >= HTTPSock_Status[seqnum].upload_content_length) {
								// Это последний пакет - нужно обрезать boundary
								uint32_t remaining = HTTPSock_Status[seqnum].upload_content_length -
													HTTPSock_Status[seqnum].upload_bytes_received;

								if (remaining < len) {
									bytes_to_write = remaining;
#ifdef _HTTPSERVER_DEBUG_
									printf("> HTTPSocket[%d] : [Upload] Last packet, writing only %lu bytes\r\n",
										   s, bytes_to_write);
#endif
								}
							}

							// Записываем данные в файл
							if (bytes_to_write > 0) {
								UINT bytes_written = 0;
								FRESULT res = f_write(&HTTPSock_Status[seqnum].upload_file,
													 http_request,
													 bytes_to_write,
													 &bytes_written);

								if (res != FR_OK) {
#ifdef _HTTPSERVER_DEBUG_
									printf("> HTTPSocket[%d] : [Upload] ERROR: f_write failed, code %d\r\n", s, res);
#endif
									// Закрываем файл при ошибке
									f_close(&HTTPSock_Status[seqnum].upload_file);
									HTTPSock_Status[seqnum].upload_active = 0;

									// Отправляем ошибку
									const char* error_response = "HTTP/1.1 500 Internal Server Error\r\n"
																 "Content-Type: text/plain\r\n"
																 "Content-Length: 12\r\n"
																 "\r\n"
																 "WRITE_FAILED";
									send(s, (uint8_t*)error_response, strlen(error_response));

									HTTPSock_Status[seqnum].sock_status = STATE_HTTP_RES_DONE;
									break;
								}

								HTTPSock_Status[seqnum].upload_bytes_written += bytes_written;
								HTTPSock_Status[seqnum].upload_bytes_received += bytes_to_write;

#ifdef _HTTPSERVER_DEBUG_
								printf("> HTTPSocket[%d] : [Upload] Written %u bytes (total: %lu / %lu)\r\n",
									   s, bytes_written,
									   HTTPSock_Status[seqnum].upload_bytes_received,
									   HTTPSock_Status[seqnum].upload_content_length);
#endif
							}

							// Проверяем - все ли данные получены
							if (HTTPSock_Status[seqnum].upload_bytes_received >=
								HTTPSock_Status[seqnum].upload_content_length)
							{
								// Закрываем файл
								f_close(&HTTPSock_Status[seqnum].upload_file);
								HTTPSock_Status[seqnum].upload_active = 0;

#ifdef _HTTPSERVER_DEBUG_
								printf("> HTTPSocket[%d] : [Upload] COMPLETE! Total written: %lu bytes\r\n",
									   s, HTTPSock_Status[seqnum].upload_bytes_written);
#endif

								// Отправляем успешный ответ
								const char* success_response = "HTTP/1.1 200 OK\r\n"
															  "Content-Type: text/plain\r\n"
															  "Content-Length: 2\r\n"
															  "\r\n"
															  "OK";
								send(s, (uint8_t*)success_response, strlen(success_response));

								// === КРИТИЧНО: Дождаться отправки данных перед закрытием ===
								gettime = get_httpServer_timecount();
								while(getSn_TX_FSR(s) != getSn_TxMAX(s))
								{
									if((get_httpServer_timecount() - gettime) > 3)
									{
#ifdef _HTTPSERVER_DEBUG_
										printf("> HTTPSocket[%d] : [Upload] TX Buffer clear timeout\r\n", s);
#endif
										break;
									}
								}

								// Переходим в состояние завершения
								HTTPSock_Status[seqnum].sock_status = STATE_HTTP_RES_DONE;
							}
						}
					}
				}
#endif
				break;

				case STATE_HTTP_RES_DONE :
#ifdef _HTTPSERVER_DEBUG_
					printf("> HTTPSocket[%d] : [State] STATE_HTTP_RES_DONE\r\n", s);
#endif
					// Socket file info structure re-initialize
					HTTPSock_Status[seqnum].file_len = 0;
					HTTPSock_Status[seqnum].file_offset = 0;
					HTTPSock_Status[seqnum].file_start = 0;
					HTTPSock_Status[seqnum].sock_status = STATE_HTTP_IDLE;

					// === ДОБАВЬ ЭТО: Очистка состояния загрузки ===
#ifdef _USE_SDCARD_
					HTTPSock_Status[seqnum].upload_active = 0;
					HTTPSock_Status[seqnum].upload_bytes_received = 0;
					HTTPSock_Status[seqnum].upload_bytes_written = 0;
					HTTPSock_Status[seqnum].upload_content_length = 0;
#endif

#ifdef _USE_WATCHDOG_
					HTTPServer_WDT_Reset();
#endif
					http_disconnect(s);
					break;

				default :
					break;
			}
			break;

		case SOCK_CLOSE_WAIT:
#ifdef _HTTPSERVER_DEBUG_
		printf("> HTTPSocket[%d] : ClOSE_WAIT\r\n", s);	// if a peer requests to close the current connection
#endif
			disconnect(s);
			break;

		case SOCK_CLOSED:
#ifdef _HTTPSERVER_DEBUG_
			printf("> HTTPSocket[%d] : CLOSED\r\n", s);
#endif
			if(socket(s, Sn_MR_TCP, HTTP_SERVER_PORT, 0x00) == s)    /* Reinitialize the socket */
			{
#ifdef _HTTPSERVER_DEBUG_
				printf("> HTTPSocket[%d] : OPEN\r\n", s);
#endif
			}
			break;

		case SOCK_INIT:
			listen(s);
			break;

		case SOCK_LISTEN:
			break;

		default :
			break;

	} // end of switch

#ifdef _USE_WATCHDOG_
	HTTPServer_WDT_Reset();
#endif
}

////////////////////////////////////////////
// Private Functions
////////////////////////////////////////////
static void send_http_response_header(uint8_t s, uint8_t content_type, uint32_t body_len, uint16_t http_status)
{
	uint8_t head_buf[300] = {0,};
	uint16_t len;

	switch(http_status)
	{
		case STATUS_OK:
			// If GZIP file - add Content-Encoding header
			if(current_file_is_gzip) {
				char * mime_type = "";

				// Determine MIME type
				if(content_type == PTYPE_JS) mime_type = "application/javascript";
				else if(content_type == PTYPE_CSS) mime_type = "text/css";
				else if(content_type == PTYPE_HTML) mime_type = "text/html";
				else if(content_type == PTYPE_JSON) mime_type = "application/json";
				else mime_type = "application/octet-stream";

				// Format GZIP header
				sprintf((char*)head_buf,
					"HTTP/1.1 200 OK\r\n"
					"Content-Type: %s\r\n"
					"Content-Encoding: gzip\r\n"
					"Content-Length: %ld\r\n"
					"\r\n",
					mime_type, body_len);

				printf("[HTTP] Sending GZIP response header (type: %s, len: %ld)\r\n", mime_type, body_len);
			}
			else {
				// Normal header without gzip
				make_http_response_head((char*)head_buf, content_type, body_len);
			}
			break;

		case STATUS_NOT_FOUND:
			memcpy(head_buf, ERROR_HTML_PAGE, sizeof(ERROR_HTML_PAGE));
			break;

		default:
			break;
	}

	len = strlen((char*)head_buf);
	send(s, head_buf, len);

#ifdef _HTTPSERVER_DEBUG_
	printf("> HTTPSocket[%d] : Send response header (%d bytes)\r\n", s, len);
#endif
}

static void send_http_response_body(uint8_t s, uint8_t * uri_name, uint8_t * buf, uint32_t start_addr, uint32_t file_len)
{
	int8_t get_seqnum;
	uint32_t send_len;

	uint8_t flag_datasend_end = 0;

#ifdef _USE_SDCARD_
	uint16_t blocklen;
#endif
#ifdef _USE_FLASH_
	uint32_t addr = 0;
#endif

	if((get_seqnum = getHTTPSequenceNum(s)) == -1) return; // exception handling; invalid number

	// Send the HTTP Response 'body'; requested file
	if(!HTTPSock_Status[get_seqnum].file_len) // ### Send HTTP response body: First part ###
	{
		if (file_len > DATA_BUF_SIZE - 1)
		{
			HTTPSock_Status[get_seqnum].file_start = start_addr;
			HTTPSock_Status[get_seqnum].file_len = file_len;
			send_len = DATA_BUF_SIZE - 1;

/////////////////////////////////////////////////////////////////////////////////////////////////
// ## 20141219 Eric added, for 'File object structure' (fs) allocation reduced (8 -> 1)
			memset(HTTPSock_Status[get_seqnum].file_name, 0x00, MAX_CONTENT_NAME_LEN);
			strcpy((char *)HTTPSock_Status[get_seqnum].file_name, (char *)uri_name);
#ifdef _HTTPSERVER_DEBUG_
			printf("> HTTPSocket[%d] : HTTP Response body - file name [ %s ]\r\n", s, HTTPSock_Status[get_seqnum].file_name);
#endif
/////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _HTTPSERVER_DEBUG_
			printf("> HTTPSocket[%d] : HTTP Response body - file len [ %ld ]byte\r\n", s, file_len);
#endif
		}
		else
		{
			// Send process end
			send_len = file_len;

#ifdef _HTTPSERVER_DEBUG_
			printf("> HTTPSocket[%d] : HTTP Response end - file len [ %ld ]byte\r\n", s, send_len);
#endif
		}
#ifdef _USE_FLASH_
		if(HTTPSock_Status[get_seqnum]->storage_type == DATAFLASH) addr = start_addr;
#endif
	}
	else // remained parts
	{
#ifdef _USE_FLASH_
		if(HTTPSock_Status[get_seqnum]->storage_type == DATAFLASH)
		{
			addr = HTTPSock_Status[get_seqnum].file_start + HTTPSock_Status[get_seqnum].file_offset;
		}
#endif
		send_len = HTTPSock_Status[get_seqnum].file_len - HTTPSock_Status[get_seqnum].file_offset;

		if(send_len > DATA_BUF_SIZE - 1)
		{
			send_len = DATA_BUF_SIZE - 1;
		}
		else
		{
#ifdef _HTTPSERVER_DEBUG_
			printf("> HTTPSocket[%d] : HTTP Response end - file len [ %ld ]byte\r\n", s, HTTPSock_Status[get_seqnum].file_len);
#endif
			// Send process end
			flag_datasend_end = 1;
		}
#ifdef _HTTPSERVER_DEBUG_
			printf("> HTTPSocket[%d] : HTTP Response body - send len [ %ld ]byte\r\n", s, send_len);
#endif
	}

/*****************************************************/
	//HTTPSock_Status[get_seqnum]->storage_type == NONE
	//HTTPSock_Status[get_seqnum]->storage_type == CODEFLASH
	//HTTPSock_Status[get_seqnum]->storage_type == SDCARD
	//HTTPSock_Status[get_seqnum]->storage_type == DATAFLASH
/*****************************************************/

	if(HTTPSock_Status[get_seqnum].storage_type == CODEFLASH)
	{
		if(HTTPSock_Status[get_seqnum].file_len) start_addr = HTTPSock_Status[get_seqnum].file_start;
		read_userReg_webContent(start_addr, &buf[0], HTTPSock_Status[get_seqnum].file_offset, send_len);
	}
#ifdef _USE_SDCARD_
	else if(HTTPSock_Status[get_seqnum].storage_type == SDCARD)
	{
		// Data read from SD Card
		fr = f_read(&fs, &buf[0], send_len, (void *)&blocklen);
		if(fr != FR_OK)
		{
			send_len = 0;
#ifdef _HTTPSERVER_DEBUG_
		printf("> HTTPSocket[%d] : [FatFs] Error code return: %d (File Read) / HTTP Send Failed - %s\r\n", s, fr, HTTPSock_Status[get_seqnum].file_name);
#endif
		}
		else
		{
			*(buf+send_len+1) = 0; // Insert '/0' for indicates the 'End of String' (null terminated)
		}
	}
#endif

#ifdef _USE_FLASH_
	else if(HTTPSock_Status[get_seqnum]->storage_type == DATAFLASH)
	{
		// Data read from external data flash memory
		read_from_flashbuf(addr, &buf[0], send_len);
		*(buf+send_len+1) = 0; // Insert '/0' for indicates the 'End of String' (null terminated)
	}
#endif
	else
	{
		send_len = 0;
	}
	// Requested content send to HTTP client
#ifdef _HTTPSERVER_DEBUG_
	printf("> HTTPSocket[%d] : [Send] HTTP Response body [ %ld ]byte\r\n", s, send_len);
#endif

	if(send_len) send(s, buf, send_len);
	else flag_datasend_end = 1;

	if(flag_datasend_end)
	{
		HTTPSock_Status[get_seqnum].file_start = 0;
		HTTPSock_Status[get_seqnum].file_len = 0;
		HTTPSock_Status[get_seqnum].file_offset = 0;
		flag_datasend_end = 0;
	}
	else
	{
		HTTPSock_Status[get_seqnum].file_offset += send_len;
#ifdef _HTTPSERVER_DEBUG_
		printf("> HTTPSocket[%d] : HTTP Response body - offset [ %ld ]\r\n", s, HTTPSock_Status[get_seqnum].file_offset);
#endif
	}

// ## 20141219 Eric added, for 'File object structure' (fs) allocation reduced (8 -> 1)
#ifdef _USE_SDCARD_
	if(flag_datasend_end) {
			f_close(&fs);
		}
#endif
// ## 20141219 added end
}

static void send_http_response_cgi(uint8_t s, uint8_t * buf, uint8_t * http_body, uint16_t file_len)
{
	uint16_t send_len = 0;

#ifdef _HTTPSERVER_DEBUG_
	printf("> HTTPSocket[%d] : HTTP Response Header + Body - CGI\r\n", s);
#endif
	send_len = sprintf((char *)buf, "%s%d\r\n\r\n%s", RES_CGIHEAD_OK, file_len, http_body);
#ifdef _HTTPSERVER_DEBUG_
	printf("> HTTPSocket[%d] : HTTP Response Header + Body - send len [ %d ]byte\r\n", s, send_len);
#endif

	send(s, buf, send_len);
}


static int8_t http_disconnect(uint8_t sn)
{
	setSn_CR(sn,Sn_CR_DISCON);
	/* wait to process the command... */
	while(getSn_CR(sn));

	return SOCK_OK;
}


static void http_process_handler(uint8_t s, st_http_request * p_http_request)
{
	uint8_t * uri_name;
	uint32_t content_addr = 0;
	uint16_t content_num = 0;
	uint32_t file_len = 0;

	uint8_t uri_buf[MAX_URI_SIZE]={0x00, };

	uint16_t http_status;
	int8_t get_seqnum;
	uint8_t content_found;

	if((get_seqnum = getHTTPSequenceNum(s)) == -1) return; // exception handling; invalid number

	http_status = 0;
	http_response = pHTTP_RX;
	file_len = 0;

	//method Analyze
	switch (p_http_request->METHOD)
	{
		case METHOD_ERR :
			http_status = STATUS_BAD_REQ;
			send_http_response_header(s, 0, 0, http_status);
			break;

		case METHOD_HEAD :
		case METHOD_GET :
			get_http_uri_name(p_http_request->URI, uri_buf);
			uri_name = uri_buf;

			// Handle root path "/"
			if (!strcmp((char *)uri_name, "/")) {
				strcpy((char *)uri_name, INITIAL_WEBPAGE);
			}
			// Handle directory paths ending with "/"
			else if (strlen((char *)uri_name) > 0 && uri_name[strlen((char *)uri_name) - 1] == '/') {
				strcat((char *)uri_name, "index.html");
			}
			// Legacy mobile paths
			else if (!strcmp((char *)uri_name, "m")) {
				strcpy((char *)uri_name, M_INITIAL_WEBPAGE);
			}
			else if (!strcmp((char *)uri_name, "mobile")) {
				strcpy((char *)uri_name, MOBILE_INITIAL_WEBPAGE);
			}
			find_http_uri_type(&p_http_request->TYPE, uri_name);	// Checking requested file types (HTML, TEXT, GIF, JPEG and Etc. are included)

#ifdef _HTTPSERVER_DEBUG_
			printf("\r\n> HTTPSocket[%d] : HTTP Method GET\r\n", s);
			printf("> HTTPSocket[%d] : Request Type = %d\r\n", s, p_http_request->TYPE);
			printf("> HTTPSocket[%d] : Request URI = %s\r\n", s, uri_name);
#endif

			if(p_http_request->TYPE == PTYPE_CGI)
			{
				content_found = http_get_cgi_handler(uri_name, pHTTP_TX, &file_len);
				if(content_found && (file_len <= (DATA_BUF_SIZE-(strlen(RES_CGIHEAD_OK)+8))))
				{
					send_http_response_cgi(s, http_response, pHTTP_TX, (uint16_t)file_len);
				}
				else
				{
					send_http_response_header(s, PTYPE_CGI, 0, STATUS_NOT_FOUND);
				}
			}
			else
			{
				// Reset GZIP flag
				current_file_is_gzip = 0;

				// Find the User registered index for web content
				if(find_userReg_webContent(uri_buf, &content_num, &file_len))
				{
					content_found = 1;
					content_addr = (uint32_t)content_num;
					HTTPSock_Status[get_seqnum].storage_type = CODEFLASH;
				}
				// Not CGI request, Web content in 'SD card' or 'Data flash' requested
			#ifdef _USE_SDCARD_
				else
				{
					// ========== GZIP SUPPORT: START ==========

					// FatFS requires paths to start with '/'
					// Create FatFS path with leading '/'
					char fatfs_path[MAX_URI_SIZE + 1];
					if (uri_name[0] != '/') {
						fatfs_path[0] = '/';
						strcpy(fatfs_path + 1, (char *)uri_name);
					} else {
						strcpy(fatfs_path, (char *)uri_name);
					}

					// Check if this is JS/CSS/HTML/JSON file
					uint8_t try_gzip = 0;
					if(strstr(fatfs_path, ".js") || strstr(fatfs_path, ".css") ||
					   strstr(fatfs_path, ".html") || strstr(fatfs_path, ".json")) {
						try_gzip = 1;
					}

					// If JS/CSS - try GZIP version first
					if(try_gzip) {
						char gz_filename[MAX_URI_SIZE + 4];  // +4 for ".gz"
						sprintf(gz_filename, "%s.gz", fatfs_path);

						printf("[HTTP] Trying GZIP version: %s\r\n", gz_filename);

						if((fr = f_open(&fs, gz_filename, FA_READ)) == FR_OK)
						{
							// GZIP version found!
							content_found = 1;
							current_file_is_gzip = 1;
							file_len = f_size(&fs);  // Use FatFS API for file size
							content_addr = 0;  // Not needed for FatFS - file handle is used
							HTTPSock_Status[get_seqnum].storage_type = SDCARD;

							printf("[HTTP] Found GZIP file: %s (%ld bytes)\r\n", gz_filename, file_len);
						}
						else {
							// No GZIP version, try normal file
							printf("[HTTP] No GZIP version, trying normal file: %s\r\n", fatfs_path);

							if((fr = f_open(&fs, fatfs_path, FA_READ)) == FR_OK)
							{
								content_found = 1;
								current_file_is_gzip = 0;
								file_len = f_size(&fs);  // Use FatFS API for file size
								content_addr = 0;  // Not needed for FatFS - file handle is used
								HTTPSock_Status[get_seqnum].storage_type = SDCARD;

								printf("[HTTP] Found normal file: %s (%ld bytes)\r\n", fatfs_path, file_len);
							}
						}
					}
					else {
						// For other files (images, fonts) - normal loading
						if((fr = f_open(&fs, fatfs_path, FA_READ)) == FR_OK)
						{
							content_found = 1;
							current_file_is_gzip = 0;
							file_len = f_size(&fs);  // Use FatFS API for file size
							content_addr = 0;  // Not needed for FatFS - file handle is used
							HTTPSock_Status[get_seqnum].storage_type = SDCARD;

							printf("[HTTP] Loaded normal file: %s (%ld bytes)\r\n", fatfs_path, file_len);
						}
					}

					// ========== GZIP SUPPORT: END ==========
				}
			#elif _USE_FLASH_
				else if(/* Read content from Dataflash */)
				{
					content_found = 1;
					HTTPSock_Status[get_seqnum].storage_type = DATAFLASH;
					; // To do
				}
			#endif

				if(!content_found)
				{
			#ifdef _HTTPSERVER_DEBUG_
					printf("> HTTPSocket[%d] : Unknown Page Request\r\n", s);
			#endif
					http_status = STATUS_NOT_FOUND;
					current_file_is_gzip = 0;  // Reset flag on error
				}
				else
				{
			#ifdef _HTTPSERVER_DEBUG_
					printf("> HTTPSocket[%d] : Find Content [%s] ok - Start [%ld] len [%ld]byte (GZIP: %s)\r\n",
						s, uri_name, content_addr, file_len, current_file_is_gzip ? "YES" : "NO");
			#endif
					http_status = STATUS_OK;
				}

				// Send HTTP header
				if(http_status)
				{
			#ifdef _HTTPSERVER_DEBUG_
					printf("> HTTPSocket[%d] : Requested content len = [%ld]byte\r\n", s, file_len);
			#endif
					send_http_response_header(s, p_http_request->TYPE, file_len, http_status);
				}

				// Send HTTP body (content)
				if(http_status == STATUS_OK)
				{
					send_http_response_body(s, uri_name, http_response, content_addr, file_len);
				}
			}
			break;

		case METHOD_POST :
			mid((char *)p_http_request->URI, "/", " HTTP", (char *)uri_buf);
			uri_name = uri_buf;
			find_http_uri_type(&p_http_request->TYPE, uri_name);	// Check file type (HTML, TEXT, GIF, JPEG are included)

#ifdef _HTTPSERVER_DEBUG_
			printf("\r\n> HTTPSocket[%d] : HTTP Method POST\r\n", s);
			printf("> HTTPSocket[%d] : Request URI = %s ", s, uri_name);
			printf("Type = %d\r\n", p_http_request->TYPE);
#endif

			if(p_http_request->TYPE == PTYPE_CGI)	// HTTP POST Method; CGI Process
			{
				content_found = http_post_cgi_handler(s, uri_name, p_http_request, http_response, &file_len);
#ifdef _HTTPSERVER_DEBUG_
				printf("> HTTPSocket[%d] : [CGI: %s] / Response len [ %ld ]byte\r\n", s, content_found?"Content found":"Content not found", file_len);
#endif

				// === КРИТИЧНО: НЕ отправляем ответ для потоковой загрузки! ===
				if(HTTPSock_Status[get_seqnum].sock_status == STATE_HTTP_UPLOAD) {
#ifdef _HTTPSERVER_DEBUG_
					printf("> HTTPSocket[%d] : [POST] Streaming upload - response will be sent after completion\r\n", s);
#endif
					// Ответ отправится в STATE_HTTP_UPLOAD когда загрузка завершится
					break;
				}

				if(content_found && (file_len <= (DATA_BUF_SIZE-(strlen(RES_CGIHEAD_OK)+8))))
				{
					send_http_response_cgi(s, pHTTP_TX, http_response, (uint16_t)file_len);

					// Reset the H/W for apply to the change configuration information
					if(content_found == HTTP_RESET) HTTPServer_ReStart();
				}
				else
				{
					send_http_response_header(s, PTYPE_CGI, 0, STATUS_NOT_FOUND);
				}
			}
			else	// HTTP POST Method; Content not found
			{
				send_http_response_header(s, 0, 0, STATUS_NOT_FOUND);
			}
			break;

		default :
			http_status = STATUS_BAD_REQ;
			send_http_response_header(s, 0, 0, http_status);
			break;
	}
}

void httpServer_time_handler(void)
{
	httpServer_tick_1s++;
}

uint32_t get_httpServer_timecount(void)
{
	return httpServer_tick_1s;
}

void reg_httpServer_webContent(uint8_t * content_name, uint8_t * content)
{
	uint16_t name_len;
	uint32_t content_len;

	if(content_name == NULL || content == NULL)
	{
		return;
	}
	else if(total_content_cnt >= MAX_CONTENT_CALLBACK)
	{
		return;
	}

	name_len = strlen((char *)content_name);
	content_len = strlen((char *)content);

	web_content[total_content_cnt].content_name = malloc(name_len+1);
	strcpy((char *)web_content[total_content_cnt].content_name, (const char *)content_name);
	web_content[total_content_cnt].content_len = content_len;
	web_content[total_content_cnt].content = content;

	total_content_cnt++;
}

uint8_t display_reg_webContent_list(void)
{
	uint16_t i;
	uint8_t ret;

	if(total_content_cnt == 0)
	{
		printf(">> Web content file not found\r\n");
		ret = 0;
	}
	else
	{
		printf("\r\n=== List of Web content in code flash ===\r\n");
		for(i = 0; i < total_content_cnt; i++)
		{
			printf(" [%d] ", i+1);
			printf("%s, ", web_content[i].content_name);
			printf("%ld byte, ", web_content[i].content_len);

			if(web_content[i].content_len < 30) printf("[%s]\r\n", web_content[i].content);
			else printf("[ ... ]\r\n");
		}
		printf("=========================================\r\n\r\n");
		ret = 1;
	}

	return ret;
}

uint8_t find_userReg_webContent(uint8_t * content_name, uint16_t * content_num, uint32_t * file_len)
{
	uint16_t i;
	uint8_t ret = 0; // '0' means 'File Not Found'

	for(i = 0; i < total_content_cnt; i++)
	{
		if(!strcmp((char *)content_name, (char *)web_content[i].content_name))
		{
			*file_len = web_content[i].content_len;
			*content_num = i;
			ret = 1; // If the requested content found, ret set to '1' (Found)
			break;
		}
	}
	return ret;
}


uint16_t read_userReg_webContent(uint16_t content_num, uint8_t * buf, uint32_t offset, uint16_t size)
{
	uint16_t ret = 0;
	uint8_t * ptr;

	if(content_num > total_content_cnt) return 0;

	ptr = web_content[content_num].content;
	if(offset) ptr += offset;

	strncpy((char *)buf, (char *)ptr, size);
	*(buf+size) = 0; // Insert '/0' for indicates the 'End of String' (null terminated)

	ret = strlen((void *)buf);
	return ret;
}
