/*make this file to work with server for vue
not for the file manager api*/

#include "../../../Wiznet/Internet/httpServer/httpServer.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../../../Wiznet/Ethernet/socket.h"
#include "../../../Wiznet/Ethernet/wizchip_conf.h"
#include "../../../Wiznet/Internet/httpServer/httpParser.h"
#include "../../../Wiznet/Internet/httpServer/httpUtil.h"

#ifdef	_USE_SDCARD_
#include "ff.h"
#endif

#ifndef DATA_BUF_SIZE
	#define DATA_BUF_SIZE		2048
#endif

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/
uint8_t HTTPSock_Num[_WIZCHIP_SOCK_NUM_] = {0, };
static st_http_request * http_request;
static st_http_request * parsed_http_request;
static uint8_t * http_response;
static uint8_t current_file_is_gzip = 0;

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
FIL fs;
FRESULT fr;
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
void default_mcu_reset(void) {;}
void default_wdt_reset(void) {;}
void (*HTTPServer_ReStart)(void) = default_mcu_reset;
void (*HTTPServer_WDT_Reset)(void) = default_wdt_reset;

void httpServer_Sockinit(uint8_t cnt, uint8_t * socklist)
{
	uint8_t i;

	for(i = 0; i < cnt; i++)
	{
		HTTPSock_Num[i] = socklist[i];
	}
}

static uint8_t getHTTPSocketNum(uint8_t seqnum)
{
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
	pHTTP_TX = tx_buf;
	pHTTP_RX = rx_buf;

	httpServer_Sockinit(cnt, socklist);
}

void reg_httpServer_cbfunc(void(*mcu_reset)(void), void(*wdt_reset)(void))
{
	if(mcu_reset) HTTPServer_ReStart = mcu_reset;
	if(wdt_reset) HTTPServer_WDT_Reset = wdt_reset;
}

