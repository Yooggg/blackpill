#include <fs_data.h>
#include <string.h>

FsItem fs_items[MAX_ITEMS];
int fs_item_count = 0;

void add_fs_item(const char* name, FsItemType type) {
    if (fs_item_count < MAX_ITEMS) {
        strncpy(fs_items[fs_item_count].name, name, MAX_NAME_LENGTH);
        fs_items[fs_item_count].name[MAX_NAME_LENGTH - 1] = '\0';
        fs_items[fs_item_count].type = type;
        fs_item_count++;
    }
}
