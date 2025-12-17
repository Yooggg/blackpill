/**
 * @file	httpUtil.c
 * @brief	HTTP Server Utilities (Fixed boundary detection bug)
 * @version 2.2
 * @date	2025/01/XX
 * @change Make this file fully working with api and not using httpServer
 */

#include "../../../Wiznet/Internet/httpServer/httpUtil.h"
#include "../../../Wiznet/Ethernet/wizchip_conf.h"
#include "ff.h"  // FatFS header
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

#define MAX_FILENAME_LEN 128
extern uint8_t * pHTTP_RX;

/**
 * @brief Обработчик GET запросов (список файлов, создание папок)
 */
uint8_t http_get_cgi_handler(uint8_t * uri_name, uint8_t * buf, uint32_t * file_len)
{
	uint8_t ret = HTTP_OK;

	// Получение списка файлов и папок (list.cgi?path=/web)
	if(strncmp((const char *)uri_name, "list.cgi", 8) == 0)
	{
		char path[128] = "";
		get_query_param((char*)buf, "path", path, sizeof(path));

		// Если путь пустой, используем корень
		if (path[0] == '\0') {
			strcpy(path, "/");
		}

		// ВАЖНО: Убираем слэш в конце для f_opendir (кроме корня)
		size_t path_len = strlen(path);
		if (path_len > 1 && path[path_len - 1] == '/') {
			path[path_len - 1] = '\0';
			path_len--;
		}

		char folders[512] = "";
		char files[512] = "";

		DIR dir;
		FILINFO fno;
		FRESULT res;

		printf("[HTTP] Listing directory: '%s'\r\n", path);

		// Открыть директорию (БЕЗ слэша в конце!)
		res = f_opendir(&dir, path);
		if (res == FR_OK) { // FR_TOO_MANY_OPEN_FILES после открытия /web/ для просмотра vue видимо там файл не закрывается.
			// Читать содержимое директории
			while (1) {
				res = f_readdir(&dir, &fno);
				if (res != FR_OK || fno.fname[0] == 0) break;

				// Пропустить точку и две точки
				if (strcmp(fno.fname, ".") == 0 || strcmp(fno.fname, "..") == 0) {
					continue;
				}

				if (fno.fattrib & AM_DIR) {
					// Это директория
					if (strlen(folders) > 0) {
						strcat(folders, ", \"");
					} else {
						strcat(folders, "\"");
					}
					strcat(folders, fno.fname);
					strcat(folders, "\"");

					printf("[HTTP] Found folder: %s\r\n", fno.fname);
				} else {
					// Это файл
					if (strlen(files) > 0) {
						strcat(files, ", \"");
					} else {
						strcat(files, "\"");
					}
					strcat(files, fno.fname);
					strcat(files, "\"");

					printf("[HTTP] Found file: %s (%lu bytes)\r\n", fno.fname, fno.fsize);
				}
			}
			f_closedir(&dir);
		} else {
			printf("[HTTP] Error opening directory: %d\r\n", res);
		}

		// Формируем JSON ответ
		char json[1060] = "";
		sprintf(json, "{\"folders\": [%s], \"files\": [%s]}", folders, files);

		strcpy((char*)buf, json);
		*file_len = strlen(json);

		printf("[HTTP] Response: %s\r\n", json);
	}

	// Создание директории (api/mkdir.cgi?name=/web)
	else if (strncmp((const char *)uri_name, "api/mkdir.cgi", 13) == 0)
	{
		char folder[128] = "";
		if (get_query_param((char*)buf, "name", folder, sizeof(folder))) {

			// Убрать слэш в конце если есть (кроме корня)
			size_t len = strlen(folder);
			if (len > 1 && folder[len-1] == '/') {
				folder[len-1] = '\0';
			}

			printf("[HTTP] Creating directory: '%s'\r\n", folder);

			FRESULT res = f_mkdir(folder);

			if (res == FR_OK) {
				printf("[HTTP] Directory created successfully\r\n");
				strcpy((char*)buf, "OK");
				*file_len = 2;
				return HTTP_OK;
			} else if (res == FR_EXIST) {
				printf("[HTTP] Directory already exists\r\n");
				strcpy((char*)buf, "EXISTS");
				*file_len = 6;
				return HTTP_OK;
			} else {
				printf("[HTTP] Error creating directory: %d\r\n", res);
				sprintf((char*)buf, "ERROR_%d", res);
				*file_len = strlen((char*)buf);
				return HTTP_FAILED;
			}
		} else {
			printf("[HTTP] Missing 'name' parameter in mkdir request\r\n");
			return HTTP_FAILED;
		}
	}
	// Удаление файла (api/delete.cgi?path=/file.txt)
	else if (strncmp((const char *)uri_name, "api/delete.cgi", 14) == 0)
	{
		char filepath[128] = "";
		if (get_query_param((char*)buf, "path", filepath, sizeof(filepath))) {

			// Убрать слэш в конце если есть (для директорий)
			size_t len = strlen(filepath);
			if (len > 1 && filepath[len-1] == '/') {
				filepath[len-1] = '\0';
			}

			printf("[HTTP] Deleting: %s\r\n", filepath);

			FRESULT res = f_unlink(filepath);

			if (res == FR_OK) {
				printf("[HTTP] Deleted successfully\r\n");
				strcpy((char*)buf, "OK");
				*file_len = 2;
				return HTTP_OK;
			} else {
				printf("[HTTP] Error deleting: %d\r\n", res);
				strcpy((char*)buf, "ERROR");
				*file_len = 5;
				return HTTP_FAILED;
			}
		} else {
			printf("[HTTP] Missing 'path' parameter\r\n");
			return HTTP_FAILED;
		}
	}

	return ret;
}

