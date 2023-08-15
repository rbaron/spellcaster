#include "sclib/signal_store.h"

#include <stdbool.h>
#include <zephyr/logging/log.h>

#define SC_SIGNAL_STORE_MAX_SIGNALS 1

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

int sc_ss_init(void) {
  for (int i = 0; i < SC_SIGNAL_STORE_MAX_SIGNALS; i++) {
    store[i].has_signal = false;
  }
  return 0;
}

int sc_ss_store(uint8_t slot, const struct sc_signal *signal) {
  LOG_DBG("Storing signal with len %d", signal->len);
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
