/** 
  ******************************************************************************
  *  @file   fatfs_journal.c
  *  @brief  FAT journaling extension. 
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2020.
  *  $$LICENSE$
  *****************************************************************************/

#include <errno.h>
#include <string.h>
#include "fatfs.h"

/* Global constants.
 * JENTRY_PER_SEC * JENTRY_SZ + JHEADER_SZ must be equal to SEC_SIZE.
 */
enum
{
    JENTRY_SZ = 48,         /* Journal entry size in bytes. */
    JHEADER_SZ = 32,        /* Sector header size in bytes. */
    JENTRY_PER_SEC = (SEC_SIZE - JHEADER_SZ) / JENTRY_SZ,
    JOURNAL_PART = 10,
    JOURNAL_CL = 3,

    FATFS_NOOP = 0,         /* FS ops codes must be nonzero. */
    FATFS_OP_UPDATE_ENTRY,
    FATFS_OP_FILL_DIRENTS,
    FATFS_OP_UPDATE_DIRENT,
    FATFS_MARKER_DONE,
    FATFS_MARKER_COMMIT,
};

int fat_read_bpb(
    struct fatfs_vol *fmp, 
    uint8_t* bpb_buf
);

int fat_lookup_dirent(
    struct fatfs_vol *fmp, 
    uint32_t sec, 
    char *name, 
    struct fatfs_node *np
);

/* Journal entry. Op determines which data is stored in the union.
 * Each physical sector contains set of entries preceded by sector header.
 */
struct jentry
{
    uint32_t op;

    union
    {
        struct 
        {
            uint32_t cl;
            uint32_t offset;
            uint32_t val;
        }
        fat;

        struct 
        {
            uint32_t sector;
            uint32_t offset;
            struct fat_dirent de;
        }
        dirent;

        struct
        {
            uint32_t cl;
        }
        fill;
    }
    data;
};

struct jheader
{
    uint32_t timestamp;
};

static struct jentry*
journal_entry(void* sec_buf, unsigned int i)
{
    char* buf = sec_buf;
    return (struct jentry*) (buf + i * JENTRY_SZ + JHEADER_SZ);
}

static uint32_t
sector_timestamp(void* sec_buf)
{
    struct jheader* header = sec_buf;
    return header->timestamp;
}

static uint32_t 
next_slot(struct fatfs_vol* fmp, uint32_t slot)
{
    return (slot + 1) % fmp->j_capacity;
}

/* Perform last uncommitted transaction by journal.*/
static int 
tx_perform(struct fatfs_vol* fmp)
{
    int i = 0;
    int error = 0;
    
    for (i = fmp->j_start; i != fmp->j_end; i = next_slot(fmp, i))
    {
        size_t size = SEC_SIZE;
        const uint32_t sec = i / JENTRY_PER_SEC;
        const uint32_t index = i % JENTRY_PER_SEC;

        error = fmp->dev->read(
            fmp->dev, 
            fmp->io_buf, 
            &size, 
            fmp->j_base_sec + sec
        );

        if (!error)
        {
            const struct jentry* e = journal_entry(fmp->io_buf, index);

            switch (e->op)
            {
            case FATFS_NOOP: 
            case FATFS_MARKER_DONE:
            case FATFS_MARKER_COMMIT:
                break;

            case FATFS_OP_UPDATE_ENTRY:
                error = write_fat_entry_direct(
                    fmp, 
                    e->data.fat.cl, 
                    e->data.fat.offset, 
                    e->data.fat.val
                );
                break;

            case FATFS_OP_FILL_DIRENTS:
                error = fat_empty_dirents_direct(fmp, e->data.fill.cl);
                break;

            case FATFS_OP_UPDATE_DIRENT:
                error = fat_write_dirent_direct(
                    fmp, &e->data.dirent.de, 
                    e->data.dirent.sector, 
                    e->data.dirent.offset
                );
                break;
            }
        }

        if (error)
        {
            break;
        }
    }

    return error;
}

/* Write next journal entry to the storage media.
 * When write boundary become aligned with logical sector then
 * sector timestamp is automatically emitted.
 */
