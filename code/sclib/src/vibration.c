#include "sclib/vibration.h"

// #include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>

#include "sclib/led.h"
#include "sclib/macros.h"

LOG_MODULE_REGISTER(vib, CONFIG_SCLIB_LOG_LEVEL);

#define A4_PERIOD_NS 2272727
#define A5_PERIOD_NS (A_PERIOD / 2)

struct pwm_dt_spec vib = PWM_DT_SPEC_GET(DT_NODELABEL(pwm_vib));

int sc_vib_init() {
  RET_IF_ERR(!device_is_ready(vib.dev));
  RET_IF_ERR(sc_vib_off());
  return 0;
}

int sc_vib_ready() {
  RET_IF_ERR(
      pwm_set_dt(&vib, SC_VIB_LOW_DUTY_CYCLE_NS, SC_VIB_LOW_DUTY_CYCLE_NS / 2));
  k_msleep(100);
  RET_IF_ERR(sc_vib_off());
  return 0;
}

int sc_vib_yes() {
  RET_IF_ERR(
      pwm_set_dt(&vib, SC_VIB_LOW_DUTY_CYCLE_NS, SC_VIB_LOW_DUTY_CYCLE_NS / 2));
  k_msleep(SC_VIB_FLASH_PERIOD_MS);
  RET_IF_ERR(pwm_set_dt(&vib, SC_VIB_HIGH_DUTY_CYCLE_NS,
                        SC_VIB_HIGH_DUTY_CYCLE_NS / 2));
  k_msleep(SC_VIB_FLASH_PERIOD_MS);
  RET_IF_ERR(sc_vib_off());
  return 0;
}

int sc_vib_no() {
  RET_IF_ERR(pwm_set_dt(&vib, SC_VIB_HIGH_DUTY_CYCLE_NS,
                        SC_VIB_HIGH_DUTY_CYCLE_NS / 2));
  k_msleep(SC_VIB_FLASH_PERIOD_MS);
  RET_IF_ERR(
      pwm_set_dt(&vib, SC_VIB_LOW_DUTY_CYCLE_NS, SC_VIB_LOW_DUTY_CYCLE_NS / 2));
  k_msleep(SC_VIB_FLASH_PERIOD_MS);
  RET_IF_ERR(sc_vib_off());
  return 0;
}

int sc_vib_flash(int times) {
  RET_IF_ERR(sc_vib_off());
  for (int i = 0; i < 2 * times; i++) {
    RET_IF_ERR(sc_vib_on());
    k_msleep(SC_VIB_FLASH_PERIOD_MS);
    RET_IF_ERR(sc_vib_off());
    k_msleep(SC_VIB_FLASH_PERIOD_MS);
  }
  return 0;
}
