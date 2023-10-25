// #include <ram_pwrdn.h>
#include <sclib/adc.h>
#include <sclib/button.h>
#include <sclib/caster.h>
#include <sclib/led.h>
#include <sclib/macros.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#include "ble.h"
#include "hid.h"

// LOG_MODULE_REGISTER(app, CONFIG_LOG_DEFAULT_LEVEL);
LOG_MODULE_REGISTER(app, LOG_LEVEL_DBG);

// struct sleep_context {
//   struct k_work_delayable dwork;
//   bool sleeping;
// };

// static struct sleep_context sleep_ctx = {
//     .sleeping = false,
// };

// static void go_to_sleep(struct k_work *work) {
//   struct k_work_delayable *dwork = k_work_delayable_from_work(work);
//   struct sleep_context *ctx = CONTAINER_OF(dwork, struct sleep_context,
//   dwork); ctx->sleeping = true; LOG_WRN("Going to sleep"); bt_disable();
// }

// Keymap.
static uint8_t get_key_from_slot(uint8_t slot) {
  // https://docs.circuitpython.org/projects/hid/en/latest/_modules/adafruit_hid/keycode.html
  // 0, 1, 2...
  static const uint8_t keymap[] = {0x27, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23};
  return keymap[slot];
}

static void caster_cb(uint8_t slot) {
  // Maybe wake up.
  // if (sleep_ctx.sleeping) {
  //   sleep_ctx.sleeping = false;
  //   LOG_WRN("Waking up");
  //   // bt_enable(NULL);
  //   // settings_load();
  //   // sc_led_flash(/*times=*/1);
  //   // Reset.
  //   // sys_reboot();
  //   NVIC_SystemReset();
  //   return;
  // }

  // __ASSERT_NO_MSG(
  //     k_work_reschedule(&sleep_ctx.dwork,
  //                       K_MSEC(1000 * CONFIG_SC_SLEEP_TIMEOUT_SEC)) >= 0);

  LOG_INF("caster_cb: slot %d", slot);
  uint8_t key = get_key_from_slot(slot);
  if (!key) {
    LOG_ERR("Failed to get key from button: %d", slot);
    return;
  }
  int err = sc_ble_send_button_press(sc_hid_get_hids_obj(), key);
  if (err) {
    LOG_ERR("Failed to send button press: %d", err);
  }
  err = sc_ble_send_button_release(sc_hid_get_hids_obj(), key);
  if (err) {
    LOG_ERR("Failed to send button release: %d", err);
  }
}

void main(void) {
  LOG_DBG("Will init");
  __ASSERT_NO_MSG(!sc_caster_init(caster_cb));
  __ASSERT_NO_MSG(!sc_adc_init());
  __ASSERT_NO_MSG(!sc_adc_init());
  __ASSERT_NO_MSG(!sc_ble_init());
  LOG_DBG("Will init HID");
  __ASSERT_NO_MSG(!sc_hid_init());
  LOG_DBG("Will start advertising");
  __ASSERT_NO_MSG(!sc_ble_start_advertising());

  // Initialize delayed sleeping work.
  // k_work_init_delayable(&sleep_ctx.dwork, go_to_sleep);
  // __ASSERT_NO_MSG(
  //     k_work_reschedule(&sleep_ctx.dwork,
  //                       K_MSEC(1000 * CONFIG_SC_SLEEP_TIMEOUT_SEC)) >= 0);

  // Probably shut down unused ram?
  // power_down_unused_ram();

  while (1) {
    k_sleep(K_FOREVER);
  }
}
