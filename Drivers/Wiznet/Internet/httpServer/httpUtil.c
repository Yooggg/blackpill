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
#include <stdbool.h>

//extern fs_vol_t volume;
//extern fs_media_t m;
//extern fs_dir_t dir;
//extern fs_file_t file;

uint8_t http_get_cgi_handler(uint8_t * uri_name, uint8_t * buf, uint32_t * file_len)
{
	uint8_t ret = HTTP_OK;

	    if(strncmp((const char *)uri_name, "list.cgi", 8) == 0)
	    {
	    	char path[128] = "";
	    	get_query_param((char*)buf, "path", path, sizeof(path));
	    	size_t path_len = strlen(path);
			if ((path_len > 0 && path[path_len - 1] != '/') || (path[0] =='\0')) {
				strcat(path, "/");
				path_len++;
			}

			char folders[512] = "";
			char files[512] = "";
			char check[512] = "";

			if (path_len == 1){
				for (int i = 0; i < fs_item_count; i++){
					const char* name = fs_items[i].name;

					const char* rest = name + strlen(path);
					const char* slash = strchr(rest, '/');
					strncpy(check, "", 512);
					strncat(check, name, slash - name);
					if(fs_items[i].type == FS_TYPE_FOLDER){
						if (strncmp(name, path, strlen(path)) != 0) continue;

						if(!strstr(folders, check)){
							if (strlen(folders) > 0) {strcat(folders, ", "); strcat(folders,"\"");}
							else strcat(folders,"\"");
							strncat(folders, name, slash - name);
							strcat(folders,"\"");
						}
					} else {
						if(!strstr(files, check)){
							if (strlen(files) > 0) {strcat(files, ", "); strcat(files,"\"");}
							else strcat(files,"\"");
							strcat(files, name);
							strcat(files,"\"");
						}
					}

				}
			} else {
				for (int i = 0; i < fs_item_count; i++) {
					const char* name = fs_items[i].name;

					if (strncmp(name, path, strlen(path)) != 0) continue;

					const char* rest = name + strlen(path);
					const char* slash = strchr(rest, '/');

					if (slash) {
						// это папка на уровень глубже
						char subdir[64] = "";
						strcat(subdir,"/");
						strncat(subdir, rest, slash - rest);
						strcat(subdir, "\0");

						// проверим, не добавлена ли уже
						if (!strstr(folders, subdir)) {
							if (strlen(folders) > 0) {strcat(folders, ", "); strcat(folders,"\"");}
							else strcat(folders, "\"");
							strcat(folders, subdir);
							strcat(folders,"\"");
						}
					} else {
						// это файл
						if (fs_items[i].type == FS_TYPE_FILE) {
							if(!strstr(files, name)){
								if (strlen(files) > 0) {strcat(files, ", "); strcat(files,"\"");}
								else strcat(files, "\"");
								strcat(files, rest);
								strcat(files,"\"");
							}
						}
						else{

							char subdir[64] = "";
							strcat(subdir,"/");
							strncat(subdir, rest, slash - rest);
							strcat(subdir, "\0");
							//strncpy(subdir, rest, slash - rest);
							//subdir[slash - rest] = '\0';

							if (!strstr(folders, subdir)) {
								if (strlen(folders) > 0) {strcat(folders, ", "); strcat(folders,"\"");}
								else strcat(folders, "\"");
								strcat(folders, subdir);
								strcat(folders,"\"");
							}
						}
					}
				}
			}
			char json[1060] = "";
			//generate_fs_json(json, sizeof(json));
			sprintf(json, "{\"folders\": [%s], \"files\": [%s]}", folders, files);
			//const char* json = "{\"folders\": [\"test\"], \"files\": [\"file.txt\"]}";
			strcpy((char*)buf, json);
			*file_len = strlen(json);
	    }
	    else if (strncmp((const char*)uri_name, "api/mkdir.cgi", 13) == 0)
	    {
	        char folder[64] = "";
	        if (get_query_param((char*)buf, "name", folder, sizeof(folder))) {
	        	fs_dir_create(&volume, folder);
	        	fs_dir_open(&volume, folder, &dir);
	        	add_fs_item(folder, FS_TYPE_FOLDER);
	        	fs_dir_close(&dir);
	        	strcpy((char*)buf, "OK");
	            *file_len = 2;
	            return HTTP_OK;
	        } else {
	            return HTTP_FAILED;
	        }
	    }
	    return ret;
}