void httpServer_run(uint8_t seqnum)
{
	uint8_t s;
	uint16_t len;
	uint32_t gettime = 0;

#ifdef _HTTPSERVER_DEBUG_
	uint8_t destip[4] = {0, };
	uint16_t destport = 0;
#endif

	http_request = (st_http_request *)pHTTP_RX;
	parsed_http_request = (st_http_request *)pHTTP_TX;

	s = getHTTPSocketNum(seqnum);

	switch(getSn_SR(s))
	{
		case SOCK_ESTABLISHED:
			if(getSn_IR(s) & Sn_IR_CON)
			{
				setSn_IR(s, Sn_IR_CON);
			}

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
						http_process_handler(s, parsed_http_request);

						if(HTTPSock_Status[seqnum].sock_status == STATE_HTTP_UPLOAD) {
#ifdef _HTTPSERVER_DEBUG_
							printf("> HTTPSocket[%d] : [State] Streaming upload started, keeping STATE_HTTP_UPLOAD\r\n", s);
#endif
							break;
						}

						gettime = get_httpServer_timecount();
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
						else HTTPSock_Status[seqnum].sock_status = STATE_HTTP_RES_DONE;
					}
					break;

				case STATE_HTTP_RES_INPROC :
#ifdef _HTTPSERVER_DEBUG_
					printf("> HTTPSocket[%d] : [State] STATE_HTTP_RES_INPROC\r\n", s);
#endif
					if(HTTPSock_Status[seqnum].file_len == 0) {
#ifdef _HTTPSERVER_DEBUG_
						printf("> HTTPSocket[%d] : [State] File transfer complete, moving to RES_DONE\r\n", s);
#endif
						HTTPSock_Status[seqnum].sock_status = STATE_HTTP_RES_DONE;
						break;
					}

					send_http_response_body(s, 0, http_response, 0, 0);

					if(HTTPSock_Status[seqnum].file_len == 0) HTTPSock_Status[seqnum].sock_status = STATE_HTTP_RES_DONE;
					break;

					/**
					 * ДЕТАЛЬНАЯ ОТЛАДКА для STATE_HTTP_UPLOAD
					 * Замените case STATE_HTTP_UPLOAD полностью
					 *
					 * Показывает ЧТО именно приходит и записывается в каждом пакете
					 */

					case STATE_HTTP_UPLOAD:
					{
						if (!HTTPSock_Status[seqnum].upload_active) {
							printf("[HTTP] ERROR: Upload not active for socket %d\r\n", s);
							f_close(&HTTPSock_Status[seqnum].upload_file);
							HTTPSock_Status[seqnum].sock_status = STATE_HTTP_IDLE;
							disconnect(s);
							break;
						}

						// Получаем размер доступных данных
						uint16_t received = getSn_RX_RSR(s);

						if (received > 0) {
							printf("\r\n[PACKET] ============================================\r\n");
							printf("[PACKET] Socket %d: RX buffer has %u bytes\r\n", s, received);
							printf("[PACKET] Current progress: %lu / %lu bytes (%.1f%%)\r\n",
							       HTTPSock_Status[seqnum].upload_bytes_written,
							       HTTPSock_Status[seqnum].upload_content_length,
							       (HTTPSock_Status[seqnum].upload_bytes_written * 100.0) /
							       HTTPSock_Status[seqnum].upload_content_length);

							// Ограничиваем размер чтения буфером
							#ifndef DATA_BUF_SIZE
							#define DATA_BUF_SIZE 2048
							#endif

							if (received > DATA_BUF_SIZE) {
								received = DATA_BUF_SIZE;
								printf("[PACKET] Limiting read to %u bytes\r\n", received);
							}

							// Получаем указатель на буфер
							extern uint8_t * pHTTP_RX;

							// Читаем данные в буфер
							uint16_t read_len = recv(s, pHTTP_RX, received);
							printf("[PACKET] recv() returned %u bytes\r\n", read_len);

							if (read_len > 0) {
								// === ПОКАЗЫВАЕМ ЧТО ПРИШЛО ===
								printf("[PACKET] First 100 bytes received:\r\n[");
								for (int i = 0; i < 100 && i < read_len; i++) {
									char c = pHTTP_RX[i];
									if (c >= 32 && c <= 126) printf("%c", c);
									else if (c == '\r') printf("\\r");
									else if (c == '\n') printf("\\n");
									else printf("<%02X>", (unsigned char)c);
								}
								printf("]\r\n");

								printf("[PACKET] Last 100 bytes received:\r\n[");
								int start = (read_len > 100) ? (read_len - 100) : 0;
								for (int i = start; i < read_len; i++) {
									char c = pHTTP_RX[i];
									if (c >= 32 && c <= 126) printf("%c", c);
									else if (c == '\r') printf("\\r");
									else if (c == '\n') printf("\\n");
									else printf("<%02X>", (unsigned char)c);
								}
								printf("]\r\n");

								// Сколько еще нужно записать?
								uint32_t remaining = HTTPSock_Status[seqnum].upload_content_length -
								                     HTTPSock_Status[seqnum].upload_bytes_written;

								printf("[PACKET] Remaining to write: %lu bytes\r\n", remaining);

								// Не пишем больше чем осталось
								uint32_t to_write = (read_len > remaining) ? remaining : read_len;

								printf("[PACKET] Will write: %lu bytes (have %u, remaining %lu)\r\n",
								       to_write, read_len, remaining);

								if (to_write != read_len) {
									printf("[PACKET] WARNING: Not writing all received data!\r\n");
									printf("[PACKET] Excess data: %u bytes\r\n", read_len - to_write);

									// Показываем что НЕ будет записано
									printf("[PACKET] Excess data (not written):\r\n[");
									for (uint32_t i = to_write; i < read_len && i < (to_write + 200); i++) {
										char c = pHTTP_RX[i];
										if (c >= 32 && c <= 126) printf("%c", c);
										else if (c == '\r') printf("\\r");
										else if (c == '\n') printf("\\n");
										else printf("<%02X>", (unsigned char)c);
									}
									printf("]\r\n");
								}

								// === ПОКАЗЫВАЕМ ЧТО БУДЕМ ПИСАТЬ ===
								if (to_write > 0) {
									printf("[PACKET] Data to be written:\r\n");
									printf("[PACKET] First 100 bytes:\r\n[");
									for (uint32_t i = 0; i < 100 && i < to_write; i++) {
										char c = pHTTP_RX[i];
										if (c >= 32 && c <= 126) printf("%c", c);
										else if (c == '\r') printf("\\r");
										else if (c == '\n') printf("\\n");
										else printf("<%02X>", (unsigned char)c);
									}
									printf("]\r\n");

									printf("[PACKET] Last 50 bytes to write:\r\n[");
									uint32_t start = (to_write > 50) ? (to_write - 50) : 0;
									for (uint32_t i = start; i < to_write; i++) {
										char c = pHTTP_RX[i];
										if (c >= 32 && c <= 126) printf("%c", c);
										else if (c == '\r') printf("\\r");
										else if (c == '\n') printf("\\n");
										else printf("<%02X>", (unsigned char)c);
									}
									printf("]\r\n");
								}

								// Записываем в файл
								UINT written = 0;
								FRESULT res = f_write(&HTTPSock_Status[seqnum].upload_file,
								                      pHTTP_RX,
								                      read_len,
								                      &written);

								printf("[PACKET] f_write(requested=%lu) returned: FR=%d, written=%u\r\n",
								       to_write, res, written);

								if (res != FR_OK) {
									printf("[HTTP] Socket %d: Write error: %d\r\n", s, res);
									f_close(&HTTPSock_Status[seqnum].upload_file);
									HTTPSock_Status[seqnum].upload_active = 0;
									HTTPSock_Status[seqnum].sock_status = STATE_HTTP_IDLE;
									disconnect(s);
									break;
								}

								if (written != to_write) {
									printf("[PACKET] WARNING: Partial write! wrote %u, requested %lu\r\n",
									       written, to_write);
								}

								HTTPSock_Status[seqnum].upload_bytes_written += written;

								printf("[PACKET] New total: %lu / %lu bytes (%.2f%%)\r\n",
								       HTTPSock_Status[seqnum].upload_bytes_written,
								       HTTPSock_Status[seqnum].upload_content_length,
								       (HTTPSock_Status[seqnum].upload_bytes_written * 100.0) /
								       HTTPSock_Status[seqnum].upload_content_length);
								printf("[PACKET] ============================================\r\n\r\n");

								// Проверяем завершенность
								if (HTTPSock_Status[seqnum].upload_bytes_written >=
								    HTTPSock_Status[seqnum].upload_content_length) {

									printf("[HTTP] ========================================\r\n");
									printf("[HTTP] Socket %d: Upload complete!\r\n", s);
									printf("[HTTP] Total written: %lu bytes\r\n",
									       HTTPSock_Status[seqnum].upload_bytes_written);
									printf("[HTTP] Expected size: %lu bytes\r\n",
									       HTTPSock_Status[seqnum].upload_content_length);
									printf("[HTTP] Match: %s\r\n",
									       (HTTPSock_Status[seqnum].upload_bytes_written ==
									        HTTPSock_Status[seqnum].upload_content_length) ? "YES" : "NO");
									printf("[HTTP] ========================================\r\n");

									// Закрываем файл
									res = f_sync(&HTTPSock_Status[seqnum].upload_file);
									printf("[HTTP] Socket %d: File synced (result: %d)\r\n", s, res);

									res = f_close(&HTTPSock_Status[seqnum].upload_file);
									printf("[HTTP] Socket %d: File closed (result: %d)\r\n", s, res);

									// Отправляем ответ клиенту
									char response[] = "HTTP/1.1 200 OK\r\n"
									                  "Content-Type: text/plain\r\n"
									                  "Content-Length: 2\r\n"
									                  "Connection: close\r\n"
									                  "\r\n"
									                  "OK";
									send(s, (uint8_t*)response, strlen(response));

									// Сбрасываем состояние
									HTTPSock_Status[seqnum].upload_active = 0;
									HTTPSock_Status[seqnum].sock_status = STATE_HTTP_RES_DONE;
								}

								// Периодически синхронизируем для надежности
								if ((HTTPSock_Status[seqnum].upload_bytes_written % 10240) == 0) {
									res = f_sync(&HTTPSock_Status[seqnum].upload_file);
								}
							} else {
								printf("[HTTP] Socket %d: recv() returned 0 or error\r\n", s);
							}
						}
						break;
					}

				case STATE_HTTP_RES_DONE :
#ifdef _HTTPSERVER_DEBUG_
					printf("> HTTPSocket[%d] : [State] STATE_HTTP_RES_DONE\r\n", s);
#endif
					HTTPSock_Status[seqnum].file_len = 0;
					HTTPSock_Status[seqnum].file_offset = 0;
					HTTPSock_Status[seqnum].file_start = 0;
					HTTPSock_Status[seqnum].storage_type = NONE;
					HTTPSock_Status[seqnum].sock_status = STATE_HTTP_IDLE;

#ifdef _USE_SDCARD_
					if(HTTPSock_Status[seqnum].upload_active) {
						FRESULT sync_result = f_sync(&HTTPSock_Status[seqnum].upload_file);
						FRESULT close_result = f_close(&HTTPSock_Status[seqnum].upload_file);
						HTTPSock_Status[seqnum].upload_active = 0;
						HTTPSock_Status[seqnum].upload_bytes_received = 0;
						HTTPSock_Status[seqnum].upload_bytes_written = 0;
						HTTPSock_Status[seqnum].upload_content_length = 0;
#ifdef _HTTPSERVER_DEBUG_
						printf("> HTTPSocket[%d] : [RES_DONE] Upload file synced and closed (sync=%d, close=%d)\r\n",
							   s, sync_result, close_result);
#endif
					}
#endif

#ifdef _HTTPSERVER_DEBUG_
					printf("> HTTPSocket[%d] : [RES_DONE] Waiting for TX buffer to clear...\r\n", s);
#endif
					gettime = get_httpServer_timecount();
					while(getSn_TX_FSR(s) != getSn_TxMAX(s))
					{
						if((get_httpServer_timecount() - gettime) > 3)
						{
#ifdef _HTTPSERVER_DEBUG_
							printf("> HTTPSocket[%d] : [RES_DONE] TX clear timeout (FSR=%d, TxMAX=%d)\r\n",
								   s, getSn_TX_FSR(s), getSn_TxMAX(s));
#endif
							break;
						}
					}

#ifdef _HTTPSERVER_DEBUG_
					printf("> HTTPSocket[%d] : [RES_DONE] TX buffer cleared, calling disconnect\r\n", s);
#endif

#ifdef _USE_WATCHDOG_
					HTTPServer_WDT_Reset();
#endif
					http_disconnect(s);

#ifdef _HTTPSERVER_DEBUG_
					printf("> HTTPSocket[%d] : [RES_DONE] Disconnect called, socket status = %d\r\n", s, getSn_SR(s));
#endif
					break;

				default :
					break;
			}
			break;

		case SOCK_CLOSE_WAIT:
