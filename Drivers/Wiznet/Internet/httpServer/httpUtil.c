/**
 * @file	httpUtil.c
 * @brief	HTTP Server Utilities (Updated for FatFS)
 * @version 2.0
 * @date	2025/01/XX
 */

#include "../../../Wiznet/Internet/httpServer/httpUtil.h"
#include "ff.h"  // FatFS header
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define MAX_FILENAME_LEN 128

// Макрос для отладки (раскомментируйте для включения)
// #define HTTP_DEBUG

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
			path[path_len - 1] = '\0';  // Убрать последний /
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
		if (res == FR_OK) {
			// Читать содержимое директории
			while (1) {
				res = f_readdir(&dir, &fno);
				if (res != FR_OK || fno.fname[0] == 0) break;  // Конец директории или ошибка

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
			// Возвращаем пустые списки при ошибке
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
			printf("[HTTP] buf content: %s\r\n", buf);
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
uint8_t http_post_cgi_handler(uint8_t * uri_name, st_http_request * p_http_request,
                               uint8_t * buf, uint32_t * file_len)
{
	// === API: Создание папки ===
	if (strncmp((char*)uri_name, "api/mkdir.cgi", 13) == 0)
	{
		char folder[128] = "";
		if (get_query_param((char*)buf, "name", folder, sizeof(folder))) {
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
		// Получаем путь из query параметра
		char upload_path[128] = "";
		get_query_param((char*)p_http_request->URI, "path", upload_path, sizeof(upload_path));

		printf("[HTTP] Upload path from URI: %s\r\n", upload_path);

		// Парсим HTTP запрос
		post_request_t request;
		if (!disassemble_post_request(p_http_request, &request)) {
			printf("[HTTP] Failed to parse POST request\r\n");
			return HTTP_FAILED;
		}

		// Проверяем что Content-Length корректный
		if (request.content_length == 0 || request.content_length > 10000000) {
			printf("[HTTP] Invalid Content-Length: %lu\r\n", request.content_length);
			return HTTP_FAILED;
		}

		// Извлекаем имя файла
		char filename[128] = "";
		char *filename_start = strstr((char*)request.content_disposition, "filename=\"");

		if (!filename_start) {
			printf("[HTTP] Filename not found in request\r\n");
			return HTTP_FAILED;
		}

		filename_start += 10; // Пропускаем 'filename="'

		int filename_length = 0;
		while(*filename_start && (*filename_start != '"') &&
		      (*filename_start != '\r') && (*filename_start != '\n') &&
		      filename_length < 126) {
			filename[filename_length++] = *filename_start++;
		}
		filename[filename_length] = '\0';

		// Формируем полный путь
		char full_path[256] = "";
		if (strlen(upload_path) > 0) {
			strcpy(full_path, upload_path);

			// Создаем директорию если нужно
			if (strcmp(full_path, "/") != 0) {
				FRESULT res = f_mkdir(full_path);
				if (res == FR_OK) {
					printf("[HTTP] Directory created: %s\r\n", full_path);
				} else if (res == FR_EXIST) {
					printf("[HTTP] Directory already exists: %s\r\n", full_path);
				} else {
					printf("[HTTP] Failed to create directory: %s (error %d)\r\n", full_path, res);
				}
			}

			// Добавляем имя файла к пути
			if (full_path[strlen(full_path)-1] != '/') {
				strcat(full_path, "/");
			}
			strcat(full_path, filename);
		} else {
			strcpy(full_path, filename);
		}

		printf("[HTTP] Uploading file: %s\r\n", full_path);

		// Вычисляем ПРАВИЛЬНЫЙ размер файла
		uint32_t content_size = 0;

		if (request.content_start && request.content_end) {
			// КРИТИЧНО: правильное вычисление размера
			if (request.content_end > request.content_start) {
				content_size = (uint32_t)(request.content_end - request.content_start);
			} else {
				printf("[HTTP] ERROR: Invalid content boundaries\r\n");
				return HTTP_FAILED;
			}
		} else {
			printf("[HTTP] ERROR: Content boundaries not found\r\n");
			return HTTP_FAILED;
		}

		// Проверка адекватности размера
		if (content_size == 0 || content_size > 5000000) {  // Макс 5 МБ
			printf("[HTTP] ERROR: Invalid file size: %lu bytes\r\n", content_size);
			return HTTP_FAILED;
		}

		printf("[HTTP] File size: %lu bytes\r\n", content_size);

		// Открываем файл для записи
		FIL file;
		FRESULT res = f_open(&file, full_path, FA_CREATE_ALWAYS | FA_WRITE);

		if (res != FR_OK) {
			printf("[HTTP] Failed to open file: %s (error %d)\r\n", full_path, res);
			return HTTP_FAILED;
		}

		// Записываем данные
		UINT bytes_written = 0;
		res = f_write(&file, request.content_start, content_size, &bytes_written);

		if (res != FR_OK) {
			printf("[HTTP] Failed to write file (error %d)\r\n", res);
			f_close(&file);
			return HTTP_FAILED;
		}

		// Закрываем файл
		f_close(&file);

		printf("[HTTP] File uploaded successfully (%lu bytes written)\r\n", bytes_written);

		// Отправляем ответ
		strcpy((char*)buf, "OK");
		*file_len = 2;
		return HTTP_OK;
	}

	// === API: Удаление файла ===
	else if (strncmp((char*)uri_name, "api/delete.cgi", 14) == 0)
	{
		char path[128] = "";
		if (get_query_param((char*)buf, "path", path, sizeof(path))) {

			// Проверяем - это файл или папка
			FILINFO fno;
			FRESULT res = f_stat(path, &fno);

			if (res == FR_OK) {
				if (fno.fattrib & AM_DIR) {
					// Это папка
					res = f_unlink(path);  // f_unlink работает и для папок (если пуста)

					if (res == FR_OK) {
						printf("[HTTP] Directory deleted: %s\r\n", path);
					} else if (res == FR_DENIED) {
						printf("[HTTP] Directory not empty: %s\r\n", path);
						strcpy((char*)buf, "Directory not empty");
						*file_len = strlen((char*)buf);
						return HTTP_FAILED;
					}
				} else {
					// Это файл
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

	// === API: Скачивание файла ===
	else if (strncmp((char*)uri_name, "api/download.cgi", 16) == 0)
	{
		// Для скачивания используется GET метод, здесь только заглушка
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
 * @brief Разбор POST запроса (multipart/form-data)
 */
uint8_t disassemble_post_request(st_http_request * p_http_request, post_request_t* request)
{
	request->content_length_header = NULL;
	request->content_length = 0;
	request->content_disposition = NULL;
	request->content_type = NULL;
	request->content_start = NULL;
	request->content_end = NULL;

	char* uri = (char*)p_http_request->URI;

	// === 1. Content-Length ===
	request->content_length_header = strstr(uri, "Content-Length: ");
	if (!request->content_length_header) {
		printf("[HTTP] ERROR: No Content-Length\r\n");
		return HTTP_FAILED;
	}

	request->content_length_header += strlen("Content-Length: ");
	request->content_length = atoi(request->content_length_header);

	printf("[HTTP] Content-Length: %lu\r\n", request->content_length);

	if (request->content_length == 0 || request->content_length > 10000000) {
		printf("[HTTP] ERROR: Invalid Content-Length: %lu\r\n", request->content_length);
		return HTTP_FAILED;
	}

	// === 2. Content-Disposition ===
	request->content_disposition = strstr(uri, "Content-Disposition: form-data;");
	if (!request->content_disposition) {
		printf("[HTTP] ERROR: No Content-Disposition\r\n");
		return HTTP_FAILED;
	}

	// === 3. Content-Type (опционально) ===
	request->content_type = strstr(request->content_disposition, "Content-Type:");

	// === 4. Начало данных ===
	char* search_start = request->content_disposition;
	if (request->content_type) {
		search_start = request->content_type;
	}

	request->content_start = strstr(search_start, "\r\n\r\n");
	if (!request->content_start) {
		printf("[HTTP] ERROR: Content start not found\r\n");
		return HTTP_FAILED;
	}
	request->content_start += 4;

	// === 5. КОНЕЦ данных - НЕ ИЩЕМ! Вычисляем по размеру ===

	// Находим размер заголовков до начала данных
	uint32_t header_size = (uint32_t)(request->content_start - uri);

	// Размер данных файла = Content-Length минус заголовки минус boundary в конце
	// Boundary обычно ~50 байт (--boundary--\r\n)
	// Для безопасности вычтем 100 байт
	uint32_t boundary_overhead = 100;

	if (request->content_length > boundary_overhead) {
		uint32_t file_data_size = request->content_length - boundary_overhead;

		// Конец данных = начало + размер данных файла
		request->content_end = request->content_start + file_data_size;

		printf("[HTTP] Calculated file size: %lu bytes\r\n", file_data_size);
	} else {
		printf("[HTTP] ERROR: Content too small\r\n");
		return HTTP_FAILED;
	}

	// === 6. Валидация ===
	if (request->content_end <= request->content_start) {
		printf("[HTTP] ERROR: Invalid boundaries\r\n");
		return HTTP_FAILED;
	}

	uint32_t calc_size = (uint32_t)(request->content_end - request->content_start);
	printf("[HTTP] File data size: %lu bytes\r\n", calc_size);

	return HTTP_OK;
}

/**
 * @brief Получение GET параметра из URI
 * @param uri URI строка (например: "list.cgi?path=/web")
 * @param key Имя параметра (например: "path")
 * @param out Буфер для результата
 * @param max_len Максимальный размер буфера
 * @return 1 если параметр найден, 0 если нет
 */
int get_query_param(const char* uri, const char* key, char* out, size_t max_len)
{
	char* pos = strstr(uri, "?");
	if (!pos) return 0;
	pos++; // Пропустить '?'

	if (strncmp(pos, key, strlen(key)) == 0 && pos[strlen(key)] == '=') {
		char* value = pos + strlen(key) + 1;
		pos = value;
		int i = 0;

		// Декодирование URL (например %2F -> /)
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
				break;  // Конец параметра
			} else {
				out[i++] = *value++;
			}
		}
		out[i] = '\0';
		return 1;
	}

	return 0;
}