static int 
tx_emit(struct fatfs_vol* fmp, const struct jentry *e)
{
    const uint32_t tail = next_slot(fmp, fmp->j_end);
    const uint32_t sec = tail / JENTRY_PER_SEC;
    const uint32_t index = tail % JENTRY_PER_SEC;
    size_t sz = SEC_SIZE;
    int err = 0;

    fmp->j_end = tail;

    err = fmp->dev->read(fmp->dev, fmp->io_buf, &sz, fmp->j_base_sec + sec);

    if (!err)
    {
        if (index == 0)
        {
            struct jheader* header = (struct jheader*) fmp->io_buf;
            memset(fmp->io_buf, 0, SEC_SIZE);
            header->timestamp = ++fmp->j_timestamp;
        }

        memcpy(journal_entry(fmp->io_buf, index), e, sizeof(*e));
        err = fmp->dev->write(fmp->dev, fmp->io_buf, &sz, fmp->j_base_sec+sec);
    }

    if (e->op == FATFS_MARKER_COMMIT)
    {
        fmp->j_start = fmp->j_end;
    }

    return err;
}

/* Append next entry to the journal.*/
static int 
fatfs_append_journal(struct fatfs_vol* fmp, const struct jentry* e)
{
    const bool last_slot = next_slot(fmp, fmp->j_end) == fmp->j_start;

    /* Forcibly abort active transaction in case when journal overflow
     * occurs. This may happen when the user writes too many data as a
     * single transaction. By default, maximum amount of data which may
     * be written by signel write API call is 1/10 of drive capacity.
     */
    if (last_slot && (e->op != FATFS_MARKER_COMMIT))
    {
        return EBUSY;
    }

    /* In order to avoid filesystem corruption, in case when any 
     * failures occur when updating the journal, FS is marked as 
     * readonly. This is single-way operation. FS may be made 
     * writable only after reinit and checking for errors.
     */
    if (fmp->j_readonly)
    {
        return EIO;
    }
    
    return tx_emit(fmp, e);
}

/* Perform and commit last transaction.*/
void 
fatfs_commit_journal(struct fatfs_vol* fmp, int status)
{
    int error = 0;
   
    do
    {
        struct jentry e = { FATFS_MARKER_DONE };

        if (!status)
        {
            error = tx_emit(fmp, &e);
        
            if (error)
            {
                break;
            }

            error = tx_perform(fmp);

            if (error)
            {
                break;
            }
        }

        e.op = FATFS_MARKER_COMMIT;
        error = tx_emit(fmp, &e);
    }
    while (0);

    if (error)
    {
        fmp->j_readonly = 1;
    }
}

int
fat_write_dirent_deferred(
    struct fatfs_vol *fmp, 
    const struct fat_dirent* de, 
    uint32_t sec, 
    uint32_t offset)
{
    struct jentry e = { FATFS_OP_UPDATE_DIRENT };
    e.data.dirent.de = *de;
    e.data.dirent.sector = sec;
    e.data.dirent.offset = offset;

    return fatfs_append_journal(fmp, &e);
}

int
fat_empty_dirents_deferred(struct fatfs_vol *fmp, uint32_t cl)
{
    struct jentry e = { FATFS_OP_FILL_DIRENTS };
    e.data.fill.cl = cl;

    return fatfs_append_journal(fmp, &e);
}

int
write_fat_entry_deferred(
    struct fatfs_vol *fmp, 
        uint32_t cl, 
        uint32_t offset, 
        uint32_t val)
{
    struct jentry e = { FATFS_OP_UPDATE_ENTRY };
    e.data.fat.cl = cl;
    e.data.fat.offset = offset;
    e.data.fat.val = val;

    return fatfs_append_journal(fmp, &e);
}

/* Finds sector containing last valid timestamp. */
static int 
find_last_timestamp(struct fatfs_vol* fmp, uint32_t* sec)
{
    size_t size = SEC_SIZE;
    const uint32_t base = fmp->j_base_sec;
    uint32_t l = 0;
    uint32_t r = (fmp->j_capacity / JENTRY_PER_SEC) - 1;

    /* Read journal sector with highest number. */
    int error = fmp->dev->read(fmp->dev, fmp->io_buf, &size, r + base);

    /* Journal is overwritten at least once if last slot is not zeroed. */
    const uint32_t wrap = journal_entry(fmp->io_buf, JENTRY_PER_SEC - 1)->op;

    if (!error)
    {
        while (r - l > 1)
        {
            const uint32_t m = (l + r) / 2;
            uint32_t M, L, V;
            int left;

            error = fmp->dev->read(fmp->dev, fmp->io_buf, &size, m + base);

            if (error)
            {
                break;
            }

            M = sector_timestamp(fmp->io_buf);
            V = journal_entry(fmp->io_buf, 0)->op;

            error = fmp->dev->read(fmp->dev, fmp->io_buf, &size, l + base);

            if (error)
            {
                break;
            }

            L = sector_timestamp(fmp->io_buf);

            left = wrap ? (M != (L + m - l)) : (V == 0);

            if (left)
            {
                r = m;
            }
            else
            {
                l = m;
            }
        }
    }

    *sec = l;
    fmp->j_timestamp = sector_timestamp(fmp->io_buf);

    return error;
}

