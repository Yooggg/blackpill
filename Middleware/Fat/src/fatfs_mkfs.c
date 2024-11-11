/** 
  ******************************************************************************
  *  @file   fatfs_mkfs.c
  *  @brief  FAT partition format. 
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2020.
  *  $$LICENSE$
  *****************************************************************************/

#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include "fatfs.h"

/*
 * Global constants.
 */
enum
{
    MEDIA_DISK = 0xf8,
    RESERVED_CLUSTERS = 2,
    ROOTDIR_SIZE = 16384,
    FSINFO_SEC = 1,
    BACKUP_SEC = 6,
    BITS_PER_SECTOR = SEC_SIZE * 8,
    FAT_COPY = 2,
    FAT16_CL_PER_SEC = SEC_SIZE / 2,
    FAT32_CL_PER_SEC = SEC_SIZE / 4,
    FAT_MIN_SECTORS = 342,
    FAT_MAX_SECTORS = 134217728,
    FSINFO_LEAD_SIG_VAL = 0x41615252,
    FSINFO_STRUC_SIG_VAL = 0x61417272,
    FSINFO_LEAD_SIG = 0,
    FSINFO_STRUC_SIG = 0x1e4 / 4,
    FSINFO_FREE_COUNT = 0x1e8 / 4,
    FSINFO_NEXT_FREE = 0x1ec / 4,
};

/*
 * FAT layout description.
 */
typedef struct
{
    unsigned int fat_type;
    uint32_t clusters_num;
    size_t log2_sec_per_cluster;
    size_t sec_per_fat;
}
fatfs_layout_t;

/*
 * Round given size in bytes to sectors number.
 */
static inline size_t 
round_sectors(size_t bytes)
{
    return (bytes + SEC_SIZE - 1) / SEC_SIZE;
}

/*
 * Copies "count" sectors from source to destination.
 */
static int 
copy_sectors(
    fs_media_t* dev, 
    uint32_t from, 
    uint32_t to, 
    uint32_t count, 
    void* buf)
{
    uint32_t i;
    uint32_t len = SEC_SIZE;
    int error;

    for (i = 0; i < count; ++i)
    {
        error = dev->read(dev, buf, &len, from);

        if (error)
        {
            return EIO;
        }

        error = dev->write(dev, buf, &len, to);

        if (error)
        {
            return EIO;
        }

        ++from;
        ++to;
    }
    return 0;
}

/*
 * Fills sectors with zeros.
 */
extern int 
erase_sectors(fs_media_t* dev, uint32_t base_sec, uint32_t count, void* buf)
{
    uint32_t i;
    uint32_t len = SEC_SIZE;

    memset(buf, 0, SEC_SIZE);

    for (i = 0; i < count; ++i)
    {
        if (dev->write(dev, buf, &len, base_sec + i))
        {
            return EIO;
        }
    }

    return 0;
}

/*
 * In current implementation FAT partitions may contain from 
 * 342 to 2097152 512-byte sectors. This results in partition sizes from 
 * ~170Kb to 64Gb.
 * FAT32 uses 32 reserved sectors at head of the partition, FAT12/16 use 1.
 */
static inline int 
calculate_layout(fatfs_layout_t* layout, uint32_t sect_num)
{
    uint32_t reserved_sect = 1;
    uint32_t clusters_max;
    uint32_t clusters_num = RESERVED_CLUSTERS;
    uint32_t fat_bytes;
    uint32_t rootdir_sect;
    uint32_t total_data_sect = 0;

    if (sect_num < FAT_MIN_SECTORS || sect_num > FAT_MAX_SECTORS)
    {
        return EINVAL;
    }

    layout->log2_sec_per_cluster = 0;

    if (sect_num < 8400)
    {
        layout->fat_type = 12;
        clusters_max = 1 << 12;
    }
    else if (sect_num < 1048576)
    {
        layout->fat_type = 16;
        clusters_max = 1 << 16;
    }
    else
    {
        layout->fat_type = 32;
        clusters_max = 2097152;
        reserved_sect = 32;
    }

    /*
     * Keep cluster size below 32Kb, so, log2_sec_per_cluster must be < 6.
     */
    while ( layout->log2_sec_per_cluster < 6 && 
            sect_num >> layout->log2_sec_per_cluster > clusters_max)
    {
        ++layout->log2_sec_per_cluster;
    }

    clusters_num += (sect_num - reserved_sect) >> layout->log2_sec_per_cluster;
    rootdir_sect = FAT32(layout) ? 0 : round_sectors(ROOTDIR_SIZE);
    
    if (FAT12(layout))
    {
        fat_bytes = (clusters_num * 3 + 1) >> 1;
    }
    else if (FAT16(layout))
    {
        fat_bytes = clusters_num << 1;
    }
    else
    {
        fat_bytes = clusters_num << 2;
    }

    layout->sec_per_fat = round_sectors(fat_bytes);

    total_data_sect = 
        sect_num - reserved_sect - rootdir_sect - layout->sec_per_fat*FAT_COPY;

    layout->clusters_num = RESERVED_CLUSTERS + 
        (total_data_sect >> layout->log2_sec_per_cluster);

    return 0;
}

