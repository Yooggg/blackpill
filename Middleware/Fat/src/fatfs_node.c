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

/*
 * Read directory entry to buffer, with cache.
 */
int
fat_read_dirent(struct fatfs_vol *fmp, uint32_t sec)
{
    size_t size = SEC_SIZE;
    int error;

    if ((error = fmp->dev->read(fmp->dev, fmp->dir_buf, &size, sec)) != 0)
        return error;
    return 0;
}

/*
 * Write directory entry from buffer.
 */
int 
fat_write_dirent_direct(struct fatfs_vol *fmp, const struct fat_dirent* de, uint32_t sec, uint32_t offset)
{
    size_t size = SEC_SIZE;
    int error = fmp->dev->read(fmp->dev, fmp->dir_buf, &size, sec);

    if (error)
        return error;

    memcpy(fmp->dir_buf + offset, de, sizeof(*de));
    return fmp->dev->write(fmp->dev, fmp->dir_buf, &size, sec);
}

int 
fat_empty_dirents_direct(struct fatfs_vol *fmp, uint32_t cl)
{
    uint32_t i;
    int error;
    uint32_t sec = cl_to_sec(fmp, cl);
    size_t size = SEC_SIZE;

    /* Initialize free cluster. */
    memset(fmp->dir_buf, 0, SEC_SIZE);

    for (i = 0; i < fmp->sec_per_cl; i++) {
        error = fmp->dev->write(fmp->dev, fmp->dir_buf, &size, sec);
        if (error)
            return error;
        sec++;
    }
    return error;
}

/*
 * Find directory entry in specified sector.
 * The fat vnode data is filled if success.
 *
 * @fmp: fatfs mount point
 * @sec: sector#
 * @name: file name
 * @node: pointer to fat node
 */
int
fat_lookup_dirent(struct fatfs_vol *fmp, uint32_t sec, char *name, struct fatfs_node *np)
{
    struct fat_dirent *de;
    int error, i;

    error = fat_read_dirent(fmp, sec);
    if (error)
        return error;

    de = (struct fat_dirent *)fmp->dir_buf;

    for (i = 0; i < DIR_PER_SEC; i++) {
        /* Find specific file or directory name */
        if (IS_EMPTY(de))
            return ENOENT;
        if (!IS_VOL(de) &&
            !fat_compare_name((char *)de->name, name)) {
            /* Found. Fill the fat vnode data. */
            *(&np->dirent) = *de;
            np->sector = sec;
            np->offset = sizeof(struct fat_dirent) * i;
            DPRINTF(("fat_lookup_dirent: found sec=%d\n", sec));
            return 0;
        }
        if (!IS_DELETED(de))
            DPRINTF(("fat_lookup_dirent: %s\n", de->name));
        de++;
    }
    return EAGAIN;
}

/*
 * Find directory entry for specified name in directory.
 * The fat vnode data is filled if success.
 *
 * @dvp: vnode for directory.
 * @name: file name
 * @np: pointer to fat node
 */
int
fatfs_lookup_node(struct fatfs_vol * dvp, char *name, struct fatfs_node *np, uint32_t cl)
{
    struct fatfs_vol *fmp = dvp;
    char fat_name[12];
    uint32_t sec, sec_start;
    unsigned int i;
    int error;

    if (name == NULL)
        return ENOENT;

    DPRINTF(("fat_lookup_denode: cl=%d name=%s\n", dvp->v_blkno, name));

    fat_convert_name(name, fat_name);
    *(fat_name + 11) = '\0';

    if (cl == CL_ROOT && !(FAT32(fmp)) ) {
        /* Search entry in root directory */
        sec_start = fmp->root_start;
        for (sec = sec_start; sec < fmp->data_start; sec++) {
            error = fat_lookup_dirent(fmp, sec, fat_name, np);
            if (error != EAGAIN)
                return error;
        }
    } else {
    if(cl == CL_ROOT)
            cl = fmp->root_start;
        /* Search entry in sub directory */
        while (!IS_EOFCL(fmp, cl)) {
            sec = cl_to_sec(fmp, cl);
            for (i = 0; i < fmp->sec_per_cl; i++) {
                error = fat_lookup_dirent(fmp, sec, fat_name, np);
                if (error != EAGAIN)
                    return error;
                sec++;
            }
            error = fat_next_cluster(fmp, cl, &cl);
            if (error)
                return error;
        }
    }
    return ENOENT;
}

/*
 * Get directory entry for specified index in sector.
 * The directory entry is filled if success.
 *
 * @fmp: fatfs mount point
 * @sec: sector#
 * @target: target index
 * @index: current index
 * @np: pointer to fat node
 */
static int
fat_get_dirent(struct fatfs_vol *fmp, uint32_t sec, int target, int *index, struct fatfs_node *np)
{
    struct fat_dirent *de;
    int error, i;

    error = fat_read_dirent(fmp, sec);
    if (error)
        return error;

    de = (struct fat_dirent *)fmp->dir_buf;
    for (i = 0; i < DIR_PER_SEC; i++) {
        if (IS_EMPTY(de))
            return ENOENT;
        if (!IS_DELETED(de) && !IS_VOL(de)) {
            /* valid file */
            if (*index == target) {
                *(&np->dirent) = *de;
                np->sector = sec;
                np->offset = sizeof(struct fat_dirent) * i;
                DPRINTF(("fat_get_dirent: found index=%d\n", *index));
                return 0;
            }
            (*index)++;
        }
        DPRINTF(("fat_get_dirent: %s\n", de->name));
        de++;
    }
    return EAGAIN;
}

