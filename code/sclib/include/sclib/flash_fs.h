#ifndef _SC_FLASH_FS_H_
#define _SC_FLASH_FS_H_

#include <stddef.h>

#define SC_FLASH_FS_MOUNT_POINT "/lfs"
#define SC_FLASH_FS_MAX_PATH_LEN 64

// Initializes and mounts littlefs.
int sc_flash_fs_init();

#endif  // _SC_FLASH_FS_H_