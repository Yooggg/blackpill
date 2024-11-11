/*
 * Copyright (c) 2005-2008, Kohsuke Ohtani
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <errno.h>
#include <string.h>
#include "fatfs.h"

static int _fatfs_remove(struct fatfs_vol *, struct fatfs_node *, char *);
static int _fatfs_rmdir(struct fatfs_vol *, struct fatfs_node *, char *);

/*
 *  Time bits: 15-11 hours (0-23), 10-5 min, 4-0 sec /2
 *  Date bits: 15-9 year - 1980, 8-5 month, 4-0 day
 */
#define TEMP_DATE   0x3021
#define TEMP_TIME   0

/*
 * Read one cluster to buffer.
 */
static int
fat_read_cluster(struct fatfs_vol *fmp, uint32_t cluster)
{
    uint32_t sec;
    size_t size;

    sec = cl_to_sec(fmp, cluster);
    size = fmp->sec_per_cl * SEC_SIZE;
    return fmp->dev->read(fmp->dev, fmp->io_buf, &size, sec);
}

/*
 * Write one cluster from buffer.
 */
static int
fat_write_cluster(struct fatfs_vol *fmp, uint32_t cluster)
{
    uint32_t sec;
    size_t size;

    sec = cl_to_sec(fmp, cluster);
    size = fmp->sec_per_cl * SEC_SIZE;
    return fmp->dev->write(fmp->dev, fmp->io_buf, &size, sec);
}

/*
 * Lookup vnode for the specified file/directory.
 * The vnode data will be set properly.
 */
int
fatfs_lookup(
    struct fatfs_vol *fmp, 
    struct fatfs_node *dp, 
    char *name, 
    struct fatfs_node *np)
{
    int error;
    uint32_t blkno = DE_CLUSTER(&(dp->dirent));

    if (*name == '\0')
        return ENOENT;

    DPRINTF(("fatfs_lookup: name=%s\n", name));

    error = fatfs_lookup_node(fmp, name, np, blkno);
    if (error) {
        DPRINTF(("fatfs_lookup: failed!! name=%s\n", name));
        return error;
    }

    DPRINTF(("fatfs_lookup: cl=%d\n", blkno));
    return 0;
}

int
fatfs_read(
    struct fatfs_vol *fmp, 
    struct fatfs_node *np, 
    uint32_t* f_offset, 
    void *buf, 
    size_t size, 
    size_t *result)
{
    int nr_read, nr_copy, buf_pos, error;
    uint32_t cl, file_pos;
    struct fat_dirent* de = &np->dirent;

    DPRINTF(("fatfs_read: vp=%x\n", vp));

    *result = 0;

    if (IS_DIR(de))
        return EISDIR;

    /* Check if current file position is already end of file. */
    file_pos = *f_offset;
    if (file_pos >= de->size)
        return 0;

    /* Get the actual read size. */
    if (de->size - file_pos < size)
        size = de->size - file_pos;

    /* Seek to the cluster for the file offset */
    error = fat_seek_cluster(fmp, DE_CLUSTER(de) , file_pos, &cl);
    if (error)
        goto out;

    /* Read and copy data */
    nr_read = 0;
    buf_pos = file_pos % fmp->cluster_size;
    do {
        if (fat_read_cluster(fmp, cl)) {
            error = EIO;
            goto out;
        }

        nr_copy = fmp->cluster_size;
        if (buf_pos > 0)
            nr_copy -= buf_pos;
        if (buf_pos + size < fmp->cluster_size)
            nr_copy = size;
        memcpy(buf, fmp->io_buf + buf_pos, nr_copy);

        file_pos += nr_copy;
        nr_read += nr_copy;
        size -= nr_copy;
        if (size <= 0)
            break;

        error = fat_next_cluster(fmp, cl, &cl);
        if (error)
            goto out;

        buf = (void *)((uint32_t)buf + nr_copy);
        buf_pos = 0;
    } while (!IS_EOFCL(fmp, cl));

    *f_offset = file_pos;
    *result = nr_read;
    error = 0;
 out:
    return error;
}

