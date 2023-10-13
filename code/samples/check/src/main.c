#include <math.h>
#include <sclib/accel.h>
#include <sclib/led.h>
#include <sclib/macros.h>
#include <sclib/vibration.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

int main(void) {
  __ASSERT_NO_MSG(!sc_led_init());
  __ASSERT_NO_MSG(!sc_vib_init());
  __ASSERT_NO_MSG(!sc_accel_init());

  struct sc_accel_entry entry;
  float angle;
  while (true) {
    if (!sc_accel_read(&entry)) {
      angle = atan2(-1 * entry.ax, entry.az) * 180 / 3.14159265;
      LOG_DBG("%10d %10d %10d %10d %10d %10d %10.2f deg", entry.ax, entry.ay,
              entry.az, entry.gx, entry.gy, entry.gz, angle);
    }
    k_sleep(K_MSEC(10));
  }
}
