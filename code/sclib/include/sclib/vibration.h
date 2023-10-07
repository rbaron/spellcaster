#ifndef _SC_VIB_H_
#define _SC_VIB_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "sclib/macros.h"

#define SC_VIB_FLASH_PERIOD_MS 400

extern struct gpio_dt_spec vib;

int sc_vib_init();

static inline int sc_vib_on() {
  return gpio_pin_set_dt(&vib, 1);
}

static inline int sc_vib_off() {
  return gpio_pin_set_dt(&vib, 0);
}

static inline int sc_vib_toggle() {
  return gpio_pin_toggle_dt(&vib);
}

static inline int sc_vib_flash(int times) {
  LOG_MODULE_DECLARE(vib, LOG_LEVEL_DBG);
  RET_IF_ERR(sc_vib_off());
  for (int i = 0; i < 2 * times; i++) {
    RET_IF_ERR(sc_vib_toggle());
    k_msleep(SC_VIB_FLASH_PERIOD_MS / 2);
  }
  return 0;
}

#endif  // _SC_VIB_H_