int
fatfs_write(
    struct fatfs_vol *fmp, 
    struct fatfs_node *np, 
    uint32_t* f_offset, 
    void *buf, 
    size_t size, 
    size_t *result, 
    int append)
{
    struct fat_dirent *de = &np->dirent;
    int nr_copy, nr_write, buf_pos, i, cl_size, error;
    uint32_t file_pos, end_pos;
    uint32_t cl;

    DPRINTF(("fatfs_write: vp=%x\n", vp));

    *result = 0;

    if (IS_DIR(de))
        return EISDIR;

    /* Check if file position exceeds the end of file. */
    end_pos = de->size;
    file_pos = (append) ? end_pos : *f_offset;
    if (file_pos + size > end_pos) {
        /* Expand the file size before writing to it */
        end_pos = file_pos + size;
        error = fat_expand_file(fmp, DE_CLUSTER(de), end_pos);
        if (error) {
            error = EIO;
            goto out;
        }

        /* Update directory entry */
        de->size = end_pos;
        error = fatfs_put_node(fmp, np);
        if (error)
            goto out;
    }

    /* Seek to the cluster for the file offset */
    error = fat_seek_cluster(fmp, DE_CLUSTER(de), file_pos, &cl);
    if (error)
        goto out;

    buf_pos = file_pos % fmp->cluster_size;
    cl_size = size / fmp->cluster_size + 1;
    nr_write = 0;
    i = 0;
    do {
        /* First and last cluster must be read before write */
        if (i == 0 || i == cl_size) {
            if (fat_read_cluster(fmp, cl)) {
                error = EIO;
                goto out;
            }
        }
        nr_copy = fmp->cluster_size;
        if (buf_pos > 0)
            nr_copy -= buf_pos;
        if (buf_pos + size < fmp->cluster_size)
            nr_copy = size;
        memcpy(fmp->io_buf + buf_pos, buf, nr_copy);

        if (fat_write_cluster(fmp, cl)) {
            error = EIO;
            goto out;
        }
        file_pos += nr_copy;
        nr_write += nr_copy;
        size -= nr_copy;
        if (size <= 0)
            break;

        error = fat_next_cluster(fmp, cl, &cl);
        if (error)
            goto out;

        buf = (void *)((uint32_t)buf + nr_copy);
        buf_pos = 0;
        i++;
    } while (!IS_EOFCL(fmp, cl));

    *f_offset = file_pos;

    /*
     * XXX: Todo!
     *    de.time = ?
     *    de.date = ?
     *    if (dirent_set(fp, &de))
     *        return EIO;
     */
    *result = nr_write;
    error = 0;
out:
    fatfs_commit(fmp, error);
    return error;
}

/*
 * Create empty file.
 */
int
fatfs_create(
    struct fatfs_vol *fmp, 
    struct fatfs_node *np, 
    char *name, 
    uint8_t attr)
{
    struct fatfs_node newnode;
    struct fat_dirent *de;
    uint32_t cl;
    int error;

    DPRINTF(("fatfs_create: %s\n", name));

    if (!fat_valid_name(name))
        return EINVAL;

    /* Allocate free cluster for new file. */
    error = fat_alloc_cluster(fmp, 0, &cl);
    if (error)
        goto out;

    de = &newnode.dirent;
    memset(de, 0, sizeof(struct fat_dirent));
    fat_convert_name(name, (char *)de->name);
    de->cluster_hi = (cl & 0xFFFF0000) >> 16;
    de->cluster = cl & 0x0000FFFF;
    de->time = TEMP_TIME;
    de->date = TEMP_DATE;
    de->attr = attr;
    error = fatfs_add_node(fmp, &newnode, DE_CLUSTER(&np->dirent));
    if (error)
        goto out;
    error = fat_set_cluster(fmp, cl, fmp->fat_eof);
out:
    fatfs_commit(fmp, error);
    return error;
}

static int
_fatfs_remove(struct fatfs_vol *fmp, struct fatfs_node *np, char *name)
{
    struct fatfs_node temp;
    struct fat_dirent *de;
    int error;

    if (*name == '\0')
        return ENOENT;

    error = fatfs_lookup_node(fmp, name, &temp, DE_CLUSTER(&np->dirent));
    if (error)
        goto out;
    de = &temp.dirent;
    if (IS_DIR(de)) {
        error = EISDIR;
        goto out;
    }
    if (!IS_FILE(de)) {
        error = EPERM;
        goto out;
    }

    /* Remove clusters */
    error = fat_free_clusters(fmp, DE_CLUSTER(de));
    if (error)
        goto out;

    /* remove directory */
    de->name[0] = 0xe5;
    error = fatfs_put_node(fmp, &temp);
out:
    return error;
}

