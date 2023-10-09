#include <sclib/accel.h>
#include <sclib/caster.h>
#include <sclib/led.h>
#include <sclib/macros.h>
#include <sclib/motion_detector.h>
#include <sclib/vibration.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "ble.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#define SC_SIGNAL_QUEUE_SIZE 1

struct caster_cb_queue_item {
  struct sc_accel_entry entry[5 * SC_ACCEL_SAMPLE_RATE_HZ / 2];
  size_t len_bytes;
};

K_MSGQ_DEFINE(caster_cb_queue, sizeof(struct caster_cb_queue_item),
              SC_SIGNAL_QUEUE_SIZE, /*align=*/4);

void caster_cb(uint8_t slot) {
  LOG_DBG("Caster callback for slot %d", slot);
}

struct caster_cb_queue_item item;
// Signal callback.
static void signal_callback(const struct sc_accel_entry *entry, size_t len) {
  memcpy(item.entry, entry, len * sizeof(struct sc_accel_entry));
  item.len_bytes = len * sizeof(struct sc_accel_entry);
  LOG_DBG("Signal callback. Len: %d, total bytes: %d", len, item.len_bytes);
  if (k_msgq_put(&caster_cb_queue, &item, K_NO_WAIT)) {
    LOG_ERR("Failed to put item in queue");
  }
}

struct caster_cb_queue_item qitem;
int main(void) {
  __ASSERT_NO_MSG(!sc_caster_init(caster_cb));
  __ASSERT_NO_MSG(!sc_caster_set_signal_callback(signal_callback));
  __ASSERT_NO_MSG(!sc_ble_init());
  while (true) {
    // Block on getting queue item.
    if (k_msgq_get(&caster_cb_queue, &qitem, K_FOREVER)) {
      LOG_ERR("Failed to get item from queue");
      continue;
    }
    if (sc_ble_send((uint8_t *)item.entry, item.len_bytes)) {
      LOG_ERR("Failed to send item over BLE");
      continue;
    }
  }
  return 0;
}