#ifdef _HTTPSERVER_DEBUG_
			printf("> HTTPSocket[%d] : CLOSE_WAIT\r\n", s);
#endif
#ifdef _USE_SDCARD_
			if(HTTPSock_Status[seqnum].file_len > 0 &&
			   HTTPSock_Status[seqnum].storage_type == SDCARD) {
				f_sync(&HTTPSock_Status[seqnum].upload_file);
				FRESULT close_result = f_close(&HTTPSock_Status[seqnum].upload_file);
#ifdef _HTTPSERVER_DEBUG_
				if(close_result != FR_OK) {
					printf("> HTTPSocket[%d] : [CLOSE_WAIT] ERROR: f_close failed with code %d!\r\n", s, close_result);
				}
				printf("> HTTPSocket[%d] : [CLOSE_WAIT] File synced and closed at offset %ld (result=%d)\r\n",
					   s, HTTPSock_Status[seqnum].file_offset, close_result);
#endif
				HTTPSock_Status[seqnum].file_len = 0;
				HTTPSock_Status[seqnum].file_offset = 0;
				HTTPSock_Status[seqnum].storage_type = NONE;
			}

			if(HTTPSock_Status[seqnum].upload_active) {
				f_sync(&HTTPSock_Status[seqnum].upload_file);
				FRESULT close_result = f_close(&HTTPSock_Status[seqnum].upload_file);
				HTTPSock_Status[seqnum].upload_active = 0;
				HTTPSock_Status[seqnum].upload_bytes_received = 0;
				HTTPSock_Status[seqnum].upload_bytes_written = 0;
				HTTPSock_Status[seqnum].upload_content_length = 0;
#ifdef _HTTPSERVER_DEBUG_
				printf("> HTTPSocket[%d] : [CLOSE_WAIT] Upload file synced and closed (result=%d)\r\n", s, close_result);
#endif
			}
