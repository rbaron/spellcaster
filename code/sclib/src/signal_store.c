#include "sclib/signal_store.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>
// #include <zephyr/fs/littlefs.h>

#include "sclib/flash_fs.h"
#include "sclib/macros.h"

// LOG_MODULE_REGISTER(signal_store, CONFIG_SCLIB_LOG_LEVEL);
LOG_MODULE_REGISTER(signal_store, LOG_LEVEL_DBG);

struct stored_signal {
  uint8_t slot;
  uint8_t version;
  bool has_signal;
  struct sc_signal signal;
};

// Temporary hack: store in memory
// TODO: store in flash.
static struct stored_signal store[SC_SIGNAL_STORE_MAX_SIGNALS];

static void mk_filename(uint8_t slot, uint8_t *filename) {
  snprintf(filename, SC_FLASH_FS_MAX_PATH_LEN,
           //  SC_FLASH_FS_MOUNT_POINT "/signals/%d", slot);
           SC_FLASH_FS_MOUNT_POINT "/%d", slot);
}

static int read_from_flash(uint8_t slot, struct stored_signal *signal) {
  // Open file for reading.
  uint8_t filename[SC_FLASH_FS_MAX_PATH_LEN];
  mk_filename(slot, filename);

  LOG_DBG("Reading from to %s", filename);

  struct fs_file_t file;
  fs_file_t_init(&file);
  RET_IF_ERR(fs_open(&file, filename, FS_O_READ));
  RET_CHECK(fs_read(&file, signal, sizeof(*signal)) == sizeof(*signal),
            "fs_read failed");
  RET_IF_ERR(fs_close(&file));
  return 0;
}

int sc_ss_init(void) {
  RET_IF_ERR(sc_flash_fs_init());
  for (int i = 0; i < SC_SIGNAL_STORE_MAX_SIGNALS; i++) {
    // Try to read from flash.
    if (!read_from_flash(i, &store[i])) {
      LOG_INF("loaded signal on slot %d, len %d", i, store[i].signal.len);
    } else {
      LOG_INF("no signal on slot %d", i);
      store[i].has_signal = false;
    }
  }
  return 0;
}

int sc_ss_store(uint8_t slot, const struct sc_signal *signal) {
  LOG_DBG("Storing signal with len %d on slot %d", signal->len, slot);
  struct stored_signal stored_sig = {
      .slot = slot,
      .version = 0,
      .has_signal = true,
      .signal = *signal,
  };
  if (slot >= SC_SIGNAL_STORE_MAX_SIGNALS) {
    LOG_ERR("slot >= SC_SIGNAL_STORE_MAX_SIGNALS");
    return -1;
  }
  store[slot] = stored_sig;
  LOG_DBG("stored signal on slot %d, len %d", stored_sig.slot,
          stored_sig.signal.len);

  // Store in flash.
  uint8_t filename[SC_FLASH_FS_MAX_PATH_LEN];
  mk_filename(slot, filename);

  LOG_DBG("Writing to %s", filename);

  // Try to delete first --
  // https://github.com/zephyrproject-rtos/zephyr/issues/60890.
  fs_unlink(filename);

  struct fs_file_t file;
  fs_file_t_init(&file);
  RET_IF_ERR(fs_open(&file, filename, FS_O_CREATE | FS_O_RDWR));
  RET_CHECK(
      fs_write(&file, &stored_sig, sizeof(stored_sig)) == sizeof(stored_sig),
      "fs_write failed");
  RET_IF_ERR(fs_close(&file));
  return 0;
}

int sc_ss_load(uint8_t slot, struct sc_signal *signal) {
  if (slot >= SC_SIGNAL_STORE_MAX_SIGNALS) {
    LOG_ERR("slot >= SC_SIGNAL_STORE_MAX_SIGNALS");
    return -1;
  }
  if (!store[slot].has_signal) {
    LOG_ERR("slot %d has no signal", slot);
    return -1;
  }
  *signal = store[slot].signal;
  LOG_DBG("loaded signal on slot %d, len %d", slot, signal->len);
  return 0;
}
