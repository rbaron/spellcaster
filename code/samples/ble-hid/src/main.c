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

LOG_MODULE_REGISTER(app, LOG_LEVEL_DBG);

// Keymap.
static uint8_t get_key_from_slot(uint8_t slot) {
  // https://docs.circuitpython.org/projects/hid/en/latest/_modules/adafruit_hid/keycode.html
  // 0, 1, 2...
  static const uint8_t keymap[] = {
      // 0: 0
      0x27,
      // 1: 1
      0x1e,
      // 2: Play/Pause -- Doesn't really work like this.
      // https://docs.circuitpython.org/projects/hid/en/latest/_modules/adafruit_hid/consumer_control_code.html
      0xcd,
      // 3: R
      0x15,
      // 4: B
      0x05,
      // 5: 5
      0x22,
      // 6: 5
      0x23,
  };
  return keymap[slot];
}

static void caster_cb(uint8_t slot) {
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
  __ASSERT_NO_MSG(!sc_ble_init());
  LOG_DBG("Will init HID");
  __ASSERT_NO_MSG(!sc_hid_init());
  LOG_DBG("Will start advertising");
  __ASSERT_NO_MSG(!sc_ble_start_advertising());

  // Probably shut down unused ram?
  // power_down_unused_ram();

  while (1) {
    k_sleep(K_FOREVER);
  }
}