#endif
			disconnect(s);
			break;

		case SOCK_FIN_WAIT:
#ifdef _HTTPSERVER_DEBUG_
		printf("> HTTPSocket[%d] : FIN_WAIT - forcing close\r\n", s);
#endif
			close(s);
			break;

		case SOCK_CLOSED:
#ifdef _HTTPSERVER_DEBUG_
			printf("> HTTPSocket[%d] : CLOSED\r\n", s);
#endif
			HTTPSock_Status[seqnum].file_len = 0;
			HTTPSock_Status[seqnum].file_offset = 0;
			HTTPSock_Status[seqnum].file_start = 0;
			HTTPSock_Status[seqnum].storage_type = NONE;
			HTTPSock_Status[seqnum].sock_status = STATE_HTTP_IDLE;

#ifdef _USE_SDCARD_
			HTTPSock_Status[seqnum].upload_active = 0;
			HTTPSock_Status[seqnum].upload_bytes_received = 0;
			HTTPSock_Status[seqnum].upload_bytes_written = 0;
			HTTPSock_Status[seqnum].upload_content_length = 0;
#endif

			if(socket(s, Sn_MR_TCP, HTTP_SERVER_PORT, 0x00) == s)
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

	}

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
			if(current_file_is_gzip) {
				char * mime_type = "";

				if(content_type == PTYPE_JS) mime_type = "application/javascript";
				else if(content_type == PTYPE_CSS) mime_type = "text/css";
				else if(content_type == PTYPE_HTML) mime_type = "text/html";
				else if(content_type == PTYPE_JSON) mime_type = "application/json";
				else mime_type = "application/octet-stream";

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

	if((get_seqnum = getHTTPSequenceNum(s)) == -1) return;
	printf("[DEBUG] S%d: ENTER send_body, storage=%d, len=%ld, ofs=%ld\r\n",
	       s, HTTPSock_Status[get_seqnum].storage_type,
	       HTTPSock_Status[get_seqnum].file_len,
	       HTTPSock_Status[get_seqnum].file_offset);

	if(!HTTPSock_Status[get_seqnum].file_len)
	{
		if (file_len > DATA_BUF_SIZE - 1)
		{
			HTTPSock_Status[get_seqnum].file_start = start_addr;
			HTTPSock_Status[get_seqnum].file_len = file_len;
			send_len = DATA_BUF_SIZE - 1;

			memset(HTTPSock_Status[get_seqnum].file_name, 0x00, MAX_CONTENT_NAME_LEN);
			strcpy((char *)HTTPSock_Status[get_seqnum].file_name, (char *)uri_name);
#ifdef _HTTPSERVER_DEBUG_
			printf("> HTTPSocket[%d] : HTTP Response body - file name [ %s ]\r\n", s, HTTPSock_Status[get_seqnum].file_name);
#endif

#ifdef _HTTPSERVER_DEBUG_
			printf("> HTTPSocket[%d] : HTTP Response body - file len [ %ld ]byte\r\n", s, file_len);
#endif
		}
		else
		{
			send_len = file_len;

			HTTPSock_Status[get_seqnum].file_len = 0;
			flag_datasend_end = 1;

#ifdef _HTTPSERVER_DEBUG_
			printf("> HTTPSocket[%d] : HTTP Response end - file len [ %ld ]byte\r\n", s, send_len);
#endif
		}
#ifdef _USE_FLASH_
		if(HTTPSock_Status[get_seqnum]->storage_type == DATAFLASH) addr = start_addr;
#endif
	}
	else
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
			printf("> HTTPSocket[%d] : HTTP Response end - remaining [ %ld ]byte\r\n", s, send_len);
#endif
			flag_datasend_end = 1;
		}
