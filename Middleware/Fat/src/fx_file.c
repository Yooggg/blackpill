/** 
  ****************************************************************************************************
  *  @file   fx_file.c
  *  @brief  Wrappers around FAT implementation.
  *
  ****************************************************************************************************
  *  Copyright (C) 2008-2019 Eremex
  *  All rights reserved.
  ***************************************************************************************************/

#include <string.h>
#include <errno.h>
#include "fx_file.h"

/*
 * Open FAT volume at given partition on specified storage device.
 */
int 
fs_volume_open(fs_media_t* media, fs_vol_t* volume, uintptr_t part, void* (*mem_alloc)(size_t))
{
    int error = EIO;
    size_t nbytes = SEC_SIZE;
    char mbr[SEC_SIZE];
    uint32_t part_base;
    uint32_t part_size;
    uint16_t signature;

    if(!media || !volume) return EINVAL;

    fs_media_lock(media);

    do {
        /* Read Master Boot Record. */
        error = media->read(media, mbr, &nbytes, 0);
        if(error || nbytes != SEC_SIZE) break;

        /* Verify MBR signature. */
        memcpy(&signature, mbr + 510, sizeof(signature));
        if(signature != 0xaa55) break;

        /* Read partition base and size. */
        memcpy(&part_base, mbr + 0x1BE + 16*part + 8, sizeof(uint32_t));
        memcpy(&part_size, mbr + 0x1BE + 16*part + 12, sizeof(uint32_t));

        /* Try initialize FAT volume. */
        error = fatfs_init(&volume->fmp, media, part_base, mem_alloc);
    }
    while (0);

    fs_media_unlock(media);
    return error;
}

/*
 * No actions required when FAT volume is being closed.
 */
int 
fs_volume_close(fs_vol_t* volume)
{
    if(!volume) return EINVAL;
    return 0;
}

/*
 * Create directory (all items of the path except last one must be existing directories).
 */
int 
fs_dir_create(fs_vol_t* volume, char* directory_name)
{
    int error;
    struct fatfs_node np;
    char* dirname;

    if(!volume || !directory_name)
        return EINVAL;

    fs_media_lock(volume->media);

    do {
        /* Get parent directory node and new directory name. */
        error = fatfs_get_node_by_path(&volume->fmp, directory_name, &np, &dirname);
        if (error) break;

        /* Make new directory. */
        error = fatfs_mkdir(&volume->fmp, &np, dirname, 0);
    }
    while (0);

    fs_media_unlock(volume->media);
    return error;
}

/*
 * Delete directory (all items of the path except last one must be existing directories).
 */
int 
fs_dir_delete(fs_vol_t* volume, char* directory_name)
{
    int error;
    struct fatfs_node np;
    char* dirname;

    if(!volume || !directory_name)
        return EINVAL;

    fs_media_lock(volume->media);

    do {
        /* Get parent directory node and target directory name. */
        error = fatfs_get_node_by_path(&volume->fmp, directory_name, &np, &dirname);
        if (error) break;

        /* Remove specified directory. */
        error = fatfs_rmdir(&volume->fmp, &np, dirname);
    }
    while (0);

    fs_media_unlock(volume->media);
    return error;
}

/*
 * Open directory and initialize directory object.
 */
int 
fs_dir_open(fs_vol_t* volume, char* directory_name, fs_dir_t *dir)
{
    int error;
    struct fatfs_node np;
    struct fatfs_node dirnode;
    char* dirname;

    if(!volume || !directory_name || !dir)
        return EINVAL;

    fs_media_lock(volume->media);

    do {
        /* Get parent directory node and target directory name. */
        error = fatfs_get_node_by_path(&volume->fmp, directory_name, &np, &dirname);
        if (error) break;

        if (!dirname[0]) {
            memset(dir, 0, sizeof(*dir));
            dir->fmp = &volume->fmp;
            dir->dirnode.dirent.attr |= FA_SUBDIR;
        } else {
            /* Lookup specified directory and (if succeeded) save directory node. */
            error = fatfs_lookup(&volume->fmp, &np, dirname, &dirnode);

            if (!error) {
                memcpy(&dir->dirnode, &dirnode, sizeof(dir->dirnode));
                dir->fmp = &volume->fmp;
                dir->index = 0;
            }
        }
    }
    while (0);

    fs_media_unlock(volume->media);
    return error;
}