uint8_t http_post_cgi_handler(uint8_t * uri_name, st_http_request * p_http_request, uint8_t * buf, uint32_t * file_len)
{
	post_request_t request;
	if (strncmp((char*)uri_name, "api/upload.cgi", 14) == 0) {
			char filename[64] = "";
	        char *filename_start = "";
	        int filename_length = 0;
	        char* content = "";
	        if (disassemble_post_request(p_http_request, &request)) {
	        	filename_start = strstr((char*) request.content_disposition, "filename=\"");
	        	filename_start += 10;
	        	while(*filename_start && (*filename_start != '"') && (*filename_start != '\r') && (*filename_start != '\n')){
					filename[filename_length++] = *filename_start++;
	        	}
	        	fs_file_create(&volume, filename);
	        	fs_file_open(&volume, &file, filename, 0);
	        	add_fs_item(filename, FS_TYPE_FILE);
	        	size_t size = 0;
	        	size_t content_size = request.content_end - request.content_start;
	        	strncpy(content, request.content_start, content_size);
	        	fs_file_write(&file, content, content_size, &size);
	        } else {
	            return HTTP_FAILED;
	        }
	 }

	    return HTTP_OK;
}


uint8_t predefined_get_cgi_processor(uint8_t * uri_name, uint8_t * buf, uint16_t * len)
{
	return 0;
}

uint8_t predefined_set_cgi_processor(uint8_t * uri_name, uint8_t * uri, uint8_t * buf, uint16_t * en)
{
	return 0;
}

uint8_t disassemble_post_request(st_http_request * p_http_request, post_request_t* request){
	request->content_length_header = strstr((char *)p_http_request->URI, "Content-Length: ");
	request->content_length = 0;

	if (request->content_length_header)
	{
		request->content_length_header += strlen("Content-Length: ");
		request->content_length = atoi(request->content_length_header);
	}

	if (request->content_length > 0)
	{
		request->content_disposition = strstr((char *)p_http_request->URI, "Content-Disposition: form-data;");
		request->content_type = strstr((char *)request->content_disposition, "Content-Type:");
		request->content_start = strstr((char*)request->content_disposition, "\r\n\r\n");
		request->content_start += 4;
		request->content_end = strstr((char*)request->content_disposition,"\r\n------");
	} else {
		return HTTP_FAILED;
	}
	return HTTP_OK;
}

void generate_fs_json(char* out, int max_len) {
    strcat(out, "{\"folders\":[");
    int first = 1;
    for (int i = 0; i < fs_item_count; i++) {
        if (fs_items[i].type == FS_TYPE_FOLDER) {
            if (!first) strcat(out, ",");
            strcat(out, "\"");
            strcat(out, fs_items[i].name);
            strcat(out, "\"");
            first = 0;
        }
    }

    strcat(out, "],\"files\":[");
    first = 1;
    for (int i = 0; i < fs_item_count; i++) {
        if (fs_items[i].type == FS_TYPE_FILE) {
            if (!first) strcat(out, ",");
            strcat(out, "\"");
            strcat(out, fs_items[i].name);
            strcat(out, "\"");
            first = 0;
        }
    }
    strcat(out, "]}");
}

int get_query_param(const char* uri, const char* key, char* out, size_t max_len) {
    char* pos = strstr(uri, "?");
    if (!pos) return false;
    pos++; // пропустить '?'

    if (strncmp(pos, key, strlen(key)) == 0 && pos[strlen(key)] == '=') {
    	char* value = pos + strlen(key) + 1;
    	pos = value;
    	int i = 0;
    	while(*value){
    		if((value[0] == '%') && (value[1] == '2') && (value[2] == 'F')){
				out[i++] = '/';
				value += 3;
			} else {
				out[i++] = *value++;
			}

    	}
    	out[i] = '\0';
    	return true;
	}

    return false;
}
