#include <sclib/accel.h>
#include <sclib/caster.h>
#include <sclib/led.h>
#include <sclib/macros.h>
#include <sclib/motion_detector.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

int main(void) {
  sc_led_init();
  // sc_accel_init();
  sc_caster_init();

  // struct sc_motion_detector md;
  // sc_md_init(&md);

  // sd_led_flash(2);
  // __ASSERT_NO_MSG(!sc_caster_init());

  // struct sc_accel_entry entry;
  while (true) {
    // if (!sc_accel_read(&entry)) {
    //   sc_md_ingest(&md, &entry);
    // }
    // LOG_DBG("Main thread is running.");
    k_msleep(1000);
  }
}
