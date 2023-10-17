#ifndef _SC_VIB_H_
#define _SC_VIB_H_

// #include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "sclib/macros.h"

#define SC_VIB_FLASH_PERIOD_MS 125

#define SC_VIB_HIGH_DUTY_CYCLE_NS 10e6
#define SC_VIB_LOW_DUTY_CYCLE_NS 100e6

extern struct pwm_dt_spec vib;

int sc_vib_init();

static inline int sc_vib_on() {
  return pwm_set_dt(&vib, SC_VIB_HIGH_DUTY_CYCLE_NS,
                    SC_VIB_HIGH_DUTY_CYCLE_NS / 2);
}

static inline int sc_vib_off() {
  return pwm_set_dt(&vib, 0, 0);
}

int sc_vib_ready();

int sc_vib_yes();

int sc_vib_yes_async();

int sc_vib_no();

int sc_vib_flash(int times);

#endif  // _SC_VIB_H_