#ifdef _HTTPSERVER_DEBUG_
			printf("> HTTPSocket[%d] : HTTP Response body - send len [ %ld ]byte\r\n", s, send_len);
#endif
	}

	if(HTTPSock_Status[get_seqnum].storage_type == CODEFLASH)
	{
		if(HTTPSock_Status[get_seqnum].file_len) start_addr = HTTPSock_Status[get_seqnum].file_start;
		read_userReg_webContent(start_addr, &buf[0], HTTPSock_Status[get_seqnum].file_offset, send_len);
	}
#ifdef _USE_SDCARD_
	else if(HTTPSock_Status[get_seqnum].storage_type == SDCARD)
	{
		printf("[DEBUG] S%d: READ fs=%p, len=%ld, ofs=%ld\r\n",
		       s, (void*)&HTTPSock_Status[get_seqnum].upload_file, send_len, HTTPSock_Status[get_seqnum].file_offset);
		fr = f_read(&HTTPSock_Status[get_seqnum].upload_file, &buf[0], send_len, (void *)&blocklen);
		printf("[DEBUG] S%d: READ result=%d, got=%u\r\n", s, fr, blocklen);
		if(fr != FR_OK)
		{
			send_len = 0;
			flag_datasend_end = 1;

			FRESULT sync_result = f_sync(&HTTPSock_Status[get_seqnum].upload_file);
			printf("[DEBUG] S%d: CLOSE fs=%p\r\n", s, (void*)&HTTPSock_Status[get_seqnum].upload_file);
			FRESULT close_result = f_close(&HTTPSock_Status[get_seqnum].upload_file);

			printf("[DEBUG] S%d: CLOSE result=%d\r\n", s, close_result);
			HTTPSock_Status[get_seqnum].file_len = 0;
			HTTPSock_Status[get_seqnum].file_offset = 0;
			HTTPSock_Status[get_seqnum].storage_type = NONE;
			printf("[DEBUG] S%d: CLOSE result=%d\r\n", s, close_result);
			printf("[DEBUG] S%d: LOCK released\r\n", s);

#ifdef _HTTPSERVER_DEBUG_
			printf("> HTTPSocket[%d] : [FatFs] Read error: %d, file synced and closed (sync=%d, close=%d)\r\n",
				   s, fr, sync_result, close_result);
#endif
		}
		else if(blocklen == 0)
		{
			send_len = 0;
			flag_datasend_end = 1;

			HTTPSock_Status[get_seqnum].file_len = 0;
			HTTPSock_Status[get_seqnum].file_offset = 0;

			FRESULT sync_result = f_sync(&HTTPSock_Status[get_seqnum].upload_file);
			printf("[DEBUG] S%d: CLOSE fs=%p\r\n", s, (void*)&HTTPSock_Status[get_seqnum].upload_file);
			FRESULT close_result = f_close(&HTTPSock_Status[get_seqnum].upload_file);
			printf("[DEBUG] S%d: CLOSE result=%d\r\n", s, close_result);

			printf("[DEBUG] S%d: LOCK released\r\n", s);
			if(close_result != FR_OK) {
#ifdef _HTTPSERVER_DEBUG_
				printf("> HTTPSocket[%d] : [FatFs] ERROR: f_close failed with code %d at EOF!\r\n", s, close_result);
#endif
			}

			HTTPSock_Status[get_seqnum].storage_type = NONE;
#ifdef _HTTPSERVER_DEBUG_
			printf("> HTTPSocket[%d] : [FatFs] EOF reached, file synced and closed (sync=%d, close=%d)\r\n",
				   s, sync_result, close_result);
#endif
		}
		else if(blocklen < send_len)
		{
			send_len = blocklen;
			flag_datasend_end = 1;

			HTTPSock_Status[get_seqnum].file_len = 0;
			HTTPSock_Status[get_seqnum].file_offset = 0;

			*(buf+send_len) = 0;

			FRESULT sync_result = f_sync(&HTTPSock_Status[get_seqnum].upload_file);
			printf("[DEBUG] S%d: CLOSE fs=%p\r\n", s, (void*)&HTTPSock_Status[get_seqnum].upload_file);
			FRESULT close_result = f_close(&HTTPSock_Status[get_seqnum].upload_file);
			printf("[DEBUG] S%d: CLOSE result=%d\r\n", s, close_result);
			printf("[DEBUG] S%d: LOCK released\r\n", s);
			if(close_result != FR_OK) {
#ifdef _HTTPSERVER_DEBUG_
				printf("> HTTPSocket[%d] : [FatFs] ERROR: f_close failed with code %d at last chunk!\r\n", s, close_result);
#endif
			}

			HTTPSock_Status[get_seqnum].storage_type = NONE;

#ifdef _HTTPSERVER_DEBUG_
			printf("> HTTPSocket[%d] : [FatFs] Last chunk (%d bytes), file synced and closed (sync=%d, close=%d)\r\n",
				   s, send_len, sync_result, close_result);
#endif
		}
		else
		{
			send_len = blocklen;
			    *(buf+send_len) = 0;

			    // Проверяем: это последний chunk?
			    if(flag_datasend_end && blocklen > 0) {
			        // Маленький файл прочитан полностью
			        printf("[DEBUG] S%d: CLOSE small file fs=%p\r\n", s, (void*)&HTTPSock_Status[get_seqnum].upload_file);
			        FRESULT close_result = f_close(&HTTPSock_Status[get_seqnum].upload_file);
			        printf("[DEBUG] S%d: CLOSE result=%d\r\n", s, close_result);
			        HTTPSock_Status[get_seqnum].storage_type = NONE;
			        printf("[DEBUG] S%d: LOCK released\r\n", s);
			    }
		}
	}