/*
 * No actions required to close directory.
 */
int 
fs_dir_close(fs_dir_t *dir)
{
    if(!dir)
        return EINVAL;

    /* Nothing to do. */
    return 0;
}

/*
 * Read directory content.
 */
int 
fs_dir_read_next_entry(fs_dir_t *dir, char* entry)
{
    struct fatfs_node temp;
    struct fat_dirent *de = &dir->dirnode.dirent;
    int error;

    fs_media_t* media;

    if (!dir || !dir->fmp || !entry)
        return EINVAL;

    media = dir->fmp->dev;

    fs_media_lock(media);

    error = fatfs_get_node(dir->fmp, dir->index, &temp, DE_CLUSTER(de));
    if (error)
        goto out;
    de = &temp.dirent;
    fat_restore_name((char *)&de->name, entry);

    fs_media_unlock(media);

    dir->index++;
    error = 0;
out:
    return error;
}

/*
 * Read first item of the directory (usually this is self-link ".") and initialize index.
 */
int
fs_dir_read_first_entry(fs_dir_t *dir, char* entry)
{
    /* Reset dir entry iterator. */
    dir->index = 0;
    return fs_dir_read_next_entry(dir, entry);
}

/*
 * Create new file with no content.
 */
int 
fs_file_create(fs_vol_t* volume, char* file_name)
{
    int error;
    struct fatfs_node np;
    char* filename;

    if(!volume || !file_name)
        return EINVAL;

    fs_media_lock(volume->media);

    do {
        /* Get parent directory node and file name. */
        error = fatfs_get_node_by_path(&volume->fmp, file_name, &np, &filename);
        if (error) break;

        /* Create new file in specified directory with zero attributes */
        error = fatfs_create(&volume->fmp, &np, filename, 0);
    }
    while (0);

    fs_media_unlock(volume->media);
    return error;
}

/*
 * Delete file.
 */
int 
fs_file_delete(fs_vol_t* volume, char* file_name)
{
    int error;
    struct fatfs_node np;
    char* filename;

    if(!volume || !file_name)
        return EINVAL;

    fs_media_lock(volume->media);

    do {
        /* Get parent directory node and file name. */
        error = fatfs_get_node_by_path(&volume->fmp, file_name, &np, &filename);
        if (error) break;

        error = fatfs_remove(&volume->fmp, &np, filename);
    }
    while (0);

    fs_media_unlock(volume->media);
    return error;
}

/*
 * Rename object. This function is applicable for all types of objects: files and directories.
 */
int 
fs_rename(fs_vol_t* volume, char* old_name, char* new_name)
{
    int error;
    struct fatfs_node parent_dir_old;
    struct fatfs_node parent_dir_new;
    char* old_itemname;
    char* new_itemname;
    bool same_dir = false;

    if(!volume || !old_name || !new_name)
        return EINVAL;

    fs_media_lock(volume->media);

    do {
        /* Get parent directory node and file name for old path. */
        error = fatfs_get_node_by_path(&volume->fmp, old_name, &parent_dir_old, &old_itemname);
        if(error) break;

        /* Get parent directory node and file name for new path. */
        error = fatfs_get_node_by_path(&volume->fmp, new_name, &parent_dir_new, &new_itemname);
        if(error) break;

        /* Underlying logic determines whether both files reside in the same dir by node pointers:
           equal pointers have "same dir" meaning, so, compare directory location and use single 
           pointer if appropriate.
        */
        same_dir =  (parent_dir_old.sector == parent_dir_new.sector) && 
                    (parent_dir_old.offset == parent_dir_new.offset);

        error = fatfs_rename(
            &volume->fmp, 
            &parent_dir_old, 
            old_itemname, 
            same_dir ? &parent_dir_old : &parent_dir_new, 
            new_itemname
        );
    }
    while (0);

    fs_media_unlock(volume->media);
    return error;
}