/**
 * @brief Обработчик POST запросов (загрузка файлов)
 */
uint8_t http_post_cgi_handler(uint8_t s, uint8_t * uri_name, st_http_request * p_http_request,
                               uint8_t * buf, uint32_t * file_len)
{
	// Declare externs OUTSIDE any block
	extern st_http_socket HTTPSock_Status[_WIZCHIP_SOCK_NUM_];
	extern uint8_t HTTPSock_Num[_WIZCHIP_SOCK_NUM_];

	int8_t seq = -1;

	// Find sequence number
	for(int i = 0; i < _WIZCHIP_SOCK_NUM_; i++) {
		if(HTTPSock_Num[i] == s) {
			seq = i;
			break;
		}
	}

	if(seq == -1) {
		printf("[HTTP] ERROR: Invalid socket\r\n");
		return HTTP_FAILED;
	}

	// === API: Создание папки ===
	if (strncmp((char*)uri_name, "api/mkdir.cgi", 13) == 0)
	{
		char folder[128] = "";
		if (get_query_param((char*)buf, "name", folder, sizeof(folder))) {

			// Убрать слэш в конце (кроме корня)
			size_t len = strlen(folder);
			if (len > 1 && folder[len-1] == '/') {
				folder[len-1] = '\0';
			}

			FRESULT res = f_mkdir(folder);

			if (res == FR_OK || res == FR_EXIST) {
				printf("[HTTP] Directory created: %s\r\n", folder);
				strcpy((char*)buf, "OK");
				*file_len = 2;
				return HTTP_OK;
			} else {
				printf("[HTTP] Failed to create directory: %s (error %d)\r\n", folder, res);
				strcpy((char*)buf, "ERROR");
				*file_len = 5;
				return HTTP_FAILED;
			}
		}
		return HTTP_FAILED;
	}

	// === API: Загрузка файла ===
	else if (strncmp((char*)uri_name, "api/upload.cgi", 14) == 0)
	{
		printf("\r\n===========================================\r\n");
		printf("[UPLOAD] Handler called!\r\n");
		printf("[UPLOAD] URI: %s\r\n", uri_name);
		printf("[UPLOAD] Type: %d\r\n", p_http_request->TYPE);
		printf("===========================================\r\n");
		extern st_http_socket HTTPSock_Status[_WIZCHIP_SOCK_NUM_];

		// === Получаем параметры из query string ===
		char upload_path[128] = "";
		char filename[128] = "";

		// Получаем path
		get_query_param((char*)p_http_request->URI, "path", upload_path, sizeof(upload_path));

		// Получаем filename
		get_query_param_raw_file((char*)p_http_request->URI, "filename", filename, sizeof(filename));

		if (strlen(filename) == 0) {
			printf("[HTTP] ERROR: No filename parameter\r\n");
			strcpy((char*)buf, "NO_FILENAME");
			*file_len = strlen((char*)buf);
			return HTTP_FAILED;
		}

		printf("[HTTP] Upload to path: %s\r\n", upload_path);
		printf("[HTTP] Filename: %s\r\n", filename);

		// === Парсим HTTP запрос (octet-stream) ===
		post_request_t request;
		if (!parse_octet_stream_upload(p_http_request, &request)) {
			printf("[HTTP] Failed to parse request\r\n");
			strcpy((char*)buf, "PARSE_ERROR");
			*file_len = strlen((char*)buf);
			return HTTP_FAILED;
		}

		/**
		 * ОБНОВЛЕННЫЙ ОБРАБОТЧИК с правильным созданием вложенных директорий
		 * Замените секцию "=== Формируем полный путь к файлу ==="
		 */

			// === Формируем полный путь к файлу ===
			char full_path[256] = "";
			FRESULT res;
			printf("[HTTP] === Path processing ===\r\n");
			printf("[HTTP] upload_path: [%s]\r\n", upload_path);
			printf("[HTTP] filename: [%s]\r\n", filename);

			if (strlen(upload_path) > 0 && strcmp(upload_path, "/") != 0) {
				// Копируем путь
				strcpy(full_path, upload_path);

				// Убеждаемся что путь начинается с /
				if (full_path[0] != '/') {
					char temp[256];
					sprintf(temp, "/%s", full_path);
					strcpy(full_path, temp);
				}

				// Создаем все вложенные директории
				char mkdir_path[256];
				strcpy(mkdir_path, full_path);

				// Убираем слэш в конце для mkdir
				size_t len = strlen(mkdir_path);
				if (len > 1 && mkdir_path[len-1] == '/') {
					mkdir_path[len-1] = '\0';
				}

				printf("[HTTP] Creating directory path: %s\r\n", mkdir_path);

				// Используем mkpath для создания вложенных директорий
				res = f_mkdir(mkdir_path);
				if (res != FR_OK && res != FR_EXIST) {
					printf("[HTTP] ERROR: Failed to create path (error %d)\r\n", res);
					sprintf((char*)buf, "MKDIR_ERROR_%d", res);
					*file_len = strlen((char*)buf);
					return HTTP_FAILED;
				}

				// Добавляем / в конце если нужно
				len = strlen(full_path);
				if (len > 0 && full_path[len-1] != '/') {
					strcat(full_path, "/");
				}

				// Добавляем имя файла
				strcat(full_path, filename);
			} else {
				// Корневая директория
				sprintf(full_path, "/%s", filename);
			}

			printf("[HTTP] Full path: [%s]\r\n", full_path);
			printf("[HTTP] === Path processing complete ===\r\n");

			// Проверяем что путь корректный
			if (strlen(full_path) == 0 || strlen(full_path) >= 256) {
				printf("[HTTP] ERROR: Invalid path length\r\n");
				strcpy((char*)buf, "INVALID_PATH");
				*file_len = strlen((char*)buf);
				return HTTP_FAILED;
			}

			printf("[HTTP] Total file size: %lu bytes\r\n", request.content_length);

			// === ОТКРЫВАЕМ ФАЙЛ ===
			printf("[HTTP] Opening file: %s\r\n", full_path);
			res = f_open(&HTTPSock_Status[seq].upload_file,
			             full_path,
			             FA_CREATE_ALWAYS | FA_WRITE);

			if (res != FR_OK) {
				printf("[HTTP] ERROR: Failed to open file (error %d)\r\n", res);

				// Подробная диагностика
				FILINFO fno;
				FRESULT check_res;

				// Проверяем директорию
				char dir_path[256];
				strcpy(dir_path, full_path);
				char* last_slash = strrchr(dir_path, '/');
				if (last_slash) {
					*last_slash = '\0';
					if (strlen(dir_path) == 0) {
						strcpy(dir_path, "/");
					}
					check_res = f_stat(dir_path, &fno);
					printf("[HTTP] Directory check (%s): %d\r\n", dir_path, check_res);
					if (check_res == FR_OK) {
						printf("[HTTP] Directory exists: %s\r\n",
						       (fno.fattrib & AM_DIR) ? "YES" : "NO");
					}
				}

				sprintf((char*)buf, "FILE_ERROR_%d", res);
				*file_len = strlen((char*)buf);
				return HTTP_FAILED;
			}

			printf("[HTTP] File opened successfully\r\n");

			// === ВЫЧИСЛЯЕМ ДОСТУПНЫЕ ДАННЫЕ В БУФЕРЕ ===
			uint32_t available = (uint32_t)(request.content_end - request.content_start);
			printf("[HTTP] Available in first packet: %lu bytes\r\n", available);

			// === ОТЛАДКА: ЧТО МЫ БУДЕМ ПИСАТЬ? ===
			printf("\r\n[DEBUG] ====== FIRST PACKET DEBUG ======\r\n");
			printf("[DEBUG] request.content_start address: %p\r\n", request.content_start);
			printf("[DEBUG] request.content_end address: %p\r\n", request.content_end);
			printf("[DEBUG] pHTTP_RX address: %p\r\n", pHTTP_RX);
			printf("[DEBUG] Offset of content_start from pHTTP_RX: %lu\r\n",
			       (uint32_t)(request.content_start - (char*)pHTTP_RX));

			// Показываем ЧТО именно будем писать
			printf("[DEBUG] First 200 bytes that will be written:\r\n");
			for (int i = 0; i < 200 && i < available; i++) {
			    char c = request.content_start[i];
			    if (c >= 32 && c <= 126) printf("%c", c);
			    else if (c == '\r') printf("\\r");
			    else if (c == '\n') printf("\\n");
			    else printf("<%02X>", (unsigned char)c);

			    if ((i + 1) % 80 == 0) printf("\r\n");
			}
			printf("\r\n");

			// Показываем что ПЕРЕД content_start
			printf("[DEBUG] 50 bytes BEFORE content_start:\r\n");
			char* before = request.content_start - 50;
			if (before >= (char*)pHTTP_RX) {  // Проверка границ
			    for (int i = 0; i < 50; i++) {
			        char c = before[i];
			        if (c >= 32 && c <= 126) printf("%c", c);
			        else if (c == '\r') printf("\\r");
			        else if (c == '\n') printf("\\n");
			        else if (c == '\0') printf("<NULL>");
			        else printf("<%02X>", (unsigned char)c);
			    }
			    printf("\r\n");
			}

			printf("[DEBUG] ====== END DEBUG ======\r\n\r\n");

			// === СКОЛЬКО ЗАПИСАТЬ ИЗ ПЕРВОГО ПАКЕТА ===
			uint32_t to_write = available;

			// Не пишем больше чем размер файла
			if (to_write > request.content_length) {
			    to_write = request.content_length;
			    printf("[HTTP] Limiting to content_length: %lu bytes\r\n", to_write);
			}

			// === ЗАПИСЫВАЕМ ПЕРВЫЙ КУСОК ===
			UINT bytes_written = 0;
			if (to_write > 0) {
				printf("\r\n[FIRST PACKET] ============================================\r\n");
				printf("[FIRST PACKET] About to write first chunk\r\n");
				printf("[FIRST PACKET] to_write: %lu bytes\r\n", to_write);
				printf("[FIRST PACKET] available: %lu bytes\r\n", available);

				// Показываем последние 100 байт которые будем писать
				printf("[FIRST PACKET] Last 100 bytes to write:\r\n[");
				uint32_t start = (to_write > 100) ? (to_write - 100) : 0;
				for (uint32_t i = start; i < to_write; i++) {
					char c = request.content_start[i];
					if (c >= 32 && c <= 126) printf("%c", c);
					else if (c == '\r') printf("\\r");
					else if (c == '\n') printf("\\n");
					else printf("<%02X>", (unsigned char)c);
				}
				printf("]\r\n");
				printf("[FIRST PACKET] ============================================\r\n\r\n");

					printf("[FIRST PACKET] content_length: %lu bytes\r\n", request.content_length);
			    res = f_write(&HTTPSock_Status[seq].upload_file,
			                  request.content_start,  // ← Откуда пишем?
			                  to_write,
			                  &bytes_written);

			    if (res != FR_OK) {
					printf("[HTTP] Write failed (error %d)\r\n", res);
					f_close(&HTTPSock_Status[seq].upload_file);
					sprintf((char*)buf, "WRITE_ERROR_%d", res);
					*file_len = strlen((char*)buf);
					return HTTP_FAILED;
				}

			    printf("[HTTP] f_write returned: FR=%d, written=%u (requested %lu)\r\n",
			           res, bytes_written, to_write);
			}

		// === ПРОВЕРЯЕМ: Весь файл получен? ===
		if (bytes_written >= request.content_length) {
			// Весь файл в одном пакете (маленький файл)
			printf("[HTTP] Upload complete! Small file (%u bytes)\r\n", bytes_written);

			res = f_sync(&HTTPSock_Status[seq].upload_file);
			printf("[HTTP] File synced (result: %d)\r\n", res);

			res = f_close(&HTTPSock_Status[seq].upload_file);
			printf("[HTTP] File closed (result: %d)\r\n", res);

			strcpy((char*)buf, "OK");
			*file_len = 2;
			return HTTP_OK;
		}

		// === БОЛЬШОЙ ФАЙЛ - переходим в streaming режим ===
		printf("[HTTP] Large file detected - switching to streaming mode\r\n");
		printf("[HTTP] Progress: %u / %lu bytes (%.1f%%)\r\n",
		       bytes_written, request.content_length,
		       (bytes_written * 100.0) / request.content_length);

		// Сохраняем состояние для продолжения загрузки
		HTTPSock_Status[seq].upload_active = 1;
		HTTPSock_Status[seq].upload_content_length = request.content_length;
		HTTPSock_Status[seq].upload_bytes_received = bytes_written;
		HTTPSock_Status[seq].upload_bytes_written = bytes_written;
		HTTPSock_Status[seq].sock_status = STATE_HTTP_UPLOAD;

		// Файл остается ОТКРЫТЫМ!
		// Следующие пакеты будут обрабатываться в STATE_HTTP_UPLOAD

		*file_len = 0;
		return HTTP_OK;
	}

	// === API: Удаление файла ===
	else if (strncmp((char*)uri_name, "api/delete.cgi", 14) == 0)
	{
		char path[128] = "";
		if (get_query_param((char*)buf, "path", path, sizeof(path))) {

			// Убрать слэш (кроме корня)
			size_t len = strlen(path);
			if (len > 1 && path[len-1] == '/') {
				path[len-1] = '\0';
			}

			// Проверяем - это файл или папка
			FILINFO fno;
			FRESULT res = f_stat(path, &fno);

			if (res == FR_OK) {
				if (fno.fattrib & AM_DIR) {
					res = f_unlink(path);

					if (res == FR_OK) {
						printf("[HTTP] Directory deleted: %s\r\n", path);
					} else if (res == FR_DENIED) {
						printf("[HTTP] Directory not empty: %s\r\n", path);
						strcpy((char*)buf, "Directory not empty");
						*file_len = strlen((char*)buf);
						return HTTP_FAILED;
					}
				} else {
					res = f_unlink(path);
					if (res == FR_OK) {
						printf("[HTTP] File deleted: %s\r\n", path);
					}
				}

				if (res == FR_OK) {
					strcpy((char*)buf, "OK");
					*file_len = 2;
					return HTTP_OK;
				}
			}

			printf("[HTTP] Failed to delete: %s (error %d)\r\n", path, res);
			strcpy((char*)buf, "ERROR");
			*file_len = 5;
			return HTTP_FAILED;
		}
		return HTTP_FAILED;
	}

	return HTTP_FAILED;
}

