/*
 * Copyright (c) 2005-2007, Kohsuke Ohtani
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

#ifndef _FATFS_V1_HEADER_
#define _FATFS_V1_HEADER_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "fs_media.h"

#ifndef DPRINTF
#define DPRINTF(a)
#endif
#define ASSERT(e)

#define SEC_SIZE    4096           /* sector size */
#define SEC_INVAL   0xffffffff  /* invalid sector */
#define EMPTY_BYTE_VAL 0x0
/*
 * Pre-defined cluster number
 */
#define CL_ROOT     0       /* cluster 0 means the root directory */
#define CL_FREE     0       /* cluster 0 also means the free cluster */
#define CL_FIRST    2       /* first legal cluster */
#define CL_LAST     0xfffffff5  /* last legal cluster */
#define CL_EOF      0xffffffff  /* EOF cluster */

#define EOF_MASK    0xfffffff8  /* mask of eof */

#define FAT12_MASK  0x00000fff
#define FAT16_MASK  0x0000ffff
#define FAT32_MASK  0x0fffffff

#pragma pack(push, 1)

/*
 * BIOS parameter block for FAT12/16.
 */
struct fat_bpb {
    uint16_t    jmp_instruction;
    uint8_t     nop_instruction;
    uint8_t     oem_id[8];
    uint16_t    bytes_per_sector;
    uint8_t     sectors_per_cluster;
    uint16_t    reserved_sectors;
    uint8_t     num_of_fats;
    uint16_t    root_entries;
    uint16_t    total_sectors;
    uint8_t     media_descriptor;
    uint16_t    sectors_per_fat;
    uint16_t    sectors_per_track;
    uint16_t    heads;
    uint32_t    hidden_sectors;
    uint32_t    big_total_sectors;
    uint8_t     physical_drive;
    uint8_t     reserved;
    uint8_t     ext_boot_signature;
    uint32_t    serial_no;
    uint8_t     volume_id[11];
    uint8_t     file_sys_id[8];
};

/*
 * BIOS parameter block for FAT32.
 */
struct fat32_bpb {
    uint16_t    jmp_instruction;        /* 0 ~ 1    : 0x00 ~ 0x01   */
    uint8_t     nop_instruction;        /* 2        : 0x02      */
    uint8_t     oem_id[8];              /* 3 ~ 10   : 0x03 ~ 0x0A   */
    uint16_t    bytes_per_sector;       /* 11 ~ 12  : 0x0B ~ 0x0C   */
    uint8_t     sectors_per_cluster;    /* 13       : 0x0D      */
    uint16_t    reserved_sectors;       /* 14 ~ 15  : 0x0E ~ 0x0F   */
    uint8_t     num_of_fats;              /* 16         : 0x10      */
    uint16_t    root_entries;             /* 17 ~ 18    : 0x11 ~ 0x12   */
    uint16_t    total_sectors;          /* 19 ~ 20  : 0x13 ~ 0x14   */
    uint8_t     media_descriptor;       /* 21       : 0x15      */
    uint16_t    sectors_per_fat;        /* 22 ~ 23  : 0x16 ~ 0x17   */
    uint16_t    sectors_per_track;    /* 24 ~ 25    : 0x18 ~ 0x19   */
    uint16_t    heads;                    /* 26 ~ 27    : 0x1A ~ 0x1B   */
    uint32_t    hidden_sectors;         /* 28 ~ 31  : 0x1C ~ 0x1F   */
    uint32_t    big_total_sectors;    /* 32 ~ 35    : 0x20 ~ 0x23   */
    
    uint32_t    sectors_per_fat32;    /* 36 ~ 39    : 0x24 ~ 0x27   */
    uint16_t    multi_fat32;              /* 40 ~ 41    : 0x28 ~ 0x29   */
    uint16_t    version;                  /* 42 ~ 43    : 0x2A ~ 0x2B   */
    uint32_t    root_clust;             /* 44 ~ 47  : 0x2C ~ 0x2F   */
    uint16_t    fsinfo;                   /* 48 ~ 49    : 0x30 ~ 0x31   */
    uint16_t    backup;                   /* 50 ~ 51    : 0x32 ~ 0x33   */
    uint8_t     reserved[12];             /* 52 ~ 63    : 0x34 ~ 0x3F   */
    
    uint8_t     physical_drive;         /* 64       : 0x40      */
    uint8_t     unused;                   /* 65         : 0x41      */
    uint8_t     ext_boot_signature;   /* 66         : 0x42      */
    uint32_t    serial_no;              /* 67 ~ 70  : 0x43 ~ 0x44   */
    uint8_t     volume_id[11];          /* 71 ~ 81  : 0x45 ~ 0x51   */
    uint8_t     file_sys_id[8];         /* 82 ~ 89  : 0x52 ~ 0x59   */
};

