#ifndef _FXFS_FAT_HEADER_
#define _FXFS_FAT_HEADER_

/** 
  ****************************************************************************************************
  *  @file   fx_file.h
  *  @brief  Wrappers around FAT implementation.
  *
  ****************************************************************************************************
  *  Copyright (C) 2008-2019 Eremex
  *  All rights reserved.
  ***************************************************************************************************/

#include "fatfs.h"
#include "fs_media.h"

/*
 * fs_file_seek() position search method
 */
#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2

/*
 * Volume/partition object.
 */
typedef struct _fs_vol_t
{
    struct fatfs_vol fmp;
    fs_media_t* media;
}
fs_vol_t;

/*
 * Directory object.
 */
typedef struct _fs_dir_t
{
    struct fatfs_vol* fmp;
    struct fatfs_node dirnode;
    uint32_t index;
}
fs_dir_t;

/*
 * File object.
 */
typedef struct _fs_file_t
{
    struct fatfs_node parent_dir;
    struct fatfs_node file_node;
    struct fatfs_vol* fmp;
    uint32_t offset;
}
fs_file_t;

#define fs_volume_format fatfs_format

int fs_volume_open(fs_media_t* media, fs_vol_t* volume, uintptr_t part, void* (*mem_alloc)(size_t));
int fs_volume_close(fs_vol_t* volume);

int fs_dir_create(fs_vol_t* volume, char* directory_name);
int fs_dir_delete(fs_vol_t* volume, char* directory_name);

int fs_dir_open(fs_vol_t* volume, char* directory_name, fs_dir_t *dir);
int fs_dir_close(fs_dir_t *dir);
int fs_dir_read_first_entry(fs_dir_t *dir, char* entry);
int fs_dir_read_next_entry(fs_dir_t *dir, char* entry);

int fs_file_create(fs_vol_t* volume, char* file_name);
int fs_file_delete(fs_vol_t* volume, char* file_name);
int fs_file_open(fs_vol_t* volume, fs_file_t* file_ptr, char* file_name, int open_type);

int fs_file_close(fs_file_t *file_ptr);
int fs_file_read(fs_file_t *file_ptr, void* buffer_ptr, uint32_t request_size, size_t* actual_size);
int fs_file_seek(fs_file_t *file_ptr, uint32_t byte_offset, int method);
int fs_file_trunc(fs_file_t *file_ptr, uint32_t size);
int fs_file_write(fs_file_t *file_ptr, void* buffer_ptr, uint32_t request_size, size_t* size);

int fs_rename(fs_vol_t* volume, char* old_name, char* new_name);

#define fs_file_rename(vol, old_name, new_name) fs_rename(vol, old_name, new_name)
#define fs_dir_rename(vol, old_name, new_name) fs_rename(vol, old_name, new_name)

#endif