#endif

#ifdef _USE_FLASH_
	else if(HTTPSock_Status[get_seqnum]->storage_type == DATAFLASH)
	{
		read_from_flashbuf(addr, &buf[0], send_len);
		*(buf+send_len+1) = 0;
	}
#endif
	else
	{
		send_len = 0;
	}

#ifdef _HTTPSERVER_DEBUG_
	printf("> HTTPSocket[%d] : [Send] HTTP Response body [ %ld ]byte\r\n", s, send_len);
#endif

	if(send_len) send(s, buf, send_len);
	else {
		flag_datasend_end = 1;
#ifdef _HTTPSERVER_DEBUG_
		printf("> HTTPSocket[%d] : [Send] No data, setting end flag\r\n", s);
#endif
	}

	if(flag_datasend_end)
	{
		HTTPSock_Status[get_seqnum].file_start = 0;
		HTTPSock_Status[get_seqnum].file_len = 0;
		HTTPSock_Status[get_seqnum].file_offset = 0;
		flag_datasend_end = 0;

#ifdef _HTTPSERVER_DEBUG_
		printf("> HTTPSocket[%d] : [Response] Transfer complete\r\n", s);
#endif
	}
	else
	{
		HTTPSock_Status[get_seqnum].file_offset += send_len;
#ifdef _HTTPSERVER_DEBUG_
		printf("> HTTPSocket[%d] : HTTP Response body - offset [ %ld ]\r\n", s, HTTPSock_Status[get_seqnum].file_offset);
#endif
	}
	printf("[DEBUG] S%d: EXIT send_body, len=%ld, ofs=%ld\r\n",
	           s, HTTPSock_Status[get_seqnum].file_len,
	           HTTPSock_Status[get_seqnum].file_offset);
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

	if((get_seqnum = getHTTPSequenceNum(s)) == -1) return;

	http_status = 0;
	http_response = pHTTP_RX;
	file_len = 0;

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

			if (!strcmp((char *)uri_name, "/")) {
				strcpy((char *)uri_name, INITIAL_WEBPAGE);
			}
			else if (strlen((char *)uri_name) > 0 && uri_name[strlen((char *)uri_name) - 1] == '/') {
				strcat((char *)uri_name, "index.html");
			}
			else if (!strcmp((char *)uri_name, "m")) {
				strcpy((char *)uri_name, M_INITIAL_WEBPAGE);
			}
			else if (!strcmp((char *)uri_name, "mobile")) {
				strcpy((char *)uri_name, MOBILE_INITIAL_WEBPAGE);
			}
			find_http_uri_type(&p_http_request->TYPE, uri_name);

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
				current_file_is_gzip = 0;

				if(find_userReg_webContent(uri_buf, &content_num, &file_len))
				{
					content_found = 1;
					content_addr = (uint32_t)content_num;
					HTTPSock_Status[get_seqnum].storage_type = CODEFLASH;
				}
			#ifdef _USE_SDCARD_
				else
				{
					char fatfs_path[MAX_URI_SIZE + 1];
					if (uri_name[0] != '/') {
						fatfs_path[0] = '/';
						strcpy(fatfs_path + 1, (char *)uri_name);
					} else {
						strcpy(fatfs_path, (char *)uri_name);
					}

					uint8_t try_gzip = 0;
					if(strstr(fatfs_path, ".js") || strstr(fatfs_path, ".css") ||
					   strstr(fatfs_path, ".html") || strstr(fatfs_path, ".json")) {
						try_gzip = 1;
					}
					printf("[DEBUG] S%d: LOCK captured\r\n", s);


					if(try_gzip) {
						char gz_filename[MAX_URI_SIZE + 4];
						sprintf(gz_filename, "%s.gz", fatfs_path);

						printf("[HTTP] Trying GZIP version: %s\r\n", gz_filename);
						printf("[DEBUG] S%d: OPEN GZIP '%s', fs=%p\r\n", s, gz_filename, (void*)&HTTPSock_Status[get_seqnum].upload_file);
						fr = f_open(&HTTPSock_Status[get_seqnum].upload_file, gz_filename, FA_READ);
						printf("[DEBUG] S%d: OPEN result=%d, size=%ld\r\n", s, fr, (fr==FR_OK)?f_size(&HTTPSock_Status[get_seqnum].upload_file):0);
						if(fr == FR_OK)
						{
							content_found = 1;
							current_file_is_gzip = 1;
							file_len = f_size(&HTTPSock_Status[get_seqnum].upload_file);
							content_addr = 0;
							HTTPSock_Status[get_seqnum].storage_type = SDCARD;

							printf("[HTTP] Found GZIP file: %s (%ld bytes)\r\n", gz_filename, file_len);
						}
						else {
							printf("[HTTP] No GZIP version (error %d), trying normal file: %s\r\n", fr, fatfs_path);
							printf("[DEBUG] S%d: OPEN NORMAL '%s', fs=%p\r\n", s, fatfs_path, (void*)&HTTPSock_Status[get_seqnum].upload_file);
							fr = f_open(&HTTPSock_Status[get_seqnum].upload_file, fatfs_path, FA_READ);
							printf("[DEBUG] S%d: OPEN result=%d, size=%ld\r\n", s, fr, (fr==FR_OK)?f_size(&HTTPSock_Status[get_seqnum].upload_file):0);
							if(fr == FR_OK)
							{
								content_found = 1;
								current_file_is_gzip = 0;
								file_len = f_size(&HTTPSock_Status[get_seqnum].upload_file);
								content_addr = 0;
								HTTPSock_Status[get_seqnum].storage_type = SDCARD;

								printf("[HTTP] Found normal file: %s (%ld bytes)\r\n", fatfs_path, file_len);
							}
							else {
								printf("[HTTP] ERROR: f_open failed for %s (error %d)\r\n", fatfs_path, fr);
							}
						}
					}
					else {
						printf("[DEBUG] S%d: OPEN '%s', fs=%p\r\n", s, fatfs_path, (void*)&HTTPSock_Status[get_seqnum].upload_file);
						fr = f_open(&HTTPSock_Status[get_seqnum].upload_file, fatfs_path, FA_READ);
						printf("[DEBUG] S%d: OPEN result=%d, size=%ld\r\n", s, fr, (fr==FR_OK)?f_size(&HTTPSock_Status[get_seqnum].upload_file):0);
						if(fr == FR_OK)
						{
							content_found = 1;
							current_file_is_gzip = 0;
							file_len = f_size(&HTTPSock_Status[get_seqnum].upload_file);
							content_addr = 0;
							HTTPSock_Status[get_seqnum].storage_type = SDCARD;

							printf("[HTTP] Loaded normal file: %s (%ld bytes)\r\n", fatfs_path, file_len);
						}
						else {
							printf("[HTTP] ERROR: f_open failed for %s (error %d)\r\n", fatfs_path, fr);
						}
					}
				}
			#elif _USE_FLASH_
				else if(0)
				{
					content_found = 1;
					HTTPSock_Status[get_seqnum].storage_type = DATAFLASH;
				}
			#endif

				if(!content_found)
				{
			#ifdef _HTTPSERVER_DEBUG_
					printf("> HTTPSocket[%d] : Unknown Page Request\r\n", s);
			#endif
					http_status = STATUS_NOT_FOUND;
					current_file_is_gzip = 0;
				}
				else
				{
			#ifdef _HTTPSERVER_DEBUG_
					printf("> HTTPSocket[%d] : Find Content [%s] ok - Start [%ld] len [%ld]byte (GZIP: %s)\r\n",
						s, uri_name, content_addr, file_len, current_file_is_gzip ? "YES" : "NO");
			#endif
					http_status = STATUS_OK;
				}

				if(http_status)
				{
			#ifdef _HTTPSERVER_DEBUG_
					printf("> HTTPSocket[%d] : Requested content len = [%ld]byte\r\n", s, file_len);
			#endif
					send_http_response_header(s, p_http_request->TYPE, file_len, http_status);
				}

				if(http_status == STATUS_OK)
				{
					send_http_response_body(s, uri_name, http_response, content_addr, file_len);
				}
			}
			break;

		case METHOD_POST :
			mid((char *)p_http_request->URI, "/", " HTTP", (char *)uri_buf);
			uri_name = uri_buf;
			find_http_uri_type(&p_http_request->TYPE, uri_name);