/*
 * FAT directory entry
 */
struct fat_dirent {
    uint8_t     name[11];     /* 0 - 10   : 0x00 ~ 0x0A       */
    uint8_t     attr;         /* 11       : 0x0B              */
    uint8_t     reserve;      /* 12       : 0x0C              */
    uint8_t     ctime_sec;    /* 13       : 0x0D              */
    uint16_t    ctime_hms;    /* 14 ~ 15  : 0x0E ~ 0x0F       */
    uint16_t    cday;         /* 16 ~ 17    : 0x10 ~ 0x11     */
    uint16_t    aday;         /* 18 ~ 19    : 0x12 ~ 0x13     */
    uint16_t    cluster_hi;   /* 20 ~ 21  : 0x14 ~ 0x15       */
    uint16_t    time;         /* 22 ~ 23    : 0x16 ~ 0x17     */
    uint16_t    date;         /* 24 ~ 25    : 0x18 ~ 0x19     */
    uint16_t    cluster;      /* 26 ~ 27  : 0x1A ~ 0x1B       */
    uint32_t    size;         /* 28 ~ 31    : 0x1C ~ 0x1F     */
};

#pragma pack(pop)

#define DE_CLUSTER(de)  (((de)->cluster_hi << 16) | (de)->cluster)

#define SLOT_EMPTY      0x00
#define SLOT_DELETED    0xe5

#define DIR_PER_SEC     (SEC_SIZE / sizeof(struct fat_dirent))

/*
 * FAT volume object.
 */
struct fatfs_vol {
    int fat_type;             /* 12, 16 or 32 */
    uint32_t    root_start;   /* start sector for root directory */
    uint32_t    fat_start;    /* start sector for fat entries */
    uint32_t    data_start;   /* start sector for data */
    uint32_t    fat_eof;      /* id of end cluster */
    uint32_t    sec_per_cl;   /* sectors per cluster */
    uint32_t    cluster_size; /* cluster size */
    uint32_t    last_cluster; /* last cluser */
    uint32_t    fat_mask;     /* mask for cluster# */
    uint32_t    free_scan;    /* start cluster# to free search */
    char    *io_buf;          /* local data buffer */
    char    *fat_buf;         /* buffer for fat entry */
    char    *dir_buf;         /* buffer for directory entry */
    fs_media_t* dev;          /* storage device */
    uint32_t    j_base_sec;
    uint32_t    j_capacity;
    uint32_t    j_start;
    uint32_t    j_end;
    uint32_t    j_timestamp;
    uint8_t     j_readonly;
};

#define FAT12(fat)  ((fat)->fat_type == 12)
#define FAT16(fat)  ((fat)->fat_type == 16)
#define FAT32(fat)  ((fat)->fat_type == 32)

#define IS_EOFCL(fat, cl) (((cl) & EOF_MASK) == ((fat)->fat_mask & EOF_MASK))

/* Macro to convert cluster# to logical sector# */
#define cl_to_sec(fat, cl) (fat->data_start + (cl - 2) * fat->sec_per_cl)

/*
 * File/directory node
 */
struct fatfs_node {
    struct fat_dirent dirent; /* copy of directory entry */
    uint32_t    sector;       /* sector# for directory entry */
    uint32_t    offset;       /* offset of directory entry in sector */
};

/*
 * FAT attribute for attr
 */
#define FA_RDONLY   0x01
#define FA_HIDDEN   0x02
#define FA_SYSTEM   0x04
#define FA_VOLID    0x08
#define FA_SUBDIR   0x10
#define FA_ARCH     0x20
#define FA_DEVICE   0x40

#define IS_DIR(de)  (((de)->attr) & FA_SUBDIR)
#define IS_VOL(de)  (((de)->attr) & FA_VOLID)
#define IS_FILE(de) (!IS_DIR(de) && !IS_VOL(de))

#define IS_DELETED(de)  ((de)->name[0] == 0xe5)
#define IS_EMPTY(de)    (((de)->name[0] == EMPTY_BYTE_VAL))

