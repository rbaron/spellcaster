#include <sclib/accel.h>
#include <sclib/macros.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

int main(void) {
  sc_accel_init();
  struct sc_accel_entry entry;
  while (true) {
    if (!sc_accel_read(&entry)) {
      LOG_DBG("%10d %10d %10d %10d %10d %10d", entry.ax, entry.ay, entry.az,
              entry.gx, entry.gy, entry.gz);
    }
    k_sleep(K_MSEC(10));
  }
}