/* Finds last journal entry in given sector. */
static int 
find_last_tx(struct fatfs_vol* fmp, uint32_t s, uint32_t* last_op)
{
    int i = 0;
    size_t sz = SEC_SIZE;
    int error = fmp->dev->read(fmp->dev, fmp->io_buf, &sz, fmp->j_base_sec + s);

    if (!error)
    {
        for (i = JENTRY_PER_SEC - 1; i >= 0; i--)
        {
            const struct jentry* e = journal_entry(fmp->io_buf, i);

            if (e->op)
            {
                fmp->j_end = s*JENTRY_PER_SEC + i;
                fmp->j_start = fmp->j_end;
                *last_op = e->op;
                break;
            }
        }
    }

    if (i < 0)
    {
        error = EBUSY;
    }

    return error;
}

/* Finds start of transaction marked by j_end. */
static int 
find_tx_start(struct fatfs_vol* fmp)
{
    uint32_t sz = SEC_SIZE;
    uint32_t index = fmp->j_end;
    uint32_t sec_remain = fmp->j_capacity / JENTRY_PER_SEC;
    int found = 0;
    int error = 0;

    while (!found && sec_remain--)
    {
        int i = 0;
        uint32_t s = index / JENTRY_PER_SEC;
        uint32_t offset = index % JENTRY_PER_SEC;
        
        error = fmp->dev->read(fmp->dev, fmp->io_buf, &sz, s + fmp->j_base_sec);

        if (error)
        {
            break;
        }

        for (i = offset; i >= 0; i--, index--)
        {
            const struct jentry* e = journal_entry(fmp->io_buf, i);

            if (e->op == FATFS_MARKER_COMMIT)
            {
                fmp->j_start = i;
                found = 1;
                break;
            }
        }

        if (index < 0)
        {
            index = fmp->j_capacity - 1;
        }
    }

    if (!found)
    {
        error = EBUSY;
    }

    return error;
}

/* Try find journal file in root directory. It must be first entry. */
int
fatfs_lookup_journal_node(
    struct fatfs_vol * dvp, 
    char *fat_name, 
    struct fatfs_node *np, 
    uint32_t cl)
{
    struct fatfs_vol *fmp = dvp;
    uint32_t sec;
    int error;

    if (cl == CL_ROOT && !(FAT32(fmp)) ) 
    {
        sec = fmp->root_start;
        error = fat_lookup_dirent(fmp, sec, fat_name, np);
        return error;
    } 
    else 
    {
        if(cl == CL_ROOT) 
        {
            cl = fmp->root_start;
            sec = cl_to_sec(fmp, cl);
            error = fat_lookup_dirent(fmp, sec, fat_name, np);
            return error;
        }
    }
    return ENOENT;
}

/* Checks filesystem for consistency. */
int 
fatfs_check(struct fatfs_vol* fmp)
{
    int error = 0;

    do
    {
        struct fatfs_node j = {0};
        const struct jentry commit = { FATFS_MARKER_COMMIT };
        uint32_t last_sec = 0; 
        uint32_t last_code = 0;

        /* Lookup journal file. */
        error = fatfs_lookup_journal_node(fmp, "/          ", &j, 0);

        if (error)
        {
            break;
        }

        fmp->j_base_sec = cl_to_sec(fmp, DE_CLUSTER(&j.dirent));
        fmp->j_capacity = (j.dirent.size / SEC_SIZE) * JENTRY_PER_SEC;

        /* Try find sector with latest timestamp. */
        error = find_last_timestamp(fmp, &last_sec);

        if (error)
        {
            break;
        }

        /* Try find latest written journal slot. */
        error = find_last_tx(fmp, last_sec, &last_code);

        if (error)
        {
            break;
        }

        /*
         * There are three possible states of the journal:
         * 1) Last transaction had been successfully committed.
         * 2) Transaction is not fully emitted.
         * 3) Last transaction fully emitted but not committed.
         */

        /* Case 1: there are nothing to do, FS is consistent. */
        if (last_code == FATFS_MARKER_COMMIT)
        {
            break;
        }

        /* Case 2: if there is no "done" marker it means that
         * transaction log is not fully populated. Consequently,
         * there are no changes in FAT data structures.
         * Just break incomplete transaction with COMMIT marker.
         */
        if (last_code != FATFS_MARKER_DONE)
        {
            error = tx_emit(fmp, &commit);
            break;
        }

        /*
         * Case 3 is the most complicated one.
         * Transaction is fully written to log, some changes may be
         * written to FAT table and directory entries and then
         * process is interrupted before COMMIT marker is written.
         * In this case we have to find transaction start and then 
         * redo actions in log.
         */
        error = find_tx_start(fmp);

        if (error)
        {
            break;
        }

        error = tx_perform(fmp);

        if (error)
        {
            break;
        }

        error = tx_emit(fmp, &commit);
    }
    while (0);

    fmp->j_readonly = (error != 0);

    return error;
}