int  fat_next_cluster(struct fatfs_vol *fmp, uint32_t cl, uint32_t *next);
int  fat_set_cluster(struct fatfs_vol *fmp, uint32_t cl, uint32_t next);
int  fat_alloc_cluster(struct fatfs_vol *fmp, uint32_t scan_start, uint32_t *free);
int  fat_free_clusters(struct fatfs_vol *fmp, uint32_t start);
int  fat_seek_cluster(struct fatfs_vol *fmp, uint32_t start, uint32_t offset, uint32_t *cl);
int  fat_expand_file(struct fatfs_vol *fmp, uint32_t cl, int size);
int  fat_expand_dir(struct fatfs_vol *fmp, uint32_t cl, uint32_t *new_cl);

int fat_read_dirent(struct fatfs_vol *fmp, uint32_t sec);
int fat_write_dirent_direct(struct fatfs_vol *fmp, const struct fat_dirent* de, uint32_t sec, uint32_t offset);
int fat_write_dirent_deferred(struct fatfs_vol *fmp, const struct fat_dirent* de, uint32_t sec, uint32_t offset);
int fat_empty_dirents_direct(struct fatfs_vol *fmp, uint32_t cl);
int fat_empty_dirents_deferred(struct fatfs_vol *fmp, uint32_t cl);
int write_fat_entry_direct(struct fatfs_vol *fmp, uint32_t cl, uint32_t offset, uint32_t val);
int write_fat_entry_deferred(struct fatfs_vol *fmp, uint32_t cl, uint32_t offset, uint32_t val);

void fat_convert_name(char *org, char *name);
void fat_restore_name(char *org, char *name);
int  fat_valid_name(char *name);
int  fat_compare_name(char *n1, char *n2);

int  fatfs_lookup_node(struct fatfs_vol * dvp, char *name, struct fatfs_node *node, uint32_t cl);
int  fatfs_get_node(struct fatfs_vol * dvp, int index, struct fatfs_node *node, uint32_t cl);
int  fatfs_put_node(struct fatfs_vol *fmp, struct fatfs_node *node);
int  fatfs_add_node(struct fatfs_vol * dvp, struct fatfs_node *node, uint32_t cl);

#ifdef FATFS_JOURNALING
#define write_fat_entry write_fat_entry_deferred
#define fat_write_dirent fat_write_dirent_deferred
#define fat_empty_dirents fat_empty_dirents_deferred
#define fatfs_commit fatfs_commit_journal
#define fatfs_mkjournal fatfs_create_journal
#define fatfs_chk fatfs_check
#else
#define write_fat_entry write_fat_entry_direct
#define fat_write_dirent fat_write_dirent_direct
#define fat_empty_dirents fat_empty_dirents_direct
#define fatfs_commit(fmp, err)
#define fatfs_mkjournal(dev, buf) 0
#define fatfs_chk(vol) 0
#endif

extern int erase_sectors(fs_media_t* dev, uint32_t base_sec, uint32_t count, void* buf);

int fatfs_create_journal(fs_media_t* dev, void* bpb_buf);
void fatfs_commit_journal(struct fatfs_vol *fmp, int error);
int fatfs_check(struct fatfs_vol* fmp);

/*
 * Low level FAT module interface.
 */
int fatfs_format(fs_media_t* dev, uint32_t nblk, void* sec_buf);
int fatfs_init(struct fatfs_vol *fmp, fs_media_t* dev, uint32_t base, void*(*alloc)(size_t));
int fatfs_get_node_by_path(struct fatfs_vol *fmp, char *path, struct fatfs_node* np, char **name);

int fatfs_alloc(struct fatfs_vol *fmp, struct fatfs_node *np, size_t size);
int fatfs_lookup(struct fatfs_vol *fmp, struct fatfs_node *dp, char *name, struct fatfs_node *np);
int fatfs_read(struct fatfs_vol *fmp, struct fatfs_node *np, uint32_t* f_offset, void *buf, size_t size, size_t *result);

int fatfs_write(struct fatfs_vol *fmp, struct fatfs_node *np, uint32_t* f_offset, void *buf, size_t size, size_t *result, int append);
int fatfs_create(struct fatfs_vol *fmp, struct fatfs_node *np, char *name, uint8_t attr);
int fatfs_remove(struct fatfs_vol *fmp, struct fatfs_node *np, char *name);
int fatfs_rename(struct fatfs_vol *fmp, struct fatfs_node *dp1, char *name1, struct fatfs_node *dp2, char *name2);
int fatfs_mkdir(struct fatfs_vol *fmp, struct fatfs_node *dp, char *name, uint32_t attr);
int fatfs_rmdir(struct fatfs_vol *fmp, struct fatfs_node *dp, char *name);
int fatfs_truncate(struct fatfs_vol *fmp, struct fatfs_node *np, uint32_t length);

#endif /* !_FATFS_H */