int
fatfs_remove(struct fatfs_vol *fmp, struct fatfs_node *np, char *name)
{
    int error = _fatfs_remove(fmp, np, name);
    fatfs_commit(fmp, error);
    return error;
}

int
fatfs_rename(
    struct fatfs_vol *fmp, 
    struct fatfs_node *dp1, 
    char *name1, 
    struct fatfs_node *dp2, 
    char *name2)
{
    struct fatfs_node np1;
    struct fat_dirent *de1;
    int error;

    error = fatfs_lookup_node(fmp, name1, &np1, DE_CLUSTER(&dp1->dirent));
    if (error)
        goto out;
    de1 = &np1.dirent;

    if (IS_FILE(de1)) {
        /* Remove destination file, first */
        error = _fatfs_remove(fmp, dp2, name2);
        if (error == EIO)
            goto out;

        /* Change file name of directory entry */
        fat_convert_name(name2, (char *)de1->name);

        /* Same directory ? */
        if (dp1 == dp2) {
            /* Change the name of existing file */
            error = fatfs_put_node(fmp, &np1);
            if (error)
                goto out;
        } else {
            /* Create new directory entry */
            error = fatfs_add_node(fmp, &np1, DE_CLUSTER(&dp2->dirent));
            if (error)
                goto out;

            /* Remove souce file */
            de1->name[0] = 0xe5;
            error = fatfs_put_node(fmp, &np1);

            if (error)
                goto out;
        }
    } else {
        struct fat_dirent de2;

        /* remove destination directory */
        error = _fatfs_rmdir(fmp, dp2, name2);
        if (error == EIO)
            goto out;

        /* Change file name of directory entry */
        fat_convert_name(name2, (char *)de1->name);

        /* Same directory ? */
        if (dp1 == dp2) {
            /* Change the name of existing directory */
            error = fatfs_put_node(fmp, &np1);
            if (error)
                goto out;
        } else {
            /* Create new directory entry */
            error = fatfs_add_node(fmp, &np1, DE_CLUSTER(&dp2->dirent));
            if (error)
                goto out;

            /* Update "." and ".." for renamed directory */
            if (fat_read_dirent(fmp, cl_to_sec(fmp, DE_CLUSTER(de1)))) {
                error = EIO;
                goto out;
            }

            de2.cluster_hi = de1->cluster_hi;
            de2.cluster = de1->cluster;
            de2.time = TEMP_TIME;
            de2.date = TEMP_DATE;

            if (fat_write_dirent(fmp, &de2, cl_to_sec(fmp, DE_CLUSTER(de1)), 0)) {
                error = EIO;
                goto out;
            }

            de2.cluster = dp2->dirent.cluster;
            de2.cluster_hi = dp2->dirent.cluster_hi;
            de2.time = TEMP_TIME;
            de2.date = TEMP_DATE;

            if (fat_write_dirent(
                    fmp, &de2, cl_to_sec(fmp, DE_CLUSTER(de1)), sizeof(de2))
               ) {
                error = EIO;
                goto out;
            }

            /* Remove source directory */
            de1->name[0] = 0xe5;
            error = fatfs_put_node(fmp, &np1);

            if (error)
                goto out;
        }
    }
out:
    fatfs_commit(fmp, error);
    return error;
}

