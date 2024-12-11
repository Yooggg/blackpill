/**
 * @file	httpUtil.c
 * @brief	HTTP Server Utilities	
 * @version 1.0
 * @date	2014/07/15
 * @par Revision
 *			2014/07/15 - 1.0 Release
 * @author	
 * \n\n @par Copyright (C) 1998 - 2014 WIZnet. All rights reserved.
 */

#include "../../../Wiznet/Internet/httpServer/httpUtil.h"
#include "../Middleware/Fat/fx_file.h"
#include "../Middleware/Fat/fatfs.h"
#include "../Middleware/Fat/fs_media.h"
#include "../Middleware/Spi_Flash/spi_flash.h"
#include "task_flash.h"

#define MAX_FILENAME_LEN 128

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern fs_vol_t volume;
extern fs_media_t m;
extern fs_dir_t dir;
extern fs_file_t file;

uint8_t http_get_cgi_handler(uint8_t * uri_name, uint8_t * buf, uint32_t * file_len)
{
	uint8_t ret = HTTP_OK;
	uint16_t len = 0;

	if(predefined_get_cgi_processor(uri_name, buf, &len))
	{
		;
	}
	else if(strcmp((const char *)uri_name, "example.cgi") == 0)
	{
		// To do
		;
	}
	else
	{
		// CGI file not found
		ret = HTTP_FAILED;
	}

	if(ret)	*file_len = len;
	return ret;
}

uint8_t http_post_cgi_handler(uint8_t * uri_name, st_http_request * p_http_request, uint8_t * buf, uint32_t * file_len)
{
	uint8_t ret = HTTP_OK;
	uint16_t len = 0;
	uint8_t val = 0;

	if(predefined_set_cgi_processor(uri_name, p_http_request->URI, buf, &len))
	{
		;
	}
//	uint8_t ret = HTTP_OK;
//	    uint16_t len = 0;
	    char filename[MAX_FILENAME_LEN] = {0}; // Буфер для имени файла

	    if (strcmp((const char *)uri_name, "upload.cgi") == 0)
	    {
	        // Найти заголовок Content-Length
	        char *content_length_header = strstr((char *)p_http_request->URI, "Content-Length: ");
	        uint32_t content_length = 0;

	        if (content_length_header)
	        {
	            content_length_header += strlen("Content-Length: ");
	            content_length = atoi(content_length_header); // Преобразуем строку в число
	        }

	        if (content_length > 0)
	        {
	            // Найти строку "filename="
	            char *content_disposition = strstr((char *)p_http_request->URI, "Content-Disposition: form-data;");
	            if (content_disposition)
	            {
	                char *filename_start = strstr(content_disposition, "filename=\"");
	                if (filename_start)
	                {
	                    filename_start += 10; // Пропускаем "filename=\""
	                    char *filename_end = strchr(filename_start, '\"');
	                    if (filename_end && (filename_end - filename_start) < MAX_FILENAME_LEN)
	                    {
	                        strncpy(filename, filename_start, filename_end - filename_start);
	                        filename[filename_end - filename_start] = '\0'; // Завершаем строку
	                        printf("Uploaded filename: %s\n", filename); // Выводим имя файла
	                    }
	                    else
	                    {
	                        printf("Filename too long or not found.\n");
	                        ret = HTTP_FAILED;
	                    }
	                }
	            }

	            // Найти начало содержимого файла
	            char *content_type = strstr((char *)p_http_request->URI, "Content-Type: text/plain");
	            char *file_data_start = strstr(content_type, "\r\n\r\n");

	            if (file_data_start)
	            {
	                file_data_start += 4; // Пропускаем \r\n\r\n
	                uint32_t file_data_len = content_length - (file_data_start - (char *)buf);
	                char res_filename[30] = "";
	                strcat(res_filename, "/");
	                int i = 0;
	                int j = 1;
	                while(dir.dirnode.dirent.name[i] != ' '){
	                	res_filename[j] = dir.dirnode.dirent.name[i];
	                	i++;
	                	j++;
	                }
	                char *file_data_end = strstr(file_data_start, "\r\n--");
	                size_t file_length = file_data_end - file_data_start;
	                strcat(res_filename, "/");
	                strcat(res_filename, filename);
//	                snprintf(res_filename, sizeof(res_filename), "%s%s%s", "/", dir.dirnode.dirent.name, filename);
//	                char *res_filename = "/dir1/license.txt";
	                fs_file_create(&volume, res_filename);
	                //fs_file_t file;
	                fs_file_open(&volume, &file, res_filename, 0);
	                size_t size = 0;
	                char *file_content = (char *)malloc(file_length);
	                char *file_content_check = (char *)malloc(file_length);
//	                memset(file_content, 0, file_length);
//	                memset(file_content_check, 0, file_length);
	                memcpy(file_content, file_data_start, file_length);
	                fs_file_write(&file, file_content, file_length, &size);
	                fs_file_open(&volume, &file, res_filename, 0);
	                fs_file_read(&file, file_content_check, file_length, &size);
	                free(file_content);
	                free(file_content_check);

	                if (1)
	                {
	                    len = sprintf((char *)buf, "File '%s' uploaded successfully, size: %lu bytes", filename, file_data_len);
	                }
	                else
	                {
	                    len = sprintf((char *)buf, "Failed to save file '%s'.", filename);
	                    ret = HTTP_FAILED;
	                }
	            }
	            else
	            {
	                len = sprintf((char *)buf, "Invalid file format.");
	                ret = HTTP_FAILED;
	            }
	        }
	        else
	        {
	            len = sprintf((char *)buf, "No file content provided.");
	            ret = HTTP_FAILED;
	        }
	    }
	    else
	    {
	        ret = HTTP_FAILED;
	    }

	    if (ret) *file_len = len;
	    return ret;
}

uint8_t predefined_get_cgi_processor(uint8_t * uri_name, uint8_t * buf, uint16_t * len)
{
	return 0;
}

uint8_t predefined_set_cgi_processor(uint8_t * uri_name, uint8_t * uri, uint8_t * buf, uint16_t * en)
{
	return 0;
}
