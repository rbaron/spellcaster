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

static struct sc_accel_entry buf[52];

int main(void) {
  // __ASSERT_NO_MSG(!sc_caster_init(caster_cb));
  __ASSERT_NO_MSG(!sc_vib_init());
  __ASSERT_NO_MSG(!sc_ble_init());
  // struct caster_cb_queue_item qitem;
  while (true) {
    sc_ble_send(buf, sizeof(buf));
    k_msleep(2000);
  }
  // k_sleep(K_FOREVER);
}