/**
 * @brief Заглушки для совместимости
 */
uint8_t predefined_get_cgi_processor(uint8_t * uri_name, uint8_t * buf, uint16_t * len)
{
	return 0;
}

uint8_t predefined_set_cgi_processor(uint8_t * uri_name, uint8_t * uri, uint8_t * buf, uint16_t * len)
{
	return 0;
}

/**
 * АЛЬТЕРНАТИВНЫЙ ПОДХОД
 * Если проблема в p_http_request->URI, используем pHTTP_RX напрямую
 *
 * Замените В НАЧАЛЕ функции disassemble_post_request:
 */

/**
 * ИСПРАВЛЕННАЯ ВЕРСИЯ #2
 * Используем p_http_request->URI (не pHTTP_RX)
 *
 * Замените функцию disassemble_post_request ПОЛНОСТЬЮ
 */

uint8_t disassemble_post_request(st_http_request * p_http_request, post_request_t* request)
{
	request->content_length_header = NULL;
	request->content_length = 0;
	request->content_disposition = NULL;
	request->content_type = NULL;
	request->content_start = NULL;
	request->content_end = NULL;

	// Используем URI из структуры запроса
	char* buffer = (char*)p_http_request->URI;

	printf("[HTTP] === Parsing POST request ===\r\n");

	// === 1. Content-Length ===
	request->content_length_header = strstr(buffer, "Content-Length: ");
	if (!request->content_length_header) {
		printf("[HTTP] ERROR: No Content-Length\r\n");
		return HTTP_FAILED;
	}

	request->content_length_header += strlen("Content-Length: ");
	request->content_length = atoi(request->content_length_header);

	printf("[HTTP] Content-Length: %lu\r\n", request->content_length);

	if (request->content_length == 0 || request->content_length > 50000000) {
		printf("[HTTP] ERROR: Invalid Content-Length: %lu\r\n", request->content_length);
		return HTTP_FAILED;
	}

	// === 2. Ищем boundary (из Content-Type заголовка) ===
	char* main_content_type = strstr(buffer, "Content-Type: ");
	if (!main_content_type) {
		printf("[HTTP] ERROR: Not multipart/form-data\r\n");
		return HTTP_FAILED;
	}

//	// Извлекаем boundary
//	char* boundary_start = strstr(main_content_type, "boundary=");
//	if (!boundary_start) {
//		printf("[HTTP] ERROR: No boundary in Content-Type\r\n");
//		return HTTP_FAILED;
//	}
//	boundary_start += strlen("boundary=");
//
//	// Копируем boundary (до \r или \n или пробела)
//	char boundary[128] = "";
//	int i = 0;
//	while (boundary_start[i] && boundary_start[i] != '\r' &&
//	       boundary_start[i] != '\n' && boundary_start[i] != ' ' && i < 127) {
//		boundary[i] = boundary_start[i];
//		i++;
//	}
//	boundary[i] = '\0';
//	printf("[HTTP] Boundary: %s\r\n", boundary);
//
//	// === 3. Ищем первый boundary в теле ===
//	// Сначала находим конец HTTP заголовков
//	char* http_body = strstr(buffer, "\r\n\r\n");
//	if (!http_body) {
//		printf("[HTTP] ERROR: HTTP body not found\r\n");
//		return HTTP_FAILED;
//	}
//	http_body += 4; // Пропускаем первый \r\n\r\n
//
//	// Теперь ищем boundary в теле
//	char boundary_marker[130];
//	sprintf(boundary_marker, "--%s", boundary);  // Добавляем "--" перед boundary
//
//	char* multipart_start = strstr(http_body, boundary_marker);
//	if (!multipart_start) {
//		printf("[HTTP] ERROR: Boundary not found in body\r\n");
//		return HTTP_FAILED;
//	}
//
//	printf("[HTTP] First boundary found\r\n");
//
//	// === 4. Content-Disposition (в multipart части) ===
//	request->content_disposition = strstr(multipart_start, "Content-Disposition:");
//	if (!request->content_disposition) {
//		printf("[HTTP] ERROR: No Content-Disposition\r\n");
//		return HTTP_FAILED;
//	}
//	printf("[HTTP] Content-Disposition found\r\n");
//
//	// === 5. Content-Type (optional, в multipart части) ===
//	// Ищем Content-Type но ТОЛЬКО до следующего \r\n\r\n
//	char* next_separator = strstr(request->content_disposition, "\r\n\r\n");
//	char* temp_content_type = strstr(request->content_disposition, "Content-Type:");
//
//	if (temp_content_type && temp_content_type < next_separator) {
//		request->content_type = temp_content_type;
//		printf("[HTTP] Content-Type found in multipart\r\n");
//	} else {
//		request->content_type = NULL;
//		printf("[HTTP] No Content-Type in multipart (optional)\r\n");
//	}
//
//	// === 6. Начало ДАННЫХ файла ===
//	// Ищем \r\n\r\n ПОСЛЕ Content-Disposition (или Content-Type если есть)
//	char* search_from = request->content_disposition;
//	if (request->content_type) {
//		search_from = request->content_type;
//	}
//
//	request->content_start = strstr(search_from, "\r\n\r\n");
//	if (!request->content_start) {
//		printf("[HTTP] ERROR: File data start not found\r\n");
//		return HTTP_FAILED;
//	}
//	request->content_start += 4; // Пропускаем \r\n\r\n

	request->content_start = strstr(main_content_type,"\r\n\r\n");
	request->content_start += 4;

	printf("[HTTP] File data start found\r\n");

	// Показываем первые 80 символов данных для проверки
	printf("[HTTP] First 80 chars: [");
	for (int j = 0; j < 80 && request->content_start[j]; j++) {
		char c = request->content_start[j];
		if (c >= 32 && c <= 126) printf("%c", c);
		else if (c == '\r') printf("\\r");
		else if (c == '\n') printf("\\n");
		else printf(".");
	}
	printf("]\r\n");

//	// === 7. КОНЕЦ данных ===
//	// Используем размер буфера приема
//	extern uint8_t * pHTTP_RX;
//	#ifndef DATA_BUF_SIZE
//	#define DATA_BUF_SIZE 2048
//	#endif
//
//	char* buffer_start = (char*)pHTTP_RX;
//	char* buffer_end = buffer_start + DATA_BUF_SIZE;
//
//	// Пытаемся найти конечный boundary
//	char end_boundary_marker[132];
//	sprintf(end_boundary_marker, "\r\n--%s", boundary);
//
//	char* end_boundary = strstr(request->content_start, end_boundary_marker);
//
//	if (end_boundary && end_boundary < buffer_end) {
//		request->content_end = end_boundary;
//		printf("[HTTP] End boundary found in buffer\r\n");
//	} else {
//		request->content_end = buffer_end;
//		printf("[HTTP] Large file - using buffer end\r\n");
//	}
//
//	// Проверка корректности
//	if (request->content_end == NULL || request->content_end <= request->content_start) {
//		printf("[HTTP] ERROR: Invalid content_end\r\n");
//		return HTTP_FAILED;
//	}
//
//	uint32_t available = (uint32_t)(request->content_end - request->content_start);
//	printf("[HTTP] Available data: %lu bytes\r\n", available);


	request->content_end = request->content_start;
	request->content_end += request->content_length;

	return HTTP_OK;
}

