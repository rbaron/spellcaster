#ifndef _SC_LED_H_
#define _SC_LED_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "sclib/macros.h"

#define SC_LED_FLASH_PERIOD_MS 400

extern struct gpio_dt_spec led;

int sc_led_init();

static inline int sc_led_on() {
  return gpio_pin_set_dt(&led, 1);
}

static inline int sc_led_off() {
  return gpio_pin_set_dt(&led, 0);
}

static inline int sc_led_toggle() {
  return gpio_pin_toggle_dt(&led);
}

static inline int sc_led_flash(int times) {
  LOG_MODULE_DECLARE(led, LOG_LEVEL_DBG);
  RET_IF_ERR(sc_led_off());
  for (int i = 0; i < 2 * times; i++) {
    RET_IF_ERR(sc_led_toggle());
    k_msleep(SC_LED_FLASH_PERIOD_MS / 2);
  }
  return 0;
}

#endif  // _SC_LED_H_