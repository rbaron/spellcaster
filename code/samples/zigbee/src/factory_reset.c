#include "factory_reset.h"

#include <hal/nrf_power.h>
#include <sclib/button.h>
#include <sclib/led.h>
#include <zephyr/logging/log.h>
#include <zigbee/zigbee_app_utils.h>

LOG_MODULE_REGISTER(factory_reset, CONFIG_LOG_DEFAULT_LEVEL);

static int factory_reset() {
  LOG_WRN("Factory resetting device");
  zb_bdb_reset_via_local_action(/*param=*/0);
  return 0;
}

static void timer_do_reset(zb_uint8_t unused_param) {
  LOG_WRN("SW1 button was pressed for 5 seconds, factory resetting device");
  sc_led_flash(/*times=*/5);
  factory_reset();
}

static void sw1_factory_reset_check_timer_cb(struct k_timer *timer_id) {
  if (!sc_button_poll(SC_BUTTON_A)) {
    LOG_DBG("SW1 button was released, will not factory reset device");
    return;
  }
  ZB_SCHEDULE_APP_CALLBACK(timer_do_reset, /*param=*/0);
}

K_TIMER_DEFINE(sw1_factory_reset_check_timer, sw1_factory_reset_check_timer_cb,
               NULL);

int sc_zb_factory_reset_check() {
  if (sc_button_poll(SC_BUTTON_A)) {
    LOG_DBG("A pressed. Scheduling timer");
    k_timer_start(&sw1_factory_reset_check_timer, K_SECONDS(5), K_NO_WAIT);
  }
  return 0;
}