/*
 * Open file for read or write.
 */
int 
fs_file_open(fs_vol_t* volume, fs_file_t* file_ptr, char* file_name, int open_type)
{
    int error;
    char* filename;

    if(!volume || !file_ptr || !file_name)
        return EINVAL;

    fs_media_lock(volume->media);

    do {
        /* Get parent directory node and file name for the path. */
        error = fatfs_get_node_by_path(&volume->fmp, file_name, &file_ptr->parent_dir, &filename);
        if (error) break;

        /* Get file node. */
        error = fatfs_lookup(&volume->fmp, &file_ptr->parent_dir, filename, &file_ptr->file_node);
        if (!error)
        {
            file_ptr->fmp = &volume->fmp;
            file_ptr->offset = 0;
        }
    }
    while (0);

    fs_media_unlock(volume->media);
    return error;
}

/*
 * No actions required in order to close file.
 */
int 
fs_file_close(fs_file_t *file_ptr)
{
    if(!file_ptr)
        return EINVAL;

    /* Nothing to do. */
    return 0;
}

/*
 * Seek file.
 */
int 
fs_file_seek(fs_file_t *file_ptr, uint32_t byte_offset, int method)
{
    uint32_t size;

    if(!file_ptr || !file_ptr->fmp)
        return EINVAL;

    size = file_ptr->file_node.dirent.size;

    switch(method)
    {
    case SEEK_SET:
        file_ptr->offset = byte_offset;
        break; 
    case SEEK_CUR:
        file_ptr->offset += byte_offset;
        break;
    case SEEK_END:
        file_ptr->offset = size - byte_offset;
        break; 
    default:
        return EINVAL;
    }

    return 0;
}

/*
 * Truncate file.
 */
int 
fs_file_trunc(fs_file_t *file_ptr, uint32_t size)
{
    int error;
    fs_media_t* media;

    if(!file_ptr || !file_ptr->fmp)
        return EINVAL;

    media = file_ptr->fmp->dev;

    fs_media_lock(media);
    error = fatfs_truncate(file_ptr->fmp, &file_ptr->file_node, size);
    fs_media_unlock(media);
    return error;
}

/*
 * Reading file content.
 */
int 
fs_file_read(fs_file_t *file_ptr, void* buffer_ptr, uint32_t request_size, size_t* actual_size)
{
    int error;
    fs_media_t* media;

    if(!file_ptr || !file_ptr->fmp || !buffer_ptr || !actual_size)
        return EINVAL;

    media = file_ptr->fmp->dev;

    fs_media_lock(media);
    error = fatfs_read(
        file_ptr->fmp, 
        &file_ptr->file_node, 
        &file_ptr->offset, 
        buffer_ptr, 
        request_size, 
        actual_size
    );

    fs_media_unlock(media);
    return error;
}

/*
 * Writing file content.
 */
int 
fs_file_write(fs_file_t *file_ptr, void* buffer_ptr, uint32_t request_size, size_t* size)
{
    int error;
    fs_media_t* media;

    if(!file_ptr || !file_ptr->fmp || !buffer_ptr || !size)
        return EINVAL;

    media = file_ptr->fmp->dev;

    fs_media_lock(media);

    error = fatfs_write(
        file_ptr->fmp, 
        &file_ptr->file_node, 
        &file_ptr->offset, 
        buffer_ptr, 
        request_size, 
        size, 
        1
    );

    fs_media_unlock(media);
    return error;
}
