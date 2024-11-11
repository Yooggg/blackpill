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
#include <limits.h>
#include "fatfs.h"

/*
 * Appends src to string dst of size siz (unlike strncat, siz is the
 * full size of dst, not space left).  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz <= strlen(dst)).
 * Returns strlen(src) + MIN(siz, strlen(initial dst)).
 * If retval >= siz, truncation occurred.
 */
size_t
strlcat(char *dst, const char *src, size_t siz)
{
    char *d = dst;
    const char *s = src;
    size_t n = siz;
    size_t dlen;

    /* Find the end of dst and adjust bytes left but don't go past end */
    while (n-- != 0 && *d != '\0')
        d++;
    dlen = d - dst;
    n = siz - dlen;

    if (n == 0)
        return(dlen + strlen(s));
    while (*s != '\0') {
        if (n != 1) {
            *d++ = *s;
            n--;
        }
        s++;
    }
    *d = '\0';

    return(dlen + (s - src));   /* count does not include NUL */
}

/*
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 */
size_t
strlcpy(char *dst, const char *src, size_t siz)
{
    char *d = dst;
    const char *s = src;
    size_t n = siz;

    /* Copy as many bytes as will fit */
    if (n != 0 && --n != 0) {
        do {
            if ((*d++ = *s++) == 0)
                break;
        } while (--n != 0);
    }

    /* Not enough room in dst, add NUL and traverse rest of src */
    if (n == 0) {
        if (siz != 0)
            *d = '\0';      /* NUL-terminate dst */
        while (*s++)
            ;
    }

    return(s - src - 1);    /* count does not include NUL */
} 


/*
 * Convert a pathname into a pointer to a node.
 *
 * @path: full path name.
 */
static int  path_to_fatnode(char *path, struct fatfs_node* np, struct fatfs_vol *m)
{
    char *p = path;
    char node[PATH_MAX];
    char name[PATH_MAX];
    int error, i;
    struct fatfs_node np1;
    struct fatfs_node np2;
    struct fatfs_node* dpp = &np1, *npp = &np2;

    node[0] = '\0';
    memset(&np1, 0, sizeof(np1));
    memset(&np2, 0, sizeof(np2));

    while (*p != '\0') {
        /*
         * Get lower directory/file name.
         */
        while (*p == '/')
            p++;
        for (i = 0; i < PATH_MAX; i++) {
            if (*p == '\0' || *p == '/')
                break;
            name[i] = *p++;
        }
        name[i] = '\0';

        /*
         * Get a vnode for the target.
         */
        strlcat(node, "/", sizeof(node));
        strlcat(node, name, sizeof(node));

        /* Find a vnode in this directory. */
        if(strlen(name) != 0) {
            error = fatfs_lookup(m, dpp, name, npp);
            if (error || (*p == '/' && !IS_DIR(&npp->dirent))) {
                /* Not found */
                return error;
            }
        }
        else {
            memset(np, 0, sizeof(*np));
            np->dirent.attr |= FA_SUBDIR;
            return 0;
        }

        dpp = npp;
        while (*p != '\0' && *p != '/')
            p++;
    }

    memcpy(np, dpp, sizeof(*np));

    return 0;
}

/*
 * Search a pathname.
 * This is a very central but not so complicated routine. ;-P
 *
 * @path: full path.
 * @vpp:  pointer to locked vnode for directory.
 * @name: pointer to file name in path.
 *
 * This routine returns a locked directory vnode and file name.
 */
int fatfs_get_node_by_path(struct fatfs_vol *fmp, char *path, struct fatfs_node* np, char **name)
{
    char buf[PATH_MAX];
    char root[] = "/";
    char *file, *dir;
    int error;
    struct fatfs_node dp;
    /*
     * Get the path for directory.
     */
    strlcpy(buf, path, sizeof(buf));
    file = strrchr(buf, '/');
    if (!buf[0])
        return ENOTDIR;
    if (file == buf) {
        dir = root;
    }
    else {
        *file = '\0';
        dir = buf;
    }
    /*
     * Get the vnode for directory
     */
    if ((error = path_to_fatnode(dir, &dp, fmp)) != 0)
        return error;
    if (!IS_DIR(&dp.dirent)) {
        return ENOTDIR;
    }
    memcpy(np, &dp, sizeof(dp));

    /*
     * Get the file name
     */
    *name = strrchr(path, '/') + 1;
    return 0;
}
