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
  sc_vib_flash(1);
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

enum VibAsyncMsg {
  VIB_ASYNC_NONE,
  VIB_ASYNC_YES,
};

K_MSGQ_DEFINE(sc_vib_msgq, sizeof(enum VibAsyncMsg),
              /*max_elements=*/10, /*align=*/4);

static void vib_async_thread(void) {
  enum VibAsyncMsg msg;
  while (true) {
    __ASSERT_NO_MSG(!k_msgq_get(&sc_vib_msgq, &msg, K_FOREVER));
    switch (msg) {
      case VIB_ASYNC_NONE:
        break;
      case VIB_ASYNC_YES:
        sc_vib_yes();
        break;
    }
  }
}

K_THREAD_DEFINE(sc_vib_tid, /*stack_size=*/1024, vib_async_thread, /*p1=*/NULL,
                /*p2=*/NULL, /*p3=*/NULL,
                /*priority=*/5, /*options=*/0, /*delay=*/0);

int sc_vib_yes_async() {
  enum VibAsyncMsg msg = VIB_ASYNC_YES;
  RET_IF_ERR(k_msgq_put(&sc_vib_msgq, &msg, K_NO_WAIT));
  return 0;
}