/*
 * Write BIOS Parameter Block (BPB).
 */
static inline int 
setup_bpb(
    fs_media_t* dev, 
    uint32_t sect_num, 
    const fatfs_layout_t* layout, 
    void* sec_buf)
{
    uint8_t* bytes = sec_buf;
    struct fat_bpb* bpb = sec_buf;
    uint32_t len = SEC_SIZE;

    memset(bytes, 0, SEC_SIZE);

    bpb->jmp_instruction = 0xfeeb;
    bpb->nop_instruction = 0x90;
    memcpy(bpb->oem_id, "JFAT    ", 11);
    bpb->bytes_per_sector = SEC_SIZE;
    bpb->sectors_per_cluster = 1 << layout->log2_sec_per_cluster;
    bpb->reserved_sectors = FAT32(layout) ? 32 : 1;
    bpb->num_of_fats = FAT_COPY;
    bpb->root_entries = !FAT32(layout) ? 
        ROOTDIR_SIZE / sizeof(struct fat_dirent) : 
        0;
    bpb->total_sectors = !FAT32(layout) ? (sect_num & 0xffff) : 0;
    bpb->media_descriptor = MEDIA_DISK;

    if (FAT32(layout))
    {
         struct fat32_bpb* bpb32 = sec_buf;
         bpb32->big_total_sectors = sect_num;
         bpb32->sectors_per_fat32 = layout->sec_per_fat;
         bpb32->root_clust = CL_FIRST;
         bpb32->fsinfo = FSINFO_SEC;
         bpb32->backup = BACKUP_SEC;
    }
    else
    {
        bpb->sectors_per_fat = (uint16_t) layout->sec_per_fat;
        bpb->ext_boot_signature = 0x29;
        memset(bpb->volume_id, ' ', sizeof(bpb->volume_id));
    }

	bytes[0x1fe] = 0x55;
	bytes[0x1ff] = 0xaa;
    return dev->write(dev, bytes, &len, 0);
}

/*
 * Set FAT12 cluster to FF7 value from given bit offset.
 */
static inline void 
fat12_cl_set(uint8_t* buf, int bit_offset)
{
    const int byte = bit_offset / 8;

    if ((bit_offset % 8) == 0)
    {
        buf[byte] = 0xf7;
        buf[byte + 1] &= 0xf0;
        buf[byte + 1] |= 0x0f;
    }
    else
    {
        buf[byte] &= 0x0f;
        buf[byte] |= 0x70;
        buf[byte + 1] = 0xff;
    }
}

/*
 * Init FAT12 table. Clusters beyond last valid one are marked as
 * invalid (ff7 code).
 */
static int 
init_fat12(fs_media_t* dev, const fatfs_layout_t* layout, void* sec_buf)
{
    uint8_t* buf = sec_buf;
    uint8_t i = 0;
    uint32_t len = SEC_SIZE;
    uint16_t j = 0;
    const uint16_t last_fat_sec = (layout->clusters_num*12) / BITS_PER_SECTOR;
    int error = 0;

    for (i = 0; i < layout->sec_per_fat; ++i)
    {
        memset(buf, 0, SEC_SIZE);

        /* First two clusters are reserved for media type code. */
        if (i == 0)
        {
            buf[0] = MEDIA_DISK;
            buf[1] = 0x8f;
            buf[2] = 0xff;
        }

        /* Note: this may set byte beyond current sector since 512 % 12 != 0.*/
        if (i >= last_fat_sec)
        {
            const uint16_t offset = (i == last_fat_sec) ? 
                (layout->clusters_num * 12) % BITS_PER_SECTOR : 
                j % BITS_PER_SECTOR;

            /* From previous iteration. */
            if (i > last_fat_sec)
            {
                buf[0] = buf[SEC_SIZE];
            }

            for (j = offset; j < BITS_PER_SECTOR; j += 12)
            {
                fat12_cl_set(buf, j);
            }
        }

        error = dev->sector_erase(dev,i + 1);

        if (error)
        {
            break;
        }
    }
    
    if (!error)
    {
        error = copy_sectors(
            dev, 1, layout->sec_per_fat + 1, layout->sec_per_fat, buf
        );
    }

    if (!error)
    {
        error = erase_sectors(
            dev, 
            1 + layout->sec_per_fat * FAT_COPY, 
            ROOTDIR_SIZE / SEC_SIZE, 
            buf
        );
    }

    return error;
}