/*
 * This function updates first dirent in root folder.
 */
static int
fatfs_insert_dirent(
    struct fatfs_vol *fmp, 
    uint32_t size, 
    char name, 
    uint8_t attr)
{
    struct fat_dirent de;
    uint32_t offset, sec, cl = CL_FIRST + 1;
    int error;

    memset(&de, 0, sizeof(struct fat_dirent));
    memset((char *)de.name, ' ', 11);
    de.name[0] = name;
    de.cluster_hi = (cl & 0xFFFF0000) >> 16;
    de.cluster = cl & 0x0000FFFF;
    de.time = 0;
    de.date = 0;
    de.attr = attr;
    de.size = size;

    if (!(FAT32(fmp))) 
    {
        sec = fmp->root_start;
        if (FAT16(fmp))
            offset = (cl * 2) % SEC_SIZE;
        else
            offset = (cl * 3 / 2) % SEC_SIZE;
    } 
    else 
    {
        sec = cl_to_sec(fmp, fmp->root_start);
        offset = (cl * 4) % SEC_SIZE;
    }

    error = fat_write_dirent_direct(fmp, &de, sec, 0);

    if (!error)
    {
        error = write_fat_entry_direct(fmp, cl, offset, fmp->fat_eof);
    }
    
    return error;
}

/*
 * This function creates journal file on empty media.
 * N.B. It is assumed that the disk is empty.
 * The buffer parameter should hold BPB populated from format/mkfs function.
 */
int 
fatfs_create_journal(fs_media_t* dev, void* buf)
{
    unsigned int i = 0;
    uint32_t journal_clusters = 0;
    
    /* Since the journal is a regular file we use temporary mountpoint 
     * in order to reuse existing infrastructure for creating files.
     * However, this moutpoint is not completely initialized and should
     * not be used for anything except file creation.
     */
    struct fatfs_vol temp_mp;
    int err = fat_read_bpb(&temp_mp, buf);

    /* File creation uses no more than one sector buffer. */
    temp_mp.dev = dev;
    temp_mp.dir_buf = buf;
    temp_mp.fat_buf = buf;
    temp_mp.io_buf = buf;

    journal_clusters = 
        ((temp_mp.last_cluster / JOURNAL_PART) / JENTRY_PER_SEC) / 
            (temp_mp.cluster_size / SEC_SIZE);

    for (i = 0; i < journal_clusters; ++i)
    {
        const uint32_t cl = i + JOURNAL_CL;
        uint32_t offset = 0;

        if(FAT32(&temp_mp))
        {
            offset = (cl * 4) % SEC_SIZE;
        }
        else
        {
            if (FAT16(&temp_mp))
                offset = (cl * 2) % SEC_SIZE;
            else
                offset = (cl * 3 / 2) % SEC_SIZE;
        }

        write_fat_entry_direct(&temp_mp, cl, offset, 0xfffffff7);
    }

    err = fatfs_insert_dirent(
        &temp_mp, 
        (temp_mp.sec_per_cl * SEC_SIZE) * journal_clusters, 
        '/', 
        FA_HIDDEN | FA_SYSTEM
    );

    /* Fill journal with zeros and emit commit mark to indicate that no
     * pending transactions are active.
     */
    memset(buf, 0, SEC_SIZE);

    for (i = 0; i < temp_mp.sec_per_cl * journal_clusters; ++i)
    {
        size_t size = SEC_SIZE;
        const uint32_t sec = cl_to_sec((&temp_mp), CL_FIRST + 1);
        journal_entry(buf, 0)->op = (i == 0) ? FATFS_MARKER_COMMIT : FATFS_NOOP;
        err = dev->write(dev, buf, &size, sec + i);
    }

    return err;
}