int
fatfs_mkdir(
    struct fatfs_vol *fmp, 
    struct fatfs_node *dp,
    char *name, 
    uint32_t attr)
{
    struct fatfs_node np;
    struct fat_dirent *de;
    uint32_t cl;
    int error;

    if (!fat_valid_name(name))
        return ENOTDIR;

    /* Allocate free cluster for directory data */
    error = fat_alloc_cluster(fmp, 0, &cl);
    if (error)
        goto out;

    memset(&np, 0, sizeof(struct fatfs_node));
    de = &np.dirent;
    fat_convert_name(name, (char *)&de->name);
    de->cluster_hi = (cl & 0xFFFF0000) >> 16;
    de->cluster = cl & 0x0000FFFF;
    de->time = TEMP_TIME;
    de->date = TEMP_DATE;
    de->attr = attr | FA_SUBDIR;
    error = fatfs_add_node(fmp, &np, DE_CLUSTER(&dp->dirent));
    if (error)
        goto out;

    /* Initialize "." and ".." for new directory */
    error = fat_empty_dirents(fmp, cl);
    if (error)
        goto out;

    de = &np.dirent;
    memcpy(de->name, ".          ", 11);
    de->attr = FA_SUBDIR;
    de->cluster_hi = (cl & 0xFFFF0000) >> 16;
    de->cluster = cl & 0x0000FFFF;
    de->time = TEMP_TIME;
    de->date = TEMP_DATE;

    if (fat_write_dirent(fmp, de, cl_to_sec(fmp, cl), 0)) {
        error = EIO;
        goto out;
    }

    memcpy(de->name, "..         ", 11);
    de->attr = FA_SUBDIR;
    de->cluster = dp->dirent.cluster;
    de->cluster_hi = dp->dirent.cluster_hi;
    de->time = TEMP_TIME;
    de->date = TEMP_DATE;

    if (fat_write_dirent(fmp, de, cl_to_sec(fmp, cl), sizeof(*de))) {
        error = EIO;
        goto out;
    }
    /* Add eof */
    error = fat_set_cluster(fmp, cl, fmp->fat_eof);
out:
    fatfs_commit(fmp, error);
    return error;
}

/*
 * remove can be done only with empty directory
 */
static int
_fatfs_rmdir(struct fatfs_vol *fmp, struct fatfs_node *dp, char *name)
{
    struct fatfs_node np;
    struct fat_dirent *de;
    int error;

    if (*name == '\0')
        return ENOENT;

    error = fatfs_lookup_node(fmp, name, &np, DE_CLUSTER(&dp->dirent));
    if (error)
        goto out;

    de = &np.dirent;
    if (!IS_DIR(de)) {
        error = ENOTDIR;
        goto out;
    }

    /* Remove clusters */
    error = fat_free_clusters(fmp, DE_CLUSTER(de));
    if (error)
        goto out;

    /* remove directory */
    de->name[0] = 0xe5;

    error = fatfs_put_node(fmp, &np);
out:
    return error;
}

int
fatfs_rmdir(struct fatfs_vol *fmp, struct fatfs_node *dp, char *name)
{
    int error = _fatfs_rmdir(fmp, dp, name);
    fatfs_commit(fmp, error);
    return error;
}

int
fatfs_truncate(struct fatfs_vol *fmp, struct fatfs_node *np, uint32_t length)
{
    struct fat_dirent *de = &np->dirent;
    int error;

    if (length == 0) {
        /* Remove clusters */
        error = fat_free_clusters(fmp, DE_CLUSTER(de));
        if (error)
            goto out;
    } else if (length > de->size) {
        error = fat_expand_file(fmp, DE_CLUSTER(de), length);
        if (error) {
            error = EIO;
            goto out;
        }
    }

    /* Update directory entry */
    de->size = length;
    error = fatfs_put_node(fmp, np);
    if (error)
        goto out;
out:
    fatfs_commit(fmp, error);
    return error;
}

/*
 * Read FS info from BIOS parameter block (BPB).
 */
