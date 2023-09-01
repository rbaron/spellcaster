#include <sclib/accel.h>
#include <sclib/caster.h>
#include <sclib/led.h>
#include <sclib/macros.h>
#include <sclib/motion_detector.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

void caster_cb(uint8_t slot) {
  LOG_DBG("Caster callback for slot %d", slot);
}

int main(void) {
  __ASSERT_NO_MSG(!sc_caster_init(caster_cb));
  while (true) {
    sc_led_toggle();
    k_sleep(K_MSEC(500));
  }
}