/*
 * Get directory entry for specified index.
 *
 * @dvp: vnode for directory.
 * @index: index of the entry
 * @np: pointer to fat node
 */
int
fatfs_get_node(struct fatfs_vol * dvp, int index, struct fatfs_node *np, uint32_t cl)
{
    struct fatfs_vol *fmp = dvp;
    uint32_t sec, sec_start;
    unsigned i;
    int cur_index, error;

    cur_index = 0;

    DPRINTF(("fatfs_get_node: index=%d\n", index));

    if (cl == CL_ROOT && !(FAT32(fmp)) ) {
        /* Get entry from the root directory */
        sec_start = fmp->root_start;
        for (sec = sec_start; sec < fmp->data_start; sec++) {
            error = fat_get_dirent(fmp, sec, index, &cur_index, np);
            if (error != EAGAIN)
                return error;
        }
    } else {
    if(cl == CL_ROOT)   
            cl = fmp->root_start;
        /* Get entry from the sub directory */
        while (!IS_EOFCL(fmp, cl)) {
            sec = cl_to_sec(fmp, cl);
            for (i = 0; i < fmp->sec_per_cl; i++) {
                error = fat_get_dirent(fmp, sec, index, &cur_index, np);
                if (error != EAGAIN)
                    return error;
                sec++;
            }
            error = fat_next_cluster(fmp, cl, &cl);
            if (error)
                return error;
        }
    }
    return ENOENT;
}

/*
 * Find empty directory entry and put new entry on it.
 *
 * @fmp: fatfs mount point
 * @sec: sector#
 * @np: pointer to fat node
 */
static int
fat_add_dirent(struct fatfs_vol *fmp, uint32_t sec, struct fatfs_node *np)
{
    struct fat_dirent *de;
    int error, i;
    uint32_t offset = 0;

    error = fat_read_dirent(fmp, sec);
    if (error)
        return error;

    de = (struct fat_dirent *)fmp->dir_buf;
    for (i = 0; i < DIR_PER_SEC; i++) {
        if (IS_DELETED(de) || IS_EMPTY(de))
            goto found;
        DPRINTF(("fat_add_dirent: scan %s\n", de->name));
        de++;
        offset += sizeof(struct fat_dirent);
    }
    return ENOENT;

 found:
    DPRINTF(("fat_add_dirent: found. sec=%d\n", sec));
    error = fat_write_dirent(fmp, &np->dirent, sec, offset);
    return error;
}

/*
 * Find empty directory entry and put new entry on it.
 * This search is done only in directory of specified cluster.
 * @dvp: vnode for directory.
 * @np: pointer to fat node
 */
int
fatfs_add_node(struct fatfs_vol *dvp, struct fatfs_node *np, uint32_t cl)
{
    struct fatfs_vol *fmp;
    uint32_t sec,sec_start;
    int error;
    unsigned i;
    uint32_t next;

    fmp = (struct fatfs_vol *)dvp;

    DPRINTF(("fatfs_add_node: cl=%d\n", cl));

    if (cl == CL_ROOT && !(FAT32(fmp)) ) {
        /* Add entry in root directory */
        sec_start = fmp->root_start;
        for (sec = sec_start; sec < fmp->data_start; sec++) {
            error = fat_add_dirent(fmp, sec, np);
            if (error != ENOENT)
                return error;
        }
    } else {
        if(cl == CL_ROOT)   
            cl = fmp->root_start;
        /* Search entry in sub directory */
        while (!IS_EOFCL(fmp, cl)) {
            sec = cl_to_sec(fmp, cl);
            for (i = 0; i < fmp->sec_per_cl; i++) {
                error = fat_add_dirent(fmp, sec, np);
                if (error != ENOENT)
                    return error;
                sec++;
            }
            error = fat_next_cluster(fmp, cl, &next);
            if (error)
                return error;
            cl = next;
        }
        /* No entry found, add one more free cluster for directory */
        DPRINTF(("fatfs_add_node: expand dir\n"));
        error = fat_expand_dir(fmp, cl, &next);
        if (error)
            return error;

        error = fat_empty_dirents(fmp, next);
        if (error)
            return error;

        /* Try again */
        sec = cl_to_sec(fmp, next);
        error = fat_add_dirent(fmp, sec, np);
        return error;
    }
    return ENOENT;
}

/*
 * Put directory entry.
 * @fmp: fat mount data
 * @np: pointer to fat node
 */
int
fatfs_put_node(struct fatfs_vol *fmp, struct fatfs_node *np)
{
    int error;

    error = fat_read_dirent(fmp, np->sector);
    if (error)
        return error;

    error = fat_write_dirent(fmp, &np->dirent, np->sector, np->offset);
    return error;
}