/**
 * @brief Получение GET параметра из URI
 */
int get_query_param(const char* uri, const char* key, char* out, size_t max_len)
{
	char* pos = strstr(uri, "?");
	if (!pos) return 0;
	pos++;

	if (strncmp(pos, key, strlen(key)) == 0 && pos[strlen(key)] == '=') {
		char* value = pos + strlen(key) + 1;
		pos = value;
		int i = 0;

		while (*value && i < (max_len - 1)) {
			if ((value[0] == '%') && (value[1] == '2') && (value[2] == 'F')) {
				out[i++] = '/';
				value += 3;
			} else if ((value[0] == '%') && (value[1] == '2') && (value[2] == '0')) {
				out[i++] = ' ';
				value += 3;
			} else if (value[0] == '+') {
				out[i++] = ' ';
				value++;
			} else if (value[0] == '&' || value[0] == ' ') {
				break;
			} else {
				out[i++] = *value++;
			}
		}
		out[i] = '\0';
		return 1;
	}

	return 0;
}

static int hex(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

void url_decode(char *dst, const char *src)
{
    char c;
    while ((c = *src++))
    {
        if (c == '+')
        {
            *dst++ = ' ';
        }
        else if (c == '%' && isxdigit(src[0]) && isxdigit(src[1]))
        {
            int hi = hex(src[0]);
            int lo = hex(src[1]);
            if (hi >= 0 && lo >= 0)
            {
                *dst++ = (char)((hi << 4) | lo);
                src += 2;
            }
        }
        else
        {
            *dst++ = c;
        }
    }
    *dst = '\0';
}

void get_query_param_raw_file(const char *url, const char *key, char *out, int out_size)
{
    char *p = strstr((char*)url, key);
    if (!p) return;

    p += strlen(key);
    if (*p != '=') return;
    p++;

    char *end = strchr(p, ' ');
    int len = end ? (end - p) : strlen(p);

    if (len >= out_size) len = out_size - 1;

    memcpy(out, p, len);
    out[len] = 0;

    // decode %20, %2F, etc
    url_decode(out, out);
}

/**
 * Парсер для application/octet-stream
 * Для вашего случая - файл идет RAW без multipart
 */

uint8_t parse_octet_stream_upload(st_http_request * p_http_request, post_request_t* request)
{
	request->content_length_header = NULL;
	request->content_length = 0;
	request->content_disposition = NULL;
	request->content_type = NULL;
	request->content_start = NULL;
	request->content_end = NULL;

	extern uint8_t * pHTTP_RX;

	// Сохраняем ОРИГИНАЛЬНЫЙ указатель для вычисления границ
	char* buffer_original = (char*)pHTTP_RX;

	// Для поиска заголовков пропускаем испорченную часть
	char* buffer = buffer_original;

	// Пропускаем "POST\0" если нужно
	if (buffer[0] == 'P' && buffer[1] == 'O' && buffer[2] == 'S' &&
	    buffer[3] == 'T' && buffer[4] == '\0') {
		buffer += 5;
		printf("[HTTP] Skipped POST\\0 at buffer start\r\n");
	}

	#ifndef DATA_BUF_SIZE
	#define DATA_BUF_SIZE 2048
	#endif

	char* buffer_end = buffer_original + DATA_BUF_SIZE;

	printf("[HTTP] === Parsing octet-stream upload ===\r\n");

	// === 1. Content-Length ===
	request->content_length_header = strstr(buffer, "Content-Length: ");
	if (!request->content_length_header) {
		printf("[HTTP] ERROR: No Content-Length\r\n");
		return HTTP_FAILED;
	}

	request->content_length_header += strlen("Content-Length: ");
	request->content_length = atoi(request->content_length_header);

	printf("[HTTP] Content-Length: %lu\r\n", request->content_length);

	if (request->content_length == 0 || request->content_length > 50000000) {
		printf("[HTTP] ERROR: Invalid Content-Length: %lu\r\n", request->content_length);
		return HTTP_FAILED;
	}

	// === 2. Проверяем что это octet-stream ===
	char* content_type = strstr(buffer, "Content-Type: application/octet-stream");
	if (!content_type) {
		printf("[HTTP] ERROR: Not application/octet-stream\r\n");
		return HTTP_FAILED;
	}
	printf("[HTTP] Content-Type: application/octet-stream\r\n");

	// === 3. Находим конец HTTP заголовков ===
	char* http_body_start = strstr(buffer, "\r\n\r\n");
	if (!http_body_start) {
		printf("[HTTP] ERROR: HTTP body not found\r\n");
		return HTTP_FAILED;
	}
	http_body_start += 4; // Пропускаем \r\n\r\n

	// === 4. ДАННЫЕ ФАЙЛА ===
	request->content_start = http_body_start;

	// === КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ: strtok портит первый байт данных ===
	// Пропускаем все нулевые байты в начале
	int skipped_nulls = 0;
	while (request->content_start < buffer_end &&
	       request->content_start[0] == '\0') {
		request->content_start++;
		skipped_nulls++;
	}

	if (skipped_nulls > 0) {
		printf("[HTTP] WARNING: Skipped %d null byte(s) at data start (strtok corruption)\r\n",
		       skipped_nulls);
	}

	// Вычисляем offset от ОРИГИНАЛЬНОГО буфера
	uint32_t headers_size = (uint32_t)(http_body_start - buffer_original);
	printf("[HTTP] HTTP headers size: %lu bytes (from buffer_original)\r\n", headers_size);
	printf("[HTTP] File data starts at offset: %lu (after skipping %d nulls)\r\n",
	       (uint32_t)(request->content_start - buffer_original), skipped_nulls);

	// === 5. КОНЕЦ данных - ИСПОЛЬЗУЕМ buffer_original! ===
	request->content_end = buffer_end;

	// Проверка
	if (request->content_end <= request->content_start) {
		printf("[HTTP] ERROR: Invalid content range\r\n");
		return HTTP_FAILED;
	}

	uint32_t available = (uint32_t)(request->content_end - request->content_start);
	printf("[HTTP] Available data in buffer: %lu bytes\r\n", available);

	uint32_t expected_available = DATA_BUF_SIZE - headers_size - skipped_nulls;
	printf("[HTTP] Expected available: %lu bytes\r\n", expected_available);

	// === Показываем первые 100 байт ===
	printf("[HTTP] First 100 bytes of file data:\r\n[");
	int bytes_to_show = (available > 100) ? 100 : available;

	for (int j = 0; j < bytes_to_show; j++) {
		char c = request->content_start[j];
		if (c >= 32 && c <= 126) printf("%c", c);
		else if (c == '\r') printf("\\r");
		else if (c == '\n') printf("\\n");
		else printf(".");
	}
	printf("]\r\n");

	printf("[HTTP] === Parsing complete ===\r\n");

	return HTTP_OK;
}