int
fat_read_bpb(struct fatfs_vol *fmp, void* bpb_buf)
{
    struct fat_bpb *bpb = (struct fat_bpb *) bpb_buf;
    struct fat32_bpb *bpb32 = (struct fat32_bpb *) bpb_buf;
    uint32_t fatsize, totalsect, maxclust;

    if (bpb->bytes_per_sector != SEC_SIZE) {
        DPRINTF(("fatfs: invalid sector size\n"));
        return EINVAL;
    }

    /* Initialize the file system object */
    fatsize = bpb->sectors_per_fat;
    if (fatsize == 0) 
        fatsize = bpb32->sectors_per_fat32;

    fatsize *= bpb->num_of_fats;
    fmp->fat_start = bpb->hidden_sectors + bpb->reserved_sectors;

    fmp->sec_per_cl = bpb->sectors_per_cluster;
    fmp->cluster_size = bpb->sectors_per_cluster * SEC_SIZE;
    totalsect = bpb->total_sectors;
    if (totalsect == 0) 
        totalsect = bpb->big_total_sectors;
    maxclust = (
        totalsect - 
        bpb->reserved_sectors - 
        fatsize - 
        (bpb->root_entries / (SEC_SIZE / sizeof(struct fat_dirent))) ) 
            / bpb->sectors_per_cluster + 2;

    fmp->fat_type = 12;
    fmp->fat_mask = FAT12_MASK;
    fmp->fat_eof = CL_EOF & FAT12_MASK;
    if (maxclust >= 0xFF7) {
        fmp->fat_type = 16;
        fmp->fat_mask = FAT16_MASK;
        fmp->fat_eof = CL_EOF & FAT16_MASK;
    }
    if (maxclust >= 0xFFF7) {
        fmp->fat_type = 32;
        fmp->fat_mask = FAT32_MASK;
        fmp->fat_eof = CL_EOF & FAT32_MASK;
    }
    fmp->data_start = 
        fmp->fat_start + fatsize + (bpb->root_entries / DIR_PER_SEC);

    if (FAT32(fmp))
        fmp->root_start = bpb32->root_clust;
    else
        fmp->root_start = fmp->fat_start + fatsize;

    fmp->last_cluster = (totalsect - fmp->data_start) / bpb->sectors_per_cluster + CL_FIRST;
    fmp->free_scan = CL_FIRST;

    DPRINTF(("----- FAT info ----- \n"));

    if(fmp->fat_type == 32){
        DPRINTF(("drive:%x\n", (int)bpb32->physical_drive));
        DPRINTF(("total_sectors:%d\n", (int)totalsect));
        DPRINTF(("heads       :%d\n", (int)bpb->heads));
        DPRINTF(("serial      :%x\n", (int)bpb32->serial_no));
    }else{
        DPRINTF(("drive:%x\n", (int)bpb->physical_drive));
        DPRINTF(("total_sectors:%d\n", (int)totalsect));
        DPRINTF(("heads       :%d\n", (int)bpb->heads));
        DPRINTF(("serial      :%x\n", (int)bpb->serial_no));
    }

    DPRINTF(("cluster size:%u sectors\n", (int)fmp->sec_per_cl));
    DPRINTF(("fat_start   :%x\n", (int)fmp->fat_start));
    DPRINTF(("root_start  :%x\n", (int)fmp->root_start));
    DPRINTF(("data_start  :%x\n", (int)fmp->data_start));
    DPRINTF(("fat_type    :FAT%u\n", (int)fmp->fat_type));
    DPRINTF(("fat_eof     :0x%x\n\n", (int)fmp->fat_eof));

    return 0;
}

/*
 * Mount file system.
 */
int
fatfs_init(
    struct fatfs_vol *fmp, 
    fs_media_t* dev, 
    uint32_t base, 
    void* (*io_mem_alloc)(size_t))
{   
    int error = ENOMEM;
    size_t size = SEC_SIZE;
    void* temp_buf = NULL; 
    void* io_buf = NULL;
    void* fat_buf = NULL;

    do
    {
        temp_buf = io_mem_alloc(SEC_SIZE);

        if (temp_buf == NULL) {
            break;
        }

        /* Read boot sector (block:0) */
        error = dev->read(dev, temp_buf, &size, base);

        if (error) {
            break;
        }

        error = fat_read_bpb(fmp, temp_buf);

        if (error) {
            break;
        }

        io_buf = io_mem_alloc(fmp->sec_per_cl * SEC_SIZE);

        if (io_buf == NULL) {
            error = ENOMEM;
            break;
        }

        fat_buf = io_mem_alloc(SEC_SIZE * 2);

        if (fat_buf == NULL) {
            error = ENOMEM;
            break;
        }

        fmp->dev = dev;
        fmp->io_buf = io_buf;
        fmp->fat_buf = fat_buf;
        fmp->dir_buf = temp_buf;
    }
    while (0);

    return error;
}
