#ifndef _FS_MEDIA_V1_HEADER_
#define _FS_MEDIA_V1_HEADER_

/** 
  ****************************************************************************************************
  *  @file   fs_media.h
  *  @brief  Thin base class for FS media abstraction.
  *
  ****************************************************************************************************
  *  Copyright (C) 2008-2019 Eremex
  *  All rights reserved.
  ***************************************************************************************************/

#include <stddef.h>
#include <stdint.h>

/*! @en
 * Media representation.
 */
typedef struct _fs_media_t
{
    int (*read)(struct _fs_media_t*, void*, uint32_t*, uint32_t);         //!< @en Device read function.
    int (*write)(struct _fs_media_t*, void*, uint32_t*, uint32_t);        //!< @en Device write function.
    int (*sector_erase)(struct _fs_media_t*, void*, uint32_t*, uint32_t); //!< @en Device erase sectors function.
    //int sec_size;                                                       //!< @en Device sectors count.
}
fs_media_t;

/* Please provide this functions if needed */
void fs_media_lock(fs_media_t* media);
void fs_media_unlock(fs_media_t* media);

#endif
