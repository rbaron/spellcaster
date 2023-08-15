#include <sclib/accel.h>
#include <sclib/caster.h>
#include <sclib/led.h>
#include <sclib/macros.h>
#include <sclib/motion_detector.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "ble.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#define SC_BUTTON_PRESS_QUEUE_SIZE 8

struct caster_cb_queue_item {
  uint8_t slot;
};

K_MSGQ_DEFINE(caster_cb_queue, sizeof(struct caster_cb_queue_item),
              SC_BUTTON_PRESS_QUEUE_SIZE, /*align=*/4);

void caster_cb(uint8_t slot) {
  LOG_DBG("Caster callback for slot %d", slot);
  struct caster_cb_queue_item item = {.slot = slot};
  if (k_msgq_put(&caster_cb_queue, &item, K_NO_WAIT)) {
    LOG_ERR("Failed to put item in queue");
  }
}

static void broadcast_action(uint8_t slot) {
  if (sc_ble_set_advertising_data(slot + 1)) {
    LOG_ERR("Failed to set advertising data");
    return;
  }
  sc_ble_start_advertising();
  k_msleep(CONFIG_SC_BLE_ADV_DURATION_MSEC);
  sc_ble_stop_advertising();
}

int main(void) {
  __ASSERT_NO_MSG(!sc_caster_init(caster_cb));
  __ASSERT_NO_MSG(!sc_ble_init());

  // sc_ble_set_advertising_data(1);
  // sc_ble_start_advertising();
  // k_msleep(2000);
  // sc_ble_stop_advertising();

  struct caster_cb_queue_item qitem;
  while (true) {
    k_msgq_get(&caster_cb_queue, &qitem, K_FOREVER);
    LOG_DBG("Got queue item: slot %d", qitem.slot);
    broadcast_action(qitem.slot);
  }
}
