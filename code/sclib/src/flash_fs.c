#include "sclib/flash_fs.h"

#include <sclib/macros.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(flash_fs, CONFIG_LOG_DEFAULT_LEVEL);

FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(storage);

static struct fs_mount_t lfs_storage_mnt = {
    .type = FS_LITTLEFS,
    .fs_data = &storage,
    .storage_dev = (void *)FIXED_PARTITION_ID(storage_partition),
    .mnt_point = SC_FLASH_FS_MOUNT_POINT,
};

int sc_flash_fs_init() {
  // TODO: if booting for the first time after a full flash erase, fs_mount will
  // complain (via a LOG_ERR) and then automatically format the flash. It all
  // works, but avoiding a scary red message would be ideal. Maybe somehow check
  // if it's formatted before mounting?
  RET_IF_ERR(fs_mount(&lfs_storage_mnt));

  // Print free space.
  struct fs_statvfs stat;
  RET_IF_ERR(fs_statvfs(SC_FLASH_FS_MOUNT_POINT, &stat));
  // Total space.
  LOG_INF("Flash FS size: %lu bytes", stat.f_frsize * stat.f_blocks);
  // Free space.
  LOG_INF("Flash FS free: %lu bytes", stat.f_frsize * stat.f_bfree);
  return 0;
}