#ifdef _HTTPSERVER_DEBUG_
			printf("\r\n> HTTPSocket[%d] : HTTP Method POST\r\n", s);
			printf("> HTTPSocket[%d] : Request URI = %s ", s, uri_name);
			printf("Type = %d\r\n", p_http_request->TYPE);
#endif

			if(p_http_request->TYPE == PTYPE_CGI || p_http_request->TYPE == PTYPE_HTML)
			{
				content_found = http_post_cgi_handler(s, uri_name, p_http_request, http_response, &file_len);
#ifdef _HTTPSERVER_DEBUG_
				printf("> HTTPSocket[%d] : [CGI: %s] / Response len [ %ld ]byte\r\n", s, content_found?"Content found":"Content not found", file_len);
#endif

				if(HTTPSock_Status[get_seqnum].sock_status == STATE_HTTP_UPLOAD) {
#ifdef _HTTPSERVER_DEBUG_
					printf("> HTTPSocket[%d] : [POST] Streaming upload - response will be sent after completion\r\n", s);
#endif
					break;
				}

				if(content_found && (file_len <= (DATA_BUF_SIZE-(strlen(RES_CGIHEAD_OK)+8))))
				{
					send_http_response_cgi(s, pHTTP_TX, http_response, (uint16_t)file_len);

					if(content_found == HTTP_RESET) HTTPServer_ReStart();
				}
				else
				{
					send_http_response_header(s, PTYPE_CGI, 0, STATUS_NOT_FOUND);
				}
			}
			else
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
	uint8_t ret = 0;

	for(i = 0; i < total_content_cnt; i++)
	{
		if(!strcmp((char *)content_name, (char *)web_content[i].content_name))
		{
			*file_len = web_content[i].content_len;
			*content_num = i;
			ret = 1;
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
	*(buf+size) = 0;

	ret = strlen((void *)buf);
	return ret;
}