/*
 * Init FAT16 table. Clusters beyond last valid one are marked as
 * invalid (fff7 code).
 */
static int 
init_fat16(fs_media_t* dev, const fatfs_layout_t* layout, void* sec_buf)
{
    uint16_t* buf = sec_buf;
    uint32_t i = 0;
    uint32_t len = SEC_SIZE;
    const uint32_t last_fat_sec = layout->clusters_num / FAT16_CL_PER_SEC;
    int error = 0;

    for (i = 0; i < layout->sec_per_fat; ++i)
    {
        memset(buf, 0, SEC_SIZE);

        if (i == 0)
        {
            buf[0] = 0xff00 | MEDIA_DISK;
            buf[1] = 0xfff8;
        }

        if (i >= last_fat_sec)
        {
            const uint16_t offset = last_fat_sec ? 
                layout->clusters_num % FAT16_CL_PER_SEC : 
                0;
            uint16_t j = 0;

            for (j = offset; j < FAT16_CL_PER_SEC; j++)
            {
                buf[j] = 0xfff7;
            }
        }

        error = dev->write(dev, buf, &len, i + 1);

        if (error)
        {
            break;
        }
    }

    if (!error)
    {
        error = copy_sectors(
            dev, 1, layout->sec_per_fat + 1, layout->sec_per_fat, buf
        );
    }

    if (!error)
    {
        error = erase_sectors(
            dev, 
            1 + layout->sec_per_fat * FAT_COPY, 
            ROOTDIR_SIZE / SEC_SIZE, 
            buf
        );
    }

    return error;
}

/*
 * Init FAT32 table. Clusters beyond last valid one are marked as
 * invalid (fffffff7 code).
 */
static int 
init_fat32(fs_media_t* dev, const fatfs_layout_t* layout, void* sec_buf)
{
    uint32_t* buf = sec_buf;
    uint32_t i = 0;
    uint32_t len = SEC_SIZE;
    const uint32_t last_fat_sec = layout->clusters_num / FAT32_CL_PER_SEC;
    int error = 0;

    for (i = 0; i < layout->sec_per_fat; ++i)
    {
        memset(buf, 0, SEC_SIZE);

        if (i == 0)
        {
            buf[0] = 0xffffff00 | MEDIA_DISK;
            buf[1] = 0xfffffff8;
            buf[2] = 0xfffffff8;
        }

        if (i >= last_fat_sec)
        {
            const uint32_t offset = (i == last_fat_sec) ?
                layout->clusters_num % FAT32_CL_PER_SEC : 
                0;
            uint32_t j = 0;

            for (j = offset; j < FAT32_CL_PER_SEC; j++)
            {
                buf[j] = 0xfffffff7;
            }
        }

        error = dev->write(dev, buf, &len, i + 32);

        if (error)
        {
            break;
        }
    }

    if (!error)
    {
        error = copy_sectors(
            dev, 32, layout->sec_per_fat + 32, layout->sec_per_fat, buf
        );
    }

    if (!error)
    {
        error = erase_sectors(
            dev, 
            32 + layout->sec_per_fat * FAT_COPY, 
            1 << layout->log2_sec_per_cluster,
            buf
        );
    }

    if (!error)
    {
        memset(buf, 0, SEC_SIZE);

        buf[FSINFO_LEAD_SIG] = FSINFO_LEAD_SIG_VAL;
        buf[FSINFO_STRUC_SIG] = FSINFO_STRUC_SIG_VAL;
        buf[FSINFO_FREE_COUNT] = 0xffffffff;
        buf[FSINFO_NEXT_FREE] = 2;
        buf[0x1fc / 4] = 0xaa550000;

        error = dev->write(dev, buf, &len, FSINFO_SEC);

        if (!error)
        {
            error = dev->write(dev, buf, &len, BACKUP_SEC);
        }
    }

    return error;
}

/*
 * Format partition. FAT type is determined by clusters number.
 */
int 
fatfs_format(fs_media_t* dev, uint32_t nblk, void* sec_buf)
{
    fatfs_layout_t layout;
    int error = calculate_layout(&layout, nblk);

    memset(sec_buf, 0, SEC_SIZE + 1);
    
    if (!error)
    {
        if (FAT12(&layout))
        {
            error = init_fat12(dev, &layout, sec_buf);
        }
        else if (FAT16(&layout))
        {
            error = init_fat16(dev, &layout, sec_buf);
        }
        else
        {
            error = init_fat32(dev, &layout, sec_buf);
        }

        if (!error)
        {
            error = setup_bpb(dev, nblk, &layout, sec_buf);
        }

        if (!error)
        {
            error = fatfs_mkjournal(dev, sec_buf);
        }
    }

    return error;
}
