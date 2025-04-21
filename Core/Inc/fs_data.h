#ifndef FS_DATA_H
#define FS_DATA_H

#define MAX_ITEMS 100
#define MAX_NAME_LENGTH 64

typedef enum {
    FS_TYPE_FILE,
    FS_TYPE_FOLDER
} FsItemType;

typedef struct {
    char name[MAX_NAME_LENGTH];
    FsItemType type;
} FsItem;

extern FsItem fs_items[MAX_ITEMS];
extern int fs_item_count;

void add_fs_item(const char* name, FsItemType type);